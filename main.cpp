#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <list>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "buffer.h"
#include "general/event_handler.h"
#include "femmesh.h"
#include "quadedge.h"
#include "solver.h"
#include "shader.h"
#include "vao.h"

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

struct Adjacency {
    std::vector<std::pair<unsigned int, std::list<unsigned int>>> arr; // adjacencies

    Adjacency(int num) : arr(num) {
        for(int i = 0; i < num; i++) {
            arr.emplace_back();
        }
    }

    void connect(unsigned int a_local, unsigned int b_local) {
        arr[a_local].second.push_back(b_local);
        arr[b_local].second.push_back(a_local);
    }
};

std::shared_ptr<std::vector<Node>> demoTriangleMesh(int size, double range, float (*u)(float, float)) {
    auto nodes = std::make_shared<std::vector<Node>>();
    
    double delta = range / (float)size;

    std::vector<std::list<unsigned int>> adj; // adjacencies

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

    //INTERNAL=================================================
    for(int x = -size + 1; x <= size - 1; x++) {
        for(int y = -size + 1; y <= size - 1; y++) {
            Node v = {{x*delta , y*delta}, active, 0.0f};
            v.position += glm::vec2{ ((double)rand() / RAND_MAX - 0.5) * (delta/5), (double)rand() / RAND_MAX * (delta/5) };
            nodes->push_back(v);
        }
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
        mVertices.push_back({n.position, {1.0, 1.0, 1.0}});
    }

    VertexArray triangleMesh;
    triangleMesh.bind();
    triangleMesh.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, mVertices.size() * sizeof(mVertex), mVertices.data()));
    triangleMesh.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), mIndices.data()));

    triangleMesh.indexCount = mIndices.size();
    triangleMesh.vertexCount = mVertices.size();
    triangleMesh.addAttrib(VertexAttrib{2, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, pos)});
    triangleMesh.addAttrib(VertexAttrib{3, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, color)});
    return triangleMesh;
}

/*
VertexArray visEdges(const QuadEdge &q) {
    std::vector<mVertex> mVertices;

//  glm::vec3 colors[] = {{1.0, 0.1, 0.1}, {0.1, 1.0, 0.1}, {0.1, 0.1, 1.0}, 
//                          {0.5, 0.0, 0.0}, {0.0, 0.5, 0.0}, {0.0, 0.0, 0.5}};

    for(auto edgerecord : qu->edgeRecords) {
        if(edgerecord == nullptr || edgerecord->edges[0].orig() <= -1) {
            continue;
        }
        glm::vec2 org = qu->origOf(&edgerecord->edges[0]).position / 11.0;
        glm::vec2 dst = qu->destOf(&edgerecord->edges[0]).position / 11.0;

        mVertices.push_back({org, {1.0, 1.0, 1.0}});
        mVertices.push_back({dst, {1.0, 1.0, 1.0}});
    }

    VertexArray triangleMesh;
    triangleMesh.bind();
    triangleMesh.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, mVertices.size() * sizeof(mVertex), mVertices.data()));
    triangleMesh.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), mIndices.data()));

    triangleMesh.indexCount = mIndices.size();
    triangleMesh.vertexCount = mVertices.size();
    triangleMesh.addAttrib(VertexAttrib{2, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, pos)});
    triangleMesh.addAttrib(VertexAttrib{3, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, color)});
    return triangleMesh;
}
*/
VertexArray errorGridGr(std::vector<mVertex> &grid, int size) {
    std::vector<unsigned int> mIndices;

//  float max = 0;
//  for(const auto &v: grid) {
//      if(glm::abs(v.color.x) > max) {
//          max = glm::abs(v.color.x);
//      }
//  }
//  printf("maximal abs value is %f\n", max);

    glm::vec3 lowColor = {0.05, 0.05, 0.05};
    glm::vec3 midColor = {0.2, 0.2, 0.5};
    glm::vec3 highColor = {1.0, 0.3, 0.0};

    int activeNr = 0;
    for(auto &v: grid) {
        float colorVal = 0.0;
        colorVal = glm::atan(v.color.x);
        glm::vec3 color = glm::pow(interpolate3(colorVal, lowColor, midColor, highColor), glm::vec3(1.25f));
        v.color = color;
    }

    int gridsize = size*2 + 1;
    for(unsigned int i = 0; i < gridsize - 1; i++) {
        for(unsigned int j = 0; j < gridsize - 1; j++) {
            mIndices.push_back(i*gridsize + j);
            mIndices.push_back((i+1)*gridsize + j);
            mIndices.push_back(i*gridsize + j + 1);

            mIndices.push_back((i+1)*gridsize + j + 1);
            mIndices.push_back(i*gridsize + j + 1);
            mIndices.push_back((i+1)*gridsize + j);
        }
    }

    VertexArray triangleMesh;
    triangleMesh.bind();
    triangleMesh.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, grid.size() * sizeof(mVertex), grid.data()));
    triangleMesh.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), mIndices.data()));

    triangleMesh.indexCount = mIndices.size();
    printf("%zu indices\n", mIndices.size());
    triangleMesh.addAttrib(VertexAttrib{2, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, pos)});
    triangleMesh.addAttrib(VertexAttrib{3, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, color)});
    return triangleMesh;
}

VertexArray demoTriangleMeshGr(const std::unique_ptr<FemMesh> &femmesh, int size, const std::vector<double> &solution) {
    std::vector<mVertex> mVertices;
    std::vector<unsigned int> mIndices;

    float max = 0;
    for(double val: solution) {
        if(glm::abs(val) > max) {
            max = glm::abs(val);
        }
    }
    printf("maximal abs value is %f\n", max);

    glm::vec3 lowColor = {0.05, 0.05, 0.05};
    glm::vec3 midColor = {0.2, 0.2, 0.5};
    glm::vec3 highColor = {1.0, 0.3, 0.0};

    const std::vector<Node> &nodes = *femmesh->nodes;

    int activeNr = 0;
    for(int i = 0; i < nodes.size(); i++) {
        float colorVal = 0.0;
        if(nodes[i].type != dirichlet) {
            colorVal = glm::atan(solution[activeNr] / max);
            activeNr++;
        } else {
            colorVal = glm::atan(nodes[i].value / max);
        }
        glm::vec3 color = glm::pow(interpolate3(colorVal, lowColor, midColor, highColor), glm::vec3(1.25f));
        mVertex v = {nodes[i].position, color};
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
    triangleMesh.addAttrib(VertexAttrib{3, sizeof(mVertex), GL_FLOAT, (void*)offsetof(mVertex, color)});
    return triangleMesh;
}

double f(double x, double y) {
    return x > 0 ? 10*x : 0;
}

float u(float x, float y) {
    return x;
}
static auto compareXY(const std::vector<Node> &nodes) {
    return [&nodes] (const unsigned int &a, const unsigned int &b) {
        glm::dvec2 aval = nodes.at(a).position;
        glm::dvec2 bval = nodes.at(b).position;

        if(aval.x < bval.x) {
            return true;
        } else if(aval.x > bval.x) {
            return false;
        } else {
            return aval.y < bval.y;
        }
    };
}

int main(int argc, char* argv[]) {

    ctx = new EventHandler("Test", glm::vec2(1200, 900));
    ctx->disableMouse();
    ctx->disable(GL_DEPTH_TEST);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
//  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(ctx->window, true);
    ImGui_ImplOpenGL3_Init();

    const double range = 1.0;
    int elementSubdivisionSize = 14;
    int prElementSubdivisionSize = elementSubdivisionSize;

    auto nodes = demoTriangleMesh(elementSubdivisionSize, range, u);
    auto qedge = std::make_unique<QuadEdge>(nodes);

    std::vector<unsigned int> ind;
    ind.reserve(nodes->size());
    for(int i = 0; i < nodes->size(); i++) {
        ind.push_back(i);
    }

    std::sort(ind.begin(), ind.end(), compareXY(*nodes));
    qedge->delaunay(ind.data(), ind.size());

    /*
    printf("Node tree...\n");
    //build node tree
    auto qtree = std::make_unique<KDTree>();
    qtree->putall(nodes);
    printf("Nodes treed!\n");
    */

    //create elements from triangulation
    auto els = genElements(*qedge);
    printf("Elements genned!\n");

    //create mesh from nodes and elements
    auto fTriangles = std::make_unique<FemMesh>(nodes, els);
    printf("Mesh init!\n");
    Solver solver;

    printf("Solving...\n");
    auto solution = solver.solve(*fTriangles, f);
    printf("Solved! colors.size %zu\n", solution.size());

    int errGridSize = 50;
    int prErrGridSize = errGridSize;

    bool renderError = false;
    bool prRenderError = renderError;

    float hError = sqrt(range/elementSubdivisionSize);
    float prHError = hError;

    printf("Estimating errors...\n");
    auto errors = solver.estimateError(*fTriangles, solution, errGridSize, range, f, hError);
    printf("Estimated!\n");

    VertexArray solMesh = demoTriangleMeshGr(fTriangles, elementSubdivisionSize, solution);
    VertexArray errMesh = errorGridGr(errors, errGridSize);

    Shader color("general/graphics/glsl/color.vert", "general/graphics/glsl/color.frag");
    Shader point("general/graphics/glsl/color.vert", "general/graphics/glsl/point.frag");
    Shader whiteLines("general/graphics/glsl/color.vert", "general/graphics/glsl/white.frag");
    color.use();

    glfwSwapInterval(1);
    glPointSize(10.0f);
    while(!glfwWindowShouldClose(ctx->window)) {
        ctx->pollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        {
            ImGui::Begin("Model loader");
            ImGui::SliderInt("solution resolution", &elementSubdivisionSize, 1, 14);
            ImGui::SliderInt("errorGrid resolution", &errGridSize, 1, 20);
            ImGui::SliderFloat("error diff step", &hError, 0.0f, 0.5f);
            ImGui::Checkbox("show error", &renderError);
            ImGui::End();
        }

        unsigned int indexcount = 0;

        if(hError != prHError || prErrGridSize != errGridSize || prElementSubdivisionSize != elementSubdivisionSize) {
            errors = solver.estimateError(*fTriangles, solution, errGridSize, range, f, hError);
            errMesh = errorGridGr(errors, errGridSize);

            prErrGridSize = errGridSize;
            prHError = hError;
        }

        if(prElementSubdivisionSize != elementSubdivisionSize) {
            *nodes = *demoTriangleMesh(elementSubdivisionSize, range, u);

            qedge = std::make_unique<QuadEdge>(nodes);

            std::vector<unsigned int> ind;
            ind.reserve(nodes->size());
            for(int i = 0; i < nodes->size(); i++) {
                ind.push_back(i);
            }

            std::sort(ind.begin(), ind.end(), compareXY(*nodes));
            qedge->delaunay(ind.data(), ind.size());
            els = genElements(*qedge);

            fTriangles->remesh(els);

            solution = solver.solve(*fTriangles, f);
            solMesh = demoTriangleMeshGr(fTriangles, elementSubdivisionSize, solution);

            prElementSubdivisionSize = elementSubdivisionSize;
        }

        if(renderError) {
            errMesh.bind();
            indexcount = errMesh.indexCount;
        } else {
            solMesh.bind();
            indexcount = solMesh.indexCount;
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        color.use();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, indexcount, GL_UNSIGNED_INT, 0);

//      whiteLines.use();
//      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//      glDrawArrays(GL_LINES, 0, triangle.vertexCount);
//      glDrawElements(GL_TRIANGLES, triangle.indexCount, GL_UNSIGNED_INT, 0);

        //Draw points
//      point.use();
//      glDrawArrays(GL_POINTS, 0, triangle.vertexCount);
//      glDrawElements(GL_POINTS, triangle.indexCount, GL_UNSIGNED_INT, 0);

        ctx->bindVertexArray(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ctx->window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}
