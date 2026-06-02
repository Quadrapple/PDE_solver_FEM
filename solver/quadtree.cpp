#include "quadtree.h"
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <random>


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

static auto compareXY(const std::vector<Node> &nodes) {
    return [&nodes] (const unsigned int &a, const unsigned int &b) {
        glm::dvec2 aval = nodes.at(a).position;
        glm::dvec2 bval = nodes.at(b).position;

        if(aval.x < bval.x) {
            return true;
        } else if(aval.x > bval.x) {
            return false;
        } else {
            return aval.y < bval.y;
        }
    };
}


void KDTree::putall(std::shared_ptr<std::vector<Node>> nodes) {
    this->nodes = nodes;
    root = std::make_unique<KDtreeQuad>(this);

    for(unsigned int i = 0; i < nodes->size(); i++) {
        root->array.push_back(i);
    }

    std::sort(root->array.begin(), root->array.end(), compareXY(*nodes) );
    root->putall(0, maxsize);
}
