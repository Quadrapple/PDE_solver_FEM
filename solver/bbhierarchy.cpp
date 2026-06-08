#include "bbhierarchy.h"
#include "femmesh.h"
#include "comparators.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <vector>

double minof3(double a, double b, double c) {
    double min = a;
    if(b < min) {
         min = b;
    }
    if(c < min) {
         min = c;
    }
    return min;
}

double maxof3(double a, double b, double c) {
    double max = a;
    if(b > max) {
         max = b;
    }
    if(c > max) {
         max = c;
    }
    return max;
}

//offset to make sure all doubles are positive
auto ZorderedCmp(const FemMesh &mesh, double offset = 1.0) {
    return [&mesh, offset] (const unsigned int &a, const unsigned int &b) {
        return compareZorder(mesh.barycenterOfElement(a) + offset, mesh.barycenterOfElement(b) + offset);
    };
}

//first - upper, second - lower
std::pair<glm::dvec2, glm::dvec2> getBB(glm::dvec2 a, glm::dvec2 b, glm::dvec2 c) {
    return {{maxof3(a.x, b.x, c.x), maxof3(a.y, b.y, c.y)},
            {minof3(a.x, b.x, c.x), minof3(a.y, b.y, c.y)}};
}

BoundingBox::BoundingBox(glm::dvec2 upper, glm::dvec2 lower, int elemInd,
            std::unique_ptr<BoundingBox> A,
            std::unique_ptr<BoundingBox> B) :
upper(upper), lower(lower), elemInd(elemInd), childA(std::move(A)), childB(std::move(B)) {
}

BoundingBox::BoundingBox(glm::dvec2 upper, glm::dvec2 lower, int elemInd) :
    upper(upper), lower(lower), elemInd(elemInd) {
}

std::unique_ptr<BoundingBox> mergeBB(std::unique_ptr<BoundingBox> b1, std::unique_ptr<BoundingBox> b2) {
    glm::dvec2 upper = {glm::max(b1->upper.x, b2->upper.x), glm::max(b1->upper.y, b2->upper.y)};
    glm::dvec2 lower = {glm::min(b1->lower.x, b2->lower.x), glm::min(b1->lower.y, b2->lower.y)};

    return std::make_unique<BoundingBox>(upper, lower, -1, std::move(b1), std::move(b2));
}

BBHierarchy::BBHierarchy() : root(nullptr) {
}

void BBHierarchy::putall(const FemMesh &mesh) {
    std::vector<unsigned int> elIndices;
    std::deque<std::unique_ptr<BoundingBox>> bbs;
    elIndices.reserve(mesh.elems.size());

    for(int i = 0; i < mesh.elems.size(); i++) {
        elIndices.push_back(i);
    }
    std::sort(elIndices.begin(), elIndices.end(), ZorderedCmp(mesh));

    for(auto elInd : elIndices) {
        glm::dvec2 n0 = mesh.nodeOfElement(elInd, 0).position;
        glm::dvec2 n1 = mesh.nodeOfElement(elInd, 1).position;
        glm::dvec2 n2 = mesh.nodeOfElement(elInd, 2).position;

        auto res = getBB(n0, n1, n2);
        bbs.push_back(std::make_unique<BoundingBox>(res.first, res.second, elInd));
    }

    while(bbs.size() > 1) {
        auto b1 = std::move(bbs.front());
        bbs.pop_front();
        auto b2 = std::move(bbs.front());
        bbs.pop_front();
        bbs.push_back(mergeBB(std::move(b1), std::move(b2)));
    }

    root = std::move(bbs.front());
}

int BoundingBox::elementFor(glm::dvec2 point, const FemMesh &mesh) const {
    if(insideBB(point)) {
        if(elemInd < 0) {
            return glm::max(childA->elementFor(point, mesh),
                    childB->elementFor(point, mesh));

        } else if(mesh.pointInElem(elemInd, point)) {
            return elemInd;
        }
    } 
    return -1;
}

bool BoundingBox::insideBB(glm::dvec2 point) const{
    if(point.x <= upper.x && point.x >= lower.x &&
            point.y <= upper.y && point.y >= lower.y) {
        return true;
    }
    return false;
}

int BBHierarchy::elementFor(glm::dvec2 point, const FemMesh &mesh) {
    if(root->insideBB(point)) {
        return root->elementFor(point, mesh);
    } else {
        return -1;
    }
}

std::vector<int> BBHierarchy::elementsFor(const std::vector<glm::dvec2> &points, const FemMesh &mesh) {
    std::vector<int> result(points.size(), -1);
    std::deque< std::pair<const BoundingBox*, std::vector<unsigned int>> > searchfront;

    std::vector<unsigned int> pInds;
    //assume root to have children

    for(int i = 0; i < points.size(); i++) {
        const glm::dvec2 &point = points[i];
        if(root->insideBB(point)) {
            pInds.push_back(i);
        }
    }

    searchfront.push_back(std::make_pair(root.get(), std::move(pInds)));

    while(!searchfront.empty()) {
        const auto &pointInds = searchfront.front().second;
        const auto bb = searchfront.front().first;

        if(bb->elemInd >= 0) {
            for(const auto &pInd : pointInds) {
                const glm::dvec2 &point = points[pInd];
                if(mesh.pointInElem(bb->elemInd, point)) {
                    result[pInd] = bb->elemInd;
                }
            }

        } else {
            std::vector<unsigned int> pIndsA, pIndsB;

            for(const auto &pInd : pointInds) {
                const glm::dvec2 &point = points[pInd];
                if(bb->childA->insideBB(point)) {
                    pIndsA.push_back(pInd);
                }
                if(bb->childB->insideBB(point)) {
                    pIndsB.push_back(pInd);
                }
            }

            if(!pIndsA.empty()) {
                searchfront.push_back(std::make_pair(bb->childA.get(), std::move(pIndsA)));
            }
            if(!pIndsB.empty()) {
                searchfront.push_back(std::make_pair(bb->childB.get(), std::move(pIndsB)));
            }
        }
        searchfront.pop_front();
    }
    return result;
}
