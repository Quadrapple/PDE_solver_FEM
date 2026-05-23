#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

#include "shader.h"
#include "vao.h"
#include "texture.h"

class Mesh {
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<std::shared_ptr<Texture>> textures);
        void draw(Shader &shader);
        void drawInstanced(Shader &shader, std::vector<glm::mat4> modelMatrices);

    private:
        VertexArray vao;

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<std::shared_ptr<Texture>> textures;

        void setupMesh();

};

#endif
