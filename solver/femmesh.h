#pragma once

#include <glm/glm.hpp>
#include <vector>

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
};

struct Node {
    glm::vec2 position;
    bool locked;
};

class FemMesh {
    public:
        FemMesh();
        FemMesh(std::vector<glm::vec2> nodePos, std::vector<unsigned int> elementIndices);
        FemMesh(std::vector<Node> nodes, std::vector<unsigned int> elementIndices);

        Node nodeOfElement(int elIndex, int localNodeIndex) const;
        void setupFE(std::vector<unsigned int> indices);
        std::vector<Node> nodes;
        std::vector<FiniteElement> elems;
    private:
};
