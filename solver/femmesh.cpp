#include "femmesh.h"
#include "bbhierarchy.h"
#include <cstdio>
#include <glm/ext/scalar_constants.hpp>

FemMesh::FemMesh() : activeNodes(), passiveNodes(), nodeIndexMap(), elems(), nodes() {
}

void FemMesh::setupFE(std::vector<unsigned int> elementIndices) {
    for(int i = 0; i < elementIndices.size(); i+=3) {
        elems.push_back({{elementIndices[i], elementIndices[i + 1], elementIndices[i + 2]}});
    }
}

FemMesh::FemMesh(std::shared_ptr<std::vector<Node>> nodes, std::vector<unsigned int> elementIndices) :
    activeNodes(), passiveNodes(), nodeIndexMap(), elemsOfNodes(nodes->size()), nodes(nodes)
{
    for(unsigned int i = 0; i < nodes->size(); i++) {
        switch(nodes->at(i).type) {
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
    }

    unsigned int elemNr = 0;
    for(int i = 0; i < elementIndices.size(); i+=3) {
        glm::uvec3 indices = {nodeIndexMap[elementIndices[i]], nodeIndexMap[elementIndices[i+1]], nodeIndexMap[elementIndices[i+2]]};
        elems.push_back({indices,
                {nodes->at(elementIndices[i]).type, nodes->at(elementIndices[i+1]).type, nodes->at(elementIndices[i+2]).type}});

        elemsOfNodes[elementIndices[i]].push_back(elemNr);
        elemsOfNodes[elementIndices[i+1]].push_back(elemNr);
        elemsOfNodes[elementIndices[i+2]].push_back(elemNr);
        elemNr++;
    }

    elemBVH = std::make_unique<BBHierarchy>();
    elemBVH->putall(*this);
    printf("activeNodes size %zu, passiveNodes size %zu, nodes size %zu\n", activeNodes.size(), passiveNodes.size(), this->nodes->size());
}

glm::dvec3 barycentricCoords(glm::dvec2 n0, glm::dvec2 n1, glm::dvec2 n2, glm::dvec2 point) {
    double a = n0.x - n2.x, b = n1.x - n2.x, c = n0.y - n2.y, d = n1.y - n2.y;

    double &x = point.x, &y = point.y;

    double detT = a*d - b*c;

    double l1 = (d*(x - n2.x) - b*(y - n2.y)) / detT;
    double l2 = (a*(y - n2.y) - c*(x - n2.x)) / detT;

    return {l1, l2, 1 - l1 - l2};
}

bool FemMesh::pointInElem(unsigned int elInd, glm::dvec2 point) const {
    auto n0 = nodeOfElement(elInd, 0).position;
    auto n1 = nodeOfElement(elInd, 1).position;
    auto n2 = nodeOfElement(elInd, 2).position;

    auto l = barycentricCoords(n0, n1, n2, point);

    if(l[0] <= -glm::epsilon<double>() || l[1] < -glm::epsilon<double>() || l[2] < -glm::epsilon<double>()) {
        return false;
    }
    return true;
}

Node FemMesh::nodeOfElement(int elIndex, int localNodeIndex) const {
    const FiniteElement &el = elems[elIndex];
    switch(el.ntype[localNodeIndex]) {
        case neumann:
        case active:
            return nodes->at(activeNodes[el.nodes[localNodeIndex]]);
            break;
        case dirichlet:
            return nodes->at(passiveNodes[el.nodes[localNodeIndex]]);
            break;
    }
    printf("Somehow reached end of nodeOfElement ;( \n");
}


glm::dvec2 FemMesh::barycenterOfElement(int elIndex) const {
    const FiniteElement &el = elems[elIndex];
    glm::dvec2 acc = {0.0, 0.0};

    for(int i = 0; i < 3; i++) {
        switch(el.ntype[i]) {
            case neumann:
            case active:
                acc += activeNodes[el.nodes[i]];
                break;
            case dirichlet:
                acc += passiveNodes[el.nodes[i]];
                break;
        }
    }

    return acc / 3.0;
}


double FemMesh::hasBoundary(unsigned int nodeId) const {
    auto elIds = elemsOfNodes[nodeId];
    for(auto elid : elIds) {
        const FiniteElement &elem = elems[elid];
        if(elem.ntype[0] == dirichlet || elem.ntype[1] == dirichlet || elem.ntype[2] == dirichlet) {
            return true;
        }
    }
    return false;
}

double FemMesh::evaluate(const std::vector<double> &solution, glm::dvec2 point) const {
    int elInd = elemBVH->elementFor(point, *this);

    if(elInd < 0) {
//      printf("nanned for %f, %f\n", point.x, point.y);
        return NAN;
    }


    const FiniteElement &elem = elems[elInd];
    double values[3];
    glm::dvec2 pos[3];

    for(int i = 0; i < 3; i++) {
        if(elem.ntype[i] == dirichlet) {
            values[i] = nodes->at(passiveNodes[elem.nodes[i]]).value;
            pos[i] = nodes->at(passiveNodes[elem.nodes[i]]).position;
        } else {
            values[i] = solution[elem.nodes[i]];
            pos[i] = nodes->at(activeNodes[elem.nodes[i]]).position;
        }
    }


    glm::dvec3 barPos = barycentricCoords( pos[0], pos[1], pos[2], point);
//  printf("nodes at (%f, %f) (%f, %f) (%f, %f) for (%f, %f) with bc: (%f, %f, %f)\n", 
//          pos[0].x, pos[0].y, pos[1].x, pos[1].y, pos[2].x, pos[2].y, point.x, point.y, barPos.x, barPos.y, barPos.z);
//  printf("with vals %f, %f, %f", values[0], values[1], values[2]);

    return barPos[0]*values[0] + barPos[1]*values[1] + barPos[2]*values[2];
}

unsigned int FemMesh::indexOfNodeOfElement(int elIndex, int localNodeIndex) const {
    const FiniteElement &el = elems[elIndex];
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
