#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <memory>
#include <string>
#include <vector>

#include "mesh.h"

class Model {
    public:
        Model(std::string filename);
        Model(std::vector<Mesh> meshes, std::string directory);
        void draw(Shader &shader);

        void drawInstanced(Shader &shader, std::vector<glm::mat4> modelMatrices);

        std::string getDirectory() const;

    private:
        std::string directory;
        std::vector<Mesh> meshes;
};

#endif
