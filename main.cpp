#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "buffer.h"
#include "general/event_handler.h"
#include "femmesh.h"
#include "quadedge.h"
#include "render_target.h"
#include "shader_loader.h"
#include "solver.h"
#include "shader.h"
#include "vao.h"
#include "comparators.h"
#include "exprtk_wrapper.h"

class ControlKeyListener : public KeyPressListener {
    public:
        ControlKeyListener(EventHandler *ctx) {
            ctx->addListener((KeyPressListener*)this);
            this->ctx = ctx;
        }
        virtual void onKeyboardAction(KeyPress press) override {
            if(press.action != GLFW_PRESS) {
                return;
            }
            switch(press.key) {
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(ctx->window, true);
                    break;
                default:
                    break;
            }
        }
    private:
        bool keyPressed = false;
        EventHandler *ctx;
};

static EventHandler *ctx;
static time_t t0 = 0;
static glm::vec2 cursorPos;

std::shared_ptr<std::vector<Node>> perturbedNodeGrid(int size, double range, const RuntimeExpression &u, int randomness) {
    auto nodes = std::make_shared<std::vector<Node>>();
    
    double delta = range / (float)size;

    //BOUNDARIES===============================================
    //left boundary
    for(int y = -size; y <= size; y++) {
        Node v = {{-range, y * delta}, dirichlet, u(-range, y*delta)};
        nodes->push_back(v);
    }

    for(int x = -size + 1; x <= size - 1; x++) {
        //top boundary
        Node v = {{x*delta, -range}, dirichlet, u(x*delta, -range)};
        nodes->push_back(v);

        for(int y = -size + 1; y <= size - 1; y++) {
            Node v = {{x*delta , y*delta}, active, 0.0f};
            v.position += glm::vec2{ ((double)rand() / RAND_MAX - 0.5) * (delta/randomness), (double)rand() / RAND_MAX * (delta/randomness) };
            nodes->push_back(v);
        }
        //bottom boundary
        v = {{x*delta, range}, dirichlet, u(x*delta, range)};
        nodes->push_back(v);
    }

    //right boundary
    for(int y = -size; y <= size; y++) {
        Node v = {{range, y*delta}, dirichlet, u(range, y*delta)};
        nodes->push_back(v);
    }
    //=========================================================

    return std::move(nodes);
}

bool notDegenerate(glm::dvec2 a, glm::dvec2 b, glm::dvec2 c) {
    return ((a.x - b.x)*(a.y - c.y) - (a.x - c.x)*(a.y - b.y)) != 0;
}

//bfs over the dual graph
std::vector<unsigned int> genElements(const QuadEdge &q) {
    std::vector<unsigned int> indices;
    std::deque<Edge*> searchfront;

    Edge *start = q.edgeRecords[0]->edges[0].rotCW();
    searchfront.push_front(start);

    int limiter = 0;
    while(!searchfront.empty() && limiter < 2*q.edgeRecords.size()) {
        //get dual
        unsigned int buf[3];

        Edge *e = searchfront.back();
        searchfront.pop_back();
        Edge *first = e->Onext();

        for(int i = 0; i < 3; i++) {
            buf[i] = e->rotCCW()->orig();
            e = e->Onext();
        }

        if(e->Onext() == first &&
                notDegenerate(q.nodes->at(buf[0]).position, q.nodes->at(buf[1]).position, q.nodes->at(buf[2]).position)) {

            indices.push_back(buf[0]);
            indices.push_back(buf[1]);
            indices.push_back(buf[2]);
        }
        e->setOrig(-2);

        Edge *erNext = e->sym();
        first = erNext;

        while(erNext->orig() >= 0) {
            searchfront.push_front(erNext);
            erNext->setOrig(-1);

            erNext = erNext->sym()->Onext()->sym();
            if(erNext == first) {
                break;
            }
        }
        limiter++;
    }
    return indices;
}

glm::vec3 interpolate3(float val, glm::vec3 first, glm::vec3 second, glm::vec3 third) {
    return glm::max(1.0f - glm::abs(val), 0.0f) * second
                + glm::max(1.0f - glm::abs(val + 1.0f), 0.0f) * first
                + glm::max(1.0f - glm::abs(val - 1.0f), 0.0f) * third;
}

VertexArray visTriangulation(std::vector<unsigned int> &mIndices, const std::vector<Node> &nodes) {
    std::vector<mVertex> mVertices;

    for(const Node &n : nodes) {
        mVertices.push_back({n.position, 1.0f});
    }

    VertexArray triangleMesh;
    triangleMesh.bind();
    triangleMesh.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, mVertices.size() * sizeof(mVertex), mVertices.data()));
    triangleMesh.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), mIndices.data()));

    triangleMesh.indexCount = mIndices.size();
    triangleMesh.vertexCount = mVertices.size();
    triangleMesh.addAttrib(VertexAttrib{2, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, pos)});
    triangleMesh.addAttrib(VertexAttrib{1, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, value)});
    return triangleMesh;
}

SkeletonMesh getGrid(int size, double range) {
    std::vector<unsigned int> indices;
    std::vector<unsigned int> boundary;
    std::vector<glm::dvec2> vertices;

    double d = range / size;

    for(int y = -size; y <= size; y++) {
        boundary.push_back(vertices.size());
        vertices.push_back({-range , y*d});
    }

    for(int x = -size + 1; x <= size - 1; x++) {
        boundary.push_back(vertices.size());
        vertices.push_back({x*d , -range});

        for(int y = -size + 1; y <= size - 1; y++) {
            vertices.push_back({x*d , y*d});
        }

        boundary.push_back(vertices.size());
        vertices.push_back({x*d , range});
    }

    for(int y = -size; y <= size; y++) {
        boundary.push_back(vertices.size());
        vertices.push_back({range , y*d});
    }

    int gridsize = size*2 + 1;
    for(unsigned int i = 0; i < gridsize - 1; i++) {
        for(unsigned int j = 0; j < gridsize - 1; j++) {
            indices.push_back(i*gridsize + j);
            indices.push_back((i+1)*gridsize + j);
            indices.push_back(i*gridsize + j + 1);

            indices.push_back((i+1)*gridsize + j + 1);
            indices.push_back(i*gridsize + j + 1);
            indices.push_back((i+1)*gridsize + j);
        }
    }

    return {std::move(vertices), std::move(indices), std::move(boundary)};
}

auto indZorderedCmp(const std::vector<glm::dvec2> &vertices, double offset = 1.0) {
    return [&vertices, offset] (const unsigned int &a, const unsigned int &b) {
        return compareZorder(vertices[a] + offset, vertices[b] + offset);
    };
}

void zOrderMesh(SkeletonMesh &skMesh) {
    std::vector<glm::dvec2> sortedVertices(skMesh.vertices.size());
    std::vector<unsigned int> oughtToAt(skMesh.vertices.size()); // idx -> idx_sorted
    std::vector<unsigned int> atToOught(skMesh.vertices.size()); // idx_sorted -> idx

    for(int i = 0; i < skMesh.vertices.size(); i++) {
        oughtToAt[i] = i;
    }

    std::sort(oughtToAt.begin(), oughtToAt.end(), indZorderedCmp(skMesh.vertices));

    for(int i = 0; i < skMesh.vertices.size(); i++) {
        sortedVertices[i] = skMesh.vertices[oughtToAt[i]];
        atToOught[oughtToAt[i]] = i;
    }
    skMesh.vertices = std::move(sortedVertices);

    for(int i = 0; i < skMesh.indices.size(); i++) {
        skMesh.indices[i] = atToOught[skMesh.indices[i]];
    }

    for(int i = 0; i < skMesh.boundary.size(); i++) {
        skMesh.boundary[i] = atToOught[skMesh.boundary[i]];
    }
}

std::pair<std::shared_ptr<std::vector<Node>>, std::vector<unsigned int>> perfectNodeGrid(int size, double range, const RuntimeExpression &u) {
    auto nodes = std::make_shared<std::vector<Node>>();

    SkeletonMesh grid = getGrid(size, range);
    zOrderMesh(grid);

    for(auto pos : grid.vertices) {
        Node v = {pos, active, 0.0};
        nodes->push_back(v);
    }

    for(auto ind : grid.boundary) {
        Node &node = nodes->at(ind);
        node.type = dirichlet;
        node.value = u(node.position.x, node.position.y);
    }

    return std::make_pair(nodes, grid.indices);
}

/*
VertexArray meshGr(const SkeletonMesh &grid, const std::vector<double> values) {
    std::vector<mVertex> mVertices;

    glm::vec3 lowColor = {0.05, 0.05, 0.05};
    glm::vec3 midColor = {0.2, 0.2, 0.5};
    glm::vec3 highColor = {1.0, 0.3, 0.0};

    double max = 0;
    for(double val: values) {
        if(glm::abs(val) > max) {
            max = glm::abs(val);
        }
    }
    max = glm::max(1.0, max);

    int activeNr = 0;
    for(int i = 0; i < values.size(); i++) {
        float colorVal = glm::atan(glm::clamp(values[i] - 1.0, -1.0, 5.0));
        glm::vec3 color = glm::pow(interpolate3(colorVal, lowColor, midColor, highColor), glm::vec3(1.25f));
        mVertices.push_back({grid.vertices[i], 0.1});
    }

    VertexArray triangleMesh;
    triangleMesh.bind();
    triangleMesh.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, mVertices.size() * sizeof(mVertex), mVertices.data()));
    triangleMesh.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, grid.indices.size() * sizeof(unsigned int), grid.indices.data()));

    triangleMesh.indexCount = grid.indices.size();
    triangleMesh.addAttrib(VertexAttrib{2, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, pos)});
    triangleMesh.addAttrib(VertexAttrib{3, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, value)});
    return triangleMesh;
}
*/

VertexArray visSolution(const std::unique_ptr<FemMesh> &femmesh, int size, const std::vector<double> &solution) {
    std::vector<mVertex> mVertices;
    std::vector<unsigned int> mIndices;

    const std::vector<Node> &nodes = *femmesh->nodes;

    int activeNr = 0;
    for(int i = 0; i < nodes.size(); i++) {
        float colorVal = 0.0;
        if(nodes[i].type != dirichlet) {
            colorVal = solution[activeNr];
            activeNr++;
        } else {
            colorVal = nodes[i].value;
        }
//      glm::vec3 color = glm::pow(interpolate3(colorVal, lowColor, midColor, highColor), glm::vec3(1.25f));
        mVertex v = {nodes[i].position, colorVal};
        mVertices.push_back(v);
    }

    for(int i = 0; i < femmesh->elems.size(); i++) {
        mIndices.push_back(femmesh->indexOfNodeOfElement(i, 0));
        mIndices.push_back(femmesh->indexOfNodeOfElement(i, 1));
        mIndices.push_back(femmesh->indexOfNodeOfElement(i, 2));
    }

    VertexArray triangleMesh;
    triangleMesh.bind();
    triangleMesh.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, mVertices.size() * sizeof(mVertex), mVertices.data()));
    triangleMesh.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), mIndices.data()));

    triangleMesh.indexCount = mIndices.size();
    triangleMesh.addAttrib(VertexAttrib{2, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, pos)});
    triangleMesh.addAttrib(VertexAttrib{1, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, value)});
    return triangleMesh;
}


time_t getsystime() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

time_t gettime() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count() - t0;
}

VertexArray getScreenQuad() {
    float quadVertices[] = {
        -1.0, -1.0, 0.0, 0.0,
        1.0, -1.0, 1.0, 0.0,
        1.0, 1.0, 1.0, 1.0,
        -1.0, 1.0, 0.0, 1.0,
        -1.0, -1.0, 0.0, 0.0,
        1.0, 1.0, 1.0, 1.0,
    };

    VertexArray screenQuad;
    {
        screenQuad.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices));
        screenQuad.addAttrib(VertexAttrib{2, sizeof(float) * 4, GL_FLOAT, (void*)0});
        screenQuad.addAttrib(VertexAttrib{2, sizeof(float) * 4, GL_FLOAT, (void*)(2 * sizeof(float))});
    }
    return screenQuad;
}

class CursorListenerImpl : public CursorListener {
    public:
        CursorListenerImpl(int wWidth, int wHeight) {
            this->wWidth = wWidth;
            this->wHeight = wHeight;
        }

        int wWidth;
        int wHeight;

        void onCursorMove(Cursor cursor) {
            cursorPos.x = 2 * ((cursor.pos.x) / wWidth) - 1.f;
            cursorPos.y = 2 * ((wHeight - cursor.pos.y) / wHeight) - 1.f;
        }

};

unsigned int getLaplacianShader(std::string expr) {
    std::string fCode = ShaderLoader::loadFileParametrized("general/graphics/glsl/laplacian.frag", "F_IMPL", expr);
    unsigned int fShader = ShaderLoader::compileFragmentShader(fCode.c_str());

    std::string vCode = ShaderLoader::loadFile("general/graphics/glsl/post.vert");
    unsigned int vShader = ShaderLoader::compileVertexShader(vCode.c_str());

    unsigned int laplacianId = ShaderLoader::createShaderProgram(vShader, fShader); 

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return laplacianId;
}

int main(int argc, char* argv[]) {

    const int wWidth = 1000, wHeight = 1000;

    ctx = new EventHandler("Test", glm::vec2(wWidth, wHeight));
    CursorListenerImpl cl(wWidth, wHeight);
    ctx->addListener(&cl);

    ctx->disable(GL_DEPTH_TEST);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(ctx->window, true);
    ImGui_ImplOpenGL3_Init();

    RenderTarget pixelFb({wWidth, wHeight});

    const double range = 1.0;
    int elementSubdivision = 3;
    int prElementSubdivisionSize = elementSubdivision;

    bool updateNeccessary = false;

    int randomness = 5;
    int prRandomness = randomness;

    t0 = gettime();
    time_t time = 0;

    VertexArray screen = getScreenQuad();
    RenderTarget screenFb({wWidth, wHeight});

    //delanuay triangualte arbitrary node collection
    /*
    auto qedge = std::make_unique<QuadEdge>(nodes);
    qedge->triangulate();
    time = gettime() - t0;
    printf("t: %ld; Triangulated!\n", time);

    //create elements from triangulation
    auto els = genElements(*qedge);
    time = gettime() - t0;
    printf("t: %ld; Elements genned!\n", time);
    */

    RuntimeExpression fExpr("0.0");
    RuntimeExpression uExpr("x");

    //create mesh
    auto meshdata = perfectNodeGrid(elementSubdivision, range, uExpr);
    auto fTriangles = std::make_unique<FemMesh>(meshdata.first, meshdata.second);

    time = gettime();
    printf("t: %ld; Mesh init!\n", time);

    Solver solver;

    time = gettime();
    printf("t: %ld; Solving...\n", time);
    auto solution = solver.solve(*fTriangles, fExpr);
    time = gettime();
    printf("t: %ld; Solved! colors.size %zu\n", time, solution.size());

    bool renderError = false;
    bool prRenderError = renderError;

    float hError = 0.5*sqrt(range / elementSubdivision);
    float prHError = hError;

    VertexArray solMesh = visSolution(fTriangles, elementSubdivision, solution);
    time = gettime();
    printf("t: %ld; Solution mesh!\n", time);

    const Shader color("general/graphics/glsl/color.vert", "general/graphics/glsl/color.frag");
    const Shader whiteLines("general/graphics/glsl/color.vert", "general/graphics/glsl/white.frag");

    glm::vec3 lowColor = {0.05, 0.05, 0.05};
    glm::vec3 midColor = {0.2, 0.2, 0.5};
    glm::vec3 highColor = {1.0, 0.3, 0.0};

    const Shader post("general/graphics/glsl/post.vert", "general/graphics/glsl/post.frag");
    Shader *laplacian = new Shader(getLaplacianShader(fExpr.exprStr));

    post.use();
    post.setVec3("lowColor", lowColor);
    post.setVec3("midColor", midColor);
    post.setVec3("highColor", highColor);

    glm::vec3 thresholds = {-5.0f, 0.0f, 5.0f};

    laplacian->use();
    laplacian->setVec3("lowColor", lowColor);
    laplacian->setVec3("midColor", midColor);
    laplacian->setVec3("highColor", highColor);

    std::string fShaderRes = "Shader Success", fExprtkRes = "Success", uExprtkRes = "Success";

    glfwSwapInterval(1);
    glPointSize(10.0f);
    while(!glfwWindowShouldClose(ctx->window)) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        {
            ImGui::Begin("Options");
            ImGui::SliderInt("solution resolution", &elementSubdivision, 1, 100);
            ImGui::SliderFloat("error diff step", &hError, 0.0f, 0.5f);

            ImGui::SliderFloat("lowest", &thresholds.x, -10.f, 10.f);
            ImGui::SliderFloat("mid", &thresholds.y, -10.f, 10.f);
            ImGui::SliderFloat("highest", &thresholds.z, -10.f, 10.f);

            ImGui::Checkbox("show error", &renderError);
            ImGui::End();

            ImGui::Begin("Expressions");
            
            ImGui::InputTextMultiline("f(x,y) - Poisson", &fExpr.exprStr);
            ImGui::InputTextMultiline("u(x,y) - Dirichlet", &uExpr.exprStr);

            if(ImGui::Button("Compile and refresh")) {
                fShaderRes = "Shader Success";
                fExprtkRes = "Success";
                uExprtkRes = "Success";

                if(!fExpr.update()) {
                    fExprtkRes = "Fail";
                }
                if(!uExpr.update()) {
                    uExprtkRes = "Fail";
                }

                unsigned int laplacianShaderId = 0;
                try {
                    laplacianShaderId = getLaplacianShader(fExpr.exprStr);
                } catch(const std::runtime_error &e) {
                    fShaderRes = e.what();
                    laplacianShaderId = 0;
                }

                if(laplacianShaderId != 0) {
                    delete laplacian;
                    laplacian = new Shader(laplacianShaderId);

                    laplacian->use();
                    laplacian->setVec3("lowColor", lowColor);
                    laplacian->setVec3("midColor", midColor);
                    laplacian->setVec3("highColor", highColor);
                }
                updateNeccessary = true;
            }

            ImGui::Text("f: %s\n"
                    "   %s\n"
                    "u: %s\n", fExprtkRes.c_str(), fShaderRes.c_str(), uExprtkRes.c_str());

            ImGui::End();
        }

        unsigned int indexcount = 0;

        if(prElementSubdivisionSize != elementSubdivision || randomness != prRandomness || updateNeccessary) {
//          *nodes = *perturbedNodeGrid(elementSubdivision, range, u, randomness);
            auto newmesh = perfectNodeGrid(elementSubdivision, range, uExpr);
            fTriangles->remesh(newmesh.first, newmesh.second);

            time = gettime();
            printf("t: %ld; Solving...\n", time);

            solution = solver.solve(*fTriangles, fExpr);

            time = gettime();
            printf("t: %ld; Solved!\n", time);

            solMesh = visSolution(fTriangles, elementSubdivision, solution);

            prElementSubdivisionSize = elementSubdivision;
            prRandomness = randomness;
            hError = range/elementSubdivision;
            updateNeccessary = false;
        }


        solMesh.bind();
        indexcount = solMesh.indexCount;

        //Draw to buffer ===================================================
        pixelFb.bind();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        color.use();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, indexcount, GL_UNSIGNED_INT, 0);


        //Draw to screen ===================================================
        ctx->bindFramebuffer(0);
        screen.bind();

        pixelFb.getColor(0).bind(0);

        if(renderError) {
            laplacian->use();
            laplacian->setInt("screenTexture", 0);
            laplacian->setFloat("diffstep", hError);
            laplacian->setVec3("thresholds", thresholds);
        } else {
            post.use();
            post.setInt("screenTexture", 0);
            post.setVec3("thresholds", thresholds);
        }

        glDrawArrays(GL_TRIANGLES, 0, 6);

        solMesh.bind();
        unsigned int currentElement = fTriangles->elementFor(cursorPos);
        if(currentElement >= 0) {
            whiteLines.use();
//          unsigned int indices[3];

//          indices[0] = fTriangles->indexOfNodeOfElement(currentElement, 0);
//          indices[1] = fTriangles->indexOfNodeOfElement(currentElement, 1);
//          indices[2] = fTriangles->indexOfNodeOfElement(currentElement, 2);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(currentElement*3*sizeof(unsigned int)));
        }

        ctx->bindVertexArray(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ctx->pollEvents();
        glfwSwapBuffers(ctx->window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}
