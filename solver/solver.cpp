#include "solver.h"
#include <glm/common.hpp>

float triangle_area(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2) {
    return ((p0.x - p1.x)*(p0.y - p2.y) - (p0.x - p2.x)*(p0.y - p1.y)) / 2;
}

//local numbering
float K_ij(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, int i, int j) {
    glm::vec2 grads[] = {{p1.y - p2.y, p2.x - p1.x},
                         {p2.y - p0.y, p0.x - p2.x},
                         {p0.y - p1.y, p1.x - p0.x}};
    return glm::dot(grads[i], grads[j]) / (4 * triangle_area(p0,p1,p2));
}

//very basic integral approximation
float F_i(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float (*function)(float, float)) {
    const glm::vec2 barycenter = (p1 + p2 + p3) / 3.0f;
    return (function(barycenter.x, barycenter.y) / 3) * triangle_area(p1, p2, p3);
}

std::pair<std::vector<float>, std::vector<float>> Solver::assemble(const FemMesh &mesh, float (*function)(float, float)) {
    const int sideLen = mesh.nodes.size();

    auto global_K = std::vector<float>(sideLen * sideLen, 0.0);
    auto global_F = std::vector<float>(sideLen, 0.0);

    for(int el_ind = 0; el_ind < mesh.elems.size(); el_ind++) {
        FiniteElement el = mesh.elems[el_ind];

        Node n0 = mesh.nodeOfElement(el_ind, 0);
        Node n1 = mesh.nodeOfElement(el_ind, 1);
        Node n2 = mesh.nodeOfElement(el_ind, 2);

        for(int i = 0; i < 3; i++) {
            global_F[el.nodes[i]] += F_i(n0.position, n1.position, n2.position, function);
            for(int j = 0; j < 3; j++) {
                global_K[sideLen*el.nodes[i] + el.nodes[j]] += K_ij(n0.position, n1.position, n2.position, i, j); 
            }
        }
    }

    //restrict with BC
    for(int i = 0; i < sideLen; i++) {
        if(mesh.nodes[i].locked) {
            for(int j = 0; j < sideLen; j++) {
                global_K[sideLen*j + i] = 0;
                global_K[sideLen*i + j] = 0;
            }
            global_K[sideLen*i + i] = 1;
            global_F[i] = 0.0;
        }
    }

    return std::make_pair(global_K, global_F);
}

//for square matrices
std::vector<float> gaussianElimination(std::vector<float> &M, std::vector<float> &F, int sideLen) {
//  std::vector<int> pivot(sideLen);

//  for(int i = 0; i < sideLen; i++) {
//      pivot[i] = i;
//  }

    for(int i = 0; i < sideLen; i++) {
        int maxInd = i;
        float max = M[sideLen*i + i];

        //find max for elimination
        for(int j = i + 1; j < sideLen; j++) {
            if(glm::abs(M[sideLen*j + i]) > glm::abs(max)) {
                max = M[sideLen*j + i];
                maxInd = j;
            }
        }

        //set pivot and swap the solution vector
//      int itmp = pivot[maxInd];
//      pivot[i] = itmp;
//      pivot[maxInd] = pivot[i];

        float ftmp = F[maxInd];
        F[maxInd] = F[i];
        F[i] = ftmp;
        
        //swap the rows
        for(int j = i; j < sideLen; j++) {
            ftmp = M[sideLen * i + j];
            M[sideLen * i + j] = M[sideLen*maxInd + j];
            M[sideLen*maxInd + j] = ftmp;
        }

        //gussian elimination
        for(int j = i + 1; j < sideLen; j++) {
            float delta = -M[sideLen*j + i] / M[sideLen*i + i];
            M[sideLen*j + i] = 0;
            F[j] += F[i] * delta;

            for(int k = i + 1; k < sideLen; k++) {
                M[sideLen*j + k] += delta * M[sideLen*i + k];
            }
        }
    }

    //solve the triangular matrix
    std::vector<float> sol(sideLen);

    sol[sideLen - 1] = F[sideLen - 1] / M[sideLen*sideLen - 1]; 
    for(int i = sideLen - 2; i >= 0; i--) {
        sol[i] = F[i];
        for(int j = sideLen - 1; j > i; j--) {
            sol[i] -= sol[j] * M[sideLen*i + j];
        }
        sol[i] /= M[sideLen*i + i];
    }

    return sol;
}

std::vector<float> Solver::solve(const FemMesh &m, float (*function)(float, float)) {
    auto K_F = assemble(m, function);
    std::vector<float> K = K_F.first;
    std::vector<float> F = K_F.second;
    const int sideLen = F.size();

    return gaussianElimination(K, F, sideLen);
}
