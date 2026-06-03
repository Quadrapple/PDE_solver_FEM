#pragma once

#include "bbhierarchy.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class BBHierarchy;

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
        FemMesh(std::shared_ptr<std::vector<Node>> nodes, const std::vector<unsigned int> &elementIndices);

        Node nodeOfElement(int elIndex, int localNodeIndex) const;
        glm::dvec2 barycenterOfElement(int elIndex) const;


        unsigned int indexOfNodeOfElement(int elIndex, int localNodeIndex) const;
        void remesh(const std::vector<unsigned int> &indices);

        bool pointInElem(unsigned int elInd, glm::dvec2 point) const;

        std::vector<double> evaluate(const std::vector<double> &solution, const std::vector<glm::dvec2> &points) const;
        double evaluate(const std::vector<double> &solution, glm::dvec2 point) const;
        double hasBoundary(unsigned int nodeId) const;

        std::shared_ptr<std::vector<Node>> nodes;
        std::vector<unsigned int> activeNodes;
        std::vector<unsigned int> passiveNodes;

        // nodeindex -> index in passiveNodes or activeNodes
        std::vector<unsigned int> nodeIndexMap;

        std::vector<FiniteElement> elems;
        std::vector<std::vector<unsigned int>> elemsOfNodes;

    private:
        std::unique_ptr<BBHierarchy> elemBVH;
};
