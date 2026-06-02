#pragma once
#include "femmesh.h"
#include <memory>
#include <vector>

class KDTree;

struct KDtreeQuad {
    double split;
    bool full;
    int size;
    std::vector<unsigned int> array;
    const KDTree *parent;

    KDtreeQuad(const KDTree *parent);

    std::unique_ptr<KDtreeQuad> hi;
    std::unique_ptr<KDtreeQuad> lo;

    void putall(int depth, int maxlen);
    void print();
};

class KDTree {
    public:
        KDTree(const KDTree&) = delete;
        KDTree operator=(const KDTree&) = delete;

        KDTree() {
        }

        const int maxsize = 3;
        void putall(std::shared_ptr<std::vector<Node>> nodes);

        std::shared_ptr<std::vector<Node>> nodes;
        std::unique_ptr<KDtreeQuad> root;

        Node getSurroinding(glm::dvec2 position);
    private:
};
