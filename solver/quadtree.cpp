#include "quadtree.h"
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <random>

bool KDTree::compareX(const unsigned int &a, const unsigned int &b) const {
    return nodes->at(a).position.x < nodes->at(b).position.x;
}

bool KDTree::compareY(const unsigned int &a, const unsigned int &b) const {
    return nodes->at(a).position.y < nodes->at(b).position.y;
}

KDtreeQuad::KDtreeQuad(const KDTree *parent) : hi(nullptr), lo(nullptr), array(), parent(parent), full(false) {
}

void KDtreeQuad::print() {
    if(!full) {
        printf("(");
        for(auto n : array) {
            printf("%d, ", n);
        }
        printf(")");
        return;
    } else {
        hi->print();
        lo->print();
    }
}

void KDtreeQuad::putall(int depth, int maxlen) {
    size = array.size();
    if(this->array.size() <= maxlen) {
        return;
    }

    hi = std::make_unique<KDtreeQuad>(parent);
    lo = std::make_unique<KDtreeQuad>(parent);

    if(depth % 2 == 0) {
        split = parent->nodes->at(array[array.size() / 2]).position.x;
        for(auto nodeInd : this->array) {
            if(parent->nodes->at(nodeInd).position.x > split) {
                hi->array.push_back(nodeInd);
            } else if(parent->nodes->at(nodeInd).position.x < split) {
                lo->array.push_back(nodeInd);
            } else {
                //prevent one-point quadrants
                if(hi->array.size() < 2) {
                    hi->array.push_back(nodeInd);
                } else {
                    lo->array.push_back(nodeInd);
                }
            }
        }
    } else {
        split = parent->nodes->at(array[array.size() / 2]).position.y;
        for(auto nodeInd : this->array) {
            if(parent->nodes->at(nodeInd).position.y > split) {
                hi->array.push_back(nodeInd);
            } else if(parent->nodes->at(nodeInd).position.y < split) {
                lo->array.push_back(nodeInd);

            } else {
                //prevent one-point quadrants
                if(hi->array.size() < 2) {
                    hi->array.push_back(nodeInd);
                } else {
                    lo->array.push_back(nodeInd);
                }
            }
        }
    }

    this->array.clear();
    this->full = true;

    hi->putall(depth + 1, maxlen);
    lo->putall(depth + 1, maxlen);
}

void KDTree::putall(std::shared_ptr<std::vector<Node>> nodes) {
    this->nodes = nodes;
    root = std::make_unique<KDtreeQuad>(this);

    for(unsigned int i = 0; i < nodes->size(); i++) {
        root->array.push_back(i);
    }

    std::sort(root->array.begin(), root->array.end(),
            [this] (const unsigned int &a, const unsigned int &b) { return this->compareY(a,b); } );
    std::stable_sort(root->array.begin(), root->array.end(),
            [this] (const unsigned int &a, const unsigned int &b) { return this->compareX(a,b); } );
    root->putall(0, maxsize);
//  root->print();
}
