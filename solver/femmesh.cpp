#include "femmesh.h"
#include <cstdio>

FemMesh::FemMesh() : activeNodes(), passiveNodes(), elems(), nodes() {
}

void FemMesh::setupFE(std::vector<unsigned int> elementIndices) {
    for(int i = 0; i < elementIndices.size(); i+=3) {
        elems.push_back({{elementIndices[i], elementIndices[i + 1], elementIndices[i + 2]}});
    }
}

FemMesh::FemMesh(std::vector<Node> nodes, std::vector<unsigned int> elementIndices) :
    activeNodes(), passiveNodes(), nodeIndexMap() 
{
    for(unsigned int i = 0; i < nodes.size(); i++) {
        switch(nodes[i].type) {
            case neumann:
            case active:
                nodeIndexMap.push_back(activeNodes.size());
                activeNodes.push_back(i);
                break;
            case dirichlet:
                nodeIndexMap.push_back(passiveNodes.size());
                passiveNodes.push_back(i);
                break;
        }
        this->nodes.push_back(nodes[i]);
    }

    for(int i = 0; i < elementIndices.size(); i+=3) {
        glm::uvec3 indices = {nodeIndexMap[elementIndices[i]], nodeIndexMap[elementIndices[i+1]], nodeIndexMap[elementIndices[i+2]]};
        elems.push_back({indices,
                {nodes[elementIndices[i]].type, nodes[elementIndices[i+1]].type, nodes[elementIndices[i+2]].type}});
    }

    printf("activeNodes size %zu, passiveNodes size %zu, nodes size %zu\n", activeNodes.size(), passiveNodes.size(), this->nodes.size());
}

Node FemMesh::nodeOfElement(int elIndex, int localNodeIndex) const {
    FiniteElement el = elems[elIndex];
    switch(el.ntype[localNodeIndex]) {
        case neumann:
        case active:
            return nodes[activeNodes[el.nodes[localNodeIndex]]];
            break;
        case dirichlet:
            return nodes[passiveNodes[el.nodes[localNodeIndex]]];
            break;
    }
    printf("Somehow reached end of nodeOfElement ;( \n");
}

unsigned int FemMesh::indexOfNodeOfElement(int elIndex, int localNodeIndex) const {
    FiniteElement el = elems[elIndex];
    switch(el.ntype[localNodeIndex]) {
        case neumann:
        case active:
            return activeNodes[el.nodes[localNodeIndex]];
            break;
        case dirichlet:
            return passiveNodes[el.nodes[localNodeIndex]];
            break;
    }
    printf("Somehow reached end of nodeOfElement ;( \n");
}
