#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

enum NodeType {
    active, neumann, dirichlet
};

struct Node {
    glm::dvec2 position;
    NodeType type;
    double value;
};

// ^
// |
// | y
//
//[2]
// | \
// |    \
// |       \
//[0]_______[1] ---> x
//
//Basis functions:
// 1 <- x;
// 2 <- y;
// 0 <- 1 - x - y;
struct FiniteElement {
    glm::uvec3 nodes;
    NodeType ntype[3];
};

class FemMesh {
    public:
        FemMesh();
        FemMesh(std::shared_ptr<std::vector<Node>> nodes, std::vector<unsigned int> elementIndices);

        Node nodeOfElement(int elIndex, int localNodeIndex) const;
        unsigned int indexOfNodeOfElement(int elIndex, int localNodeIndex) const;
        void setupFE(std::vector<unsigned int> indices);
        std::vector<unsigned int> activeNodes;
        std::vector<unsigned int> passiveNodes;
        std::shared_ptr<std::vector<Node>> nodes;

        std::vector<unsigned int> nodeIndexMap;
        std::vector<FiniteElement> elems;
    private:
};
