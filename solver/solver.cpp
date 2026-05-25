#include "solver.h"
#include "femmesh.h"
#include <algorithm>
#include <cstdio>
#include <glm/common.hpp>
#include <iterator>

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

SquareMatrix::SquareMatrix() : rows(){
}

SquareMatrix::SquareMatrix(unsigned int rowCapacity) : rows(rowCapacity) {
    for(int i = 0; i < rowCapacity; i++) {
        rows.emplace_back(std::vector<unsigned int>(), std::vector<float>());
    }
}

void SquareMatrix::put(unsigned int row, unsigned int col, float val) {
    auto *indices = &this->rows[row].indices;
    auto *values = &this->rows[row].indices;

    auto newpos = std::lower_bound(rows[row].indices.begin(), rows[row].indices.end(), col);
    auto index = std::distance(rows[row].indices.begin(), newpos);

    rows[row].indices.insert(newpos, col);
    rows[row].values.insert(rows[row].values.begin() + index, val);

    if(row == 1) {
//      printf("put(..) reached!\n");
//      printf("row: %d, col: %d; indices.size: %zu, values.size: %zu\n", row, col, rows[row].indices.size(), rows[row].values.size());
    }
    if(rows[row].indices.size() != rows[row].values.size()) {
        printf("indices and values diverge at row: %d, col: %d; with: indices.size: %zu, values.size: %zu", row, col, rows[row].indices.size(), rows[row].values.size());
        exit(0);
    }
}

std::pair<SquareMatrix, std::vector<float>> Solver::assemble(const FemMesh &mesh, float (*function)(float, float)) {
    const int sideLen = mesh.activeNodes.size();
    const int elementCount = mesh.elems.size();

    auto global_K = SquareMatrix(sideLen);
    auto global_F = std::vector<float>(sideLen, 0.0);


    for(int el_ind = 0; el_ind < elementCount; el_ind++) {
        FiniteElement el = mesh.elems[el_ind];
        //printf("element %d with %d, %d, %d!\n", el_ind, el.nodes[0], el.nodes[1], el.nodes[2]);

        Node n0 = mesh.nodeOfElement(el_ind, 0);
        Node n1 = mesh.nodeOfElement(el_ind, 1);
        Node n2 = mesh.nodeOfElement(el_ind, 2);
        Node nodes[3] = {n0, n1, n2};


        unsigned int node_I;
        for(int i = 0; i < 3; i++) {
            switch(el.ntype[i]) {
                case active:
                case neumann:
                    node_I = el.nodes[i];

                    global_F[node_I] += F_i(n0.position, n1.position, n2.position, function);

                    for(int j = 0; j < 3; j++) {
                        unsigned int node_J = el.nodes[j];

                        if(el.ntype[j] == dirichlet) {
                            global_F[node_I] -= mesh.nodes[mesh.passiveNodes[node_J]].value * K_ij(n0.position, n1.position, n2.position, i, j);
                        } else {
                            global_K.put(node_I, node_J, K_ij(n0.position, n1.position, n2.position, i, j));
//                          //global_K[sideLen*node_I + node_J] += K_ij(n0.position, n1.position, n2.position, i, j); 
                        }
                    }
                    break;

                case dirichlet:
                    break;
            }
        }
    }

    return std::make_pair(global_K, global_F);
}


float aTbProd(const std::vector<float> &a, const AssembledRow &row) {

    if(row.indices.size() != row.indices.size()) {
        printf("indices and values diverge; with: indices.size: %zu, values.size: %zu", row.indices.size(), row.values.size());
        exit(0);
    }

    const unsigned int nnz = row.values.size();
    float res = 0.0;

    for(int i = 0; i < nnz; i++) {
        res += a[row.indices[i]] * row.values[i];
    }
    return res;
}

//assume the matrix is square with sides equal to the length of the vector a_T@M@b
float aTMbProd(const std::vector<float> &a, const SquareMatrix &M, const std::vector<float> &b) {
    const unsigned int len = a.size();
    float res = 0.0;
    float acc = 0.0;

    for(int i = 0; i < len; i++) {
        res += a[i]*aTbProd(b, M.rows[i]);
    }
    return res;
}

float aTbProd(const std::vector<float> &a, const std::vector<float> &b) {
    const unsigned int len = a.size();
    float res = 0.0;

    for(int i = 0; i < len; i++) {
        res += a[i] * b[i];
    }
    return res;
}

std::vector<float> CG(const SquareMatrix &M, std::vector<float> &F) {
    std::vector<float> x(F.size(), 0.0);

    std::vector<float> r(F);
    std::vector<float> p(F);

    float alpha = 0.0;
    float beta = 0.0;

    const unsigned int len = F.size();

    float rTr = aTbProd(r, r);
    float rTr_n = 0.0;

    for(int k = 0; k < len; k++) {
        alpha = rTr / aTMbProd(p, M, p);

        for(int i = 0; i < len; i++) {
            x[i] += alpha*p[i];
        }

        for(int i = 0; i < len; i++) {
            r[i] -= alpha * aTbProd(p, M.rows[i]);
        }

        rTr_n = aTbProd(r, r);

        if(rTr_n < 0.0001) {
            break;
        }

        beta = rTr_n / rTr;
        rTr = rTr_n;

        for(int i = 0; i < len; i++) {
            p[i] = p[i]*beta + r[i];
        }
    }
    return x;
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

void printSquareMat(const SquareMatrix &M) {
    for(auto row : M.rows) {
        printf("%zu: ", row.indices.size());

        for(int i = 0; i < row.indices.size(); i++) {
            printf("(%d, %f), ", row.indices[i], row.values[i]);
        }

        printf("\n");
    }
}

std::vector<float> Solver::solve(const FemMesh &m, float (*function)(float, float)) {
    auto K_F = assemble(m, function);
    printf("assembled!\n");
    SquareMatrix K = K_F.first;
    std::vector<float> F = K_F.second;

    return CG(K, F);
}
