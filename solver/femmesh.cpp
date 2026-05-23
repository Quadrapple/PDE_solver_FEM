#include "femmesh.h"
#include <cstdio>

FemMesh::FemMesh(std::vector<glm::vec2> nodePos, std::vector<unsigned int> elementIndices) {
    for(int i = 0; i < nodePos.size(); i++) {
        nodes.push_back({nodePos[i], false});
    }
    for(int i = 0; i < elementIndices.size(); i+=3) {
        elems.push_back({{elementIndices[i], elementIndices[i + 1], elementIndices[i + 2]}});
    }
}

FemMesh::FemMesh() : nodes(), elems() {
}

void FemMesh::setupFE(std::vector<unsigned int> elementIndices) {
    for(int i = 0; i < elementIndices.size(); i+=3) {
        elems.push_back({{elementIndices[i], elementIndices[i + 1], elementIndices[i + 2]}});
    }
}

FemMesh::FemMesh(std::vector<Node> nodes, std::vector<unsigned int> elementIndices) {
    nodes = std::move(nodes);
    printf("ccreated\n");
    for(int i = 0; i < elementIndices.size(); i+=3) {
        elems.push_back({{elementIndices[i], elementIndices[i + 1], elementIndices[i + 2]}});
    }
}

Node FemMesh::nodeOfElement(int elIndex, int localNodeIndex) const {
    return nodes[elems[elIndex].nodes[localNodeIndex]];
}


