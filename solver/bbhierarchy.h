#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class FemMesh;

struct BoundingBox {
    // vec.x upper bound, vec.y lower bound
    glm::dvec2 upper;
    glm::dvec2 lower;

    BoundingBox(glm::dvec2 upper, glm::dvec2 lower, int elemInd,
            std::unique_ptr<BoundingBox> A,
            std::unique_ptr<BoundingBox> B);

    BoundingBox(glm::dvec2 upper, glm::dvec2 lower, int elemInd);

    // -1 for non-leaves
    int elemInd;

    std::unique_ptr<BoundingBox> childA;
    std::unique_ptr<BoundingBox> childB;

    bool insideBB(glm::dvec2 point) const;
    int elementFor(glm::dvec2 point, const FemMesh &mesh) const;
};

class BBHierarchy {
    public:
        BBHierarchy();

        void putall(const FemMesh &mesh);
        int elementFor(glm::dvec2 point, const FemMesh &mesh);
        std::vector<int> elementsFor(const std::vector<glm::dvec2> &points, const FemMesh &mesh);

        std::unique_ptr<BoundingBox> root;
};
