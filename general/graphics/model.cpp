#include "model.h"

void Model::draw(Shader &shader) {

    for(int i = 0; i < meshes.size(); i++) {
        meshes[i].draw(shader);
    }
}

std::string Model::getDirectory() const {
    return directory;
}

Model::Model(std::vector<Mesh> meshes, std::string directory) {
    this->meshes = std::move(meshes);
    this->directory = directory;
}
