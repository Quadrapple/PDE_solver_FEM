#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "buffer.h"
#include "general/event_handler.h"
#include "femmesh.h"
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

float vertices[] = {
    -0.5f, -0.5f, 1.0, 0.0, 0.0,
     0.5f, -0.5f, 0.0, 1.0, 0.0,
     0.0f,  0.5f, 0.0, 0.0, 1.0
};  

static unsigned int indices[] = {  // note that we start from 0!
    0, 1, 2,   // first triangle
}; 

VertexArray demoTriangle() {
    VertexArray triangle;
    triangle.bind();
    triangle.setVertices(std::make_unique<Buffer>(GL_ARRAY_BUFFER, sizeof(vertices), vertices));
    triangle.setIndices(std::make_unique<Buffer>(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices));

    triangle.addAttrib(VertexAttrib{2, sizeof(float) * 5, GL_FLOAT, (void*)0});
    triangle.addAttrib(VertexAttrib{3, sizeof(float) * 5, GL_FLOAT, (void*)(2*sizeof(float))});
    triangle.enableAttrib(0);
    return triangle;
}

struct mVertex {
    glm::vec2 pos;
    glm::vec3 color;
};

std::unique_ptr<FemMesh> demoTriangleMesh(int size, float (*u)(float, float)) {
    std::vector<Node> nodes;
    std::vector<unsigned int> mIndices;

    const float range = 0.8f; 

    float delta = range / (float)size;

    float eps = (1 + (float)size/15)*glm::epsilon<float>();

    //left boundary
    for(float y = -range; y <= range + eps; y += delta) {
        Node v = {{-range, y}, dirichlet, u(-range, y)};
        nodes.push_back(v);
    }


    for(float x = -range + delta; x <= range - delta + eps; x += delta) {
        //top boundary
        Node v = {{x, -range}, dirichlet, u(x, -range)};
        nodes.push_back(v);

        for(float y = -range + delta; y <= range - delta + eps; y += delta) {
            Node v = {{x, y}, active};
            nodes.push_back(v);
        }

        //bottom boundary
        v = {{x, range}, dirichlet, u(x, range)};
        nodes.push_back(v);
    }

    //right boundary
    for(float y = -range; y <= range + eps; y += delta) {
        Node v = {{range, y}, dirichlet, u(range, y)};
        nodes.push_back(v);
    }

    int doublesize = 2*size + 1;
    printf("Nodesize  = %zu, dsize = %d, dsize^2 = %d\n", nodes.size(), doublesize, doublesize*doublesize -1 );
    for(int i = 0; i < doublesize - 1; i++) {
        for(int j = 0; j < doublesize - 1; j++) {
            // 90 deg top left
            mIndices.push_back(doublesize * i + j);
            mIndices.push_back(doublesize * (i + 1) + j);
            mIndices.push_back(doublesize * i + (j + 1));

            //90 deg bottom right
            mIndices.push_back(doublesize * i + (j + 1));
            mIndices.push_back(doublesize * (i + 1) + j);
            mIndices.push_back(doublesize * (i + 1) + (j + 1));
        }
    }
    auto res = std::make_unique<FemMesh>(nodes, mIndices);

    printf("made mesh\n");
    return std::move(res);
}

VertexArray demoTriangleMeshGr(const std::unique_ptr<FemMesh> &femmesh, int size, std::vector<float> colors) {
    std::vector<mVertex> mVertices;
    std::vector<unsigned int> mIndices;

    const float range = 0.8f; 

//  glm::vec3 colors[] = {{1.0, 0.1, 0.1}, {0.1, 1.0, 0.1}, {0.1, 0.1, 1.0}, 
//                          {0.5, 0.0, 0.0}, {0.0, 0.5, 0.0}, {0.0, 0.0, 0.5}};
    int doublesize = 2*size + 1;

    float max = 0;
    for(auto node: femmesh->nodes) {
        if(glm::abs(node.value) > max) {
            max = glm::abs(node.value);
        }
    }

    for(int i = 0; i < doublesize; i++) {
        for(int j = 0; j < doublesize; j++) {
            float colorVal = glm::atan(femmesh->nodes[doublesize * i + j].value / max);
            float greenColorVal =(
                    (glm::abs(max) - glm::abs(femmesh->nodes[doublesize * i + j].value)*4) / glm::abs(max));

            mVertex v = {femmesh->nodes[doublesize * i + j].position, {colorVal, greenColorVal, -colorVal}};
            mVertices.push_back(v);
        }
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

float f(float x, float y) {
    return x*40;
}

float u(float x, float y) {
    return -x;
}

int main(int argc, char* argv[]) {

    ctx = new EventHandler("Test", glm::vec2(800, 600));
    ctx->disableMouse();
    ctx->disable(GL_DEPTH_TEST);

    const int size = 100;
    auto fTriangles = demoTriangleMesh(size, u);
    Solver solver;

    printf("Solving...\n");
    auto colors = solver.solve(*fTriangles, f);

    printf("Solved! colors.size %zu\n", colors.size());

    for(int i = 0; i < fTriangles->activeNodes.size(); i++) {
        fTriangles->nodes[fTriangles->activeNodes[i]].value = colors[i];
    }
    printf("assigned colors\n");

    VertexArray triangle = demoTriangleMeshGr(fTriangles, size, colors);

    printf("created triangle\n");
    glfwSwapInterval(1);
    
    Shader color("general/graphics/glsl/color.vert", "general/graphics/glsl/color.frag");
    Shader point("general/graphics/glsl/color.vert", "general/graphics/glsl/point.frag");
    Shader whiteLines("general/graphics/glsl/color.vert", "general/graphics/glsl/white.frag");
    color.use();

    glPointSize(10.0f);
    while(!glfwWindowShouldClose(ctx->window)) {
        ctx->pollEvents();

        triangle.bind();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        color.use();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, triangle.indexCount, GL_UNSIGNED_INT, 0);

//      whiteLines.use();
//      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//      glDrawElements(GL_TRIANGLES, triangle.indexCount, GL_UNSIGNED_INT, 0);

        //Draw points
//      point.use();
//      glDrawElements(GL_POINTS, triangle.indexCount, GL_UNSIGNED_INT, 0);

        ctx->bindVertexArray(0);
        glfwSwapBuffers(ctx->window);
    }
}
