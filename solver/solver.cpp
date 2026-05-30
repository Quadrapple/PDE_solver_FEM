#include "solver.h"
#include "femmesh.h"
#include <algorithm>
#include <cstdio>
#include <glm/common.hpp>
#include <iterator>

double triangle_area(glm::dvec2 p0, glm::dvec2 p1, glm::dvec2 p2) {
    return ((p0.x - p1.x)*(p0.y - p2.y) - (p0.x - p2.x)*(p0.y - p1.y)) / 2;
}

//local numbering
double K_ij(glm::dvec2 p0, glm::dvec2 p1, glm::dvec2 p2, int i, int j) {
    glm::dvec2 grads[] = {{p1.y - p2.y, p2.x - p1.x},
                         {p2.y - p0.y, p0.x - p2.x},
                         {p0.y - p1.y, p1.x - p0.x}};
    return glm::dot(grads[i], grads[j]) / (4 * triangle_area(p0,p1,p2));
}

//edge-midpoint approximation
double F_i(glm::dvec2 p1, glm::dvec2 p2, glm::dvec2 p3, double (*function)(double, double), int i) {
    const glm::dvec2 e12 = (p1 + p2) * 0.5;
    const glm::dvec2 e13 = (p1 + p3) * 0.5;
    const glm::dvec2 e23 = (p2 + p3) * 0.5;
    glm::dvec3 w;
    switch(i) {
        case 0:
            w = {0.5, 0.5, 0.0};
            break;
        case 1:
            w = {0.5, 0.0, 0.5};
            break;
        case 2:
            w = {0.0, 0.5, 0.5};
            break;
        default:
            printf("Got %d insted of 0,1 or 2 as local coordinate!\n", i);
            assert(false);
    }

    const double total = function(e12.x, e12.y) * w.x
        + function(e13.x, e13.y) * w.y
        + function(e23.x, e23.y) * w.z;
    return (total / 3) * triangle_area(p1, p2, p3);
}

SquareMatrix::SquareMatrix() : rows(){
}

SquareMatrix::SquareMatrix(unsigned int rowCapacity) : rows(rowCapacity) {
}

void SquareMatrix::put(unsigned int row, unsigned int col, double val) {
    auto *indices = &this->rows[row].indices;
    auto *values = &this->rows[row].values;

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

std::pair<SquareMatrix, std::vector<double>> Solver::assemble(const FemMesh &mesh, double (*function)(double, double)) {
    const int sideLen = mesh.activeNodes.size();
    const int elementCount = mesh.elems.size();

    auto global_K = SquareMatrix(sideLen);
    auto global_F = std::vector<double>(sideLen, 0.0);


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

                    global_F[node_I] += F_i(n0.position, n1.position, n2.position, function, i);

                    for(int j = 0; j < 3; j++) {
                        unsigned int node_J = el.nodes[j];

                        if(el.ntype[j] == dirichlet) {
                            global_F[node_I] -= mesh.nodes->at(mesh.passiveNodes[node_J]).value * K_ij(n0.position, n1.position, n2.position, i, j);
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


double aTbProd(const std::vector<double> &a, const AssembledRow &row) {

    if(row.indices.size() != row.indices.size()) {
        printf("indices and values diverge; with: indices.size: %zu, values.size: %zu", row.indices.size(), row.values.size());
        exit(0);
    }

    const unsigned int nnz = row.values.size();
    double res = 0.0;

    for(int i = 0; i < nnz; i++) {
        res += a[row.indices[i]] * row.values[i];
    }
    return res;
}

//assume the matrix is square with sides equal to the length of the vector a_T@M@b
double aTMbProd(const std::vector<double> &a, const SquareMatrix &M, const std::vector<double> &b) {
    const unsigned int len = a.size();
    double res = 0.0;
    double acc = 0.0;

    for(int i = 0; i < len; i++) {
        res += a[i]*aTbProd(b, M.rows[i]);
    }
    return res;
}

double aTbProd(const std::vector<double> &a, const std::vector<double> &b) {
    const unsigned int len = a.size();
    double res = 0.0;

    for(int i = 0; i < len; i++) {
        res += a[i] * b[i];
    }
    return res;
}

std::vector<double> CG(const SquareMatrix &M, std::vector<double> &F) {
    std::vector<double> x(F.size(), 0.0);

    std::vector<double> r(F);
    std::vector<double> p(F);

    double alpha = 0.0;
    double beta = 0.0;

    const unsigned int len = F.size();

    double rTr = aTbProd(r, r);
    double rTr_n = 0.0;

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

void printSquareMat(const SquareMatrix &M) {
    for(auto row : M.rows) {
        printf("%zu: ", row.indices.size());

        for(int i = 0; i < row.indices.size(); i++) {
            printf("(%d, %f), ", row.indices[i], row.values[i]);
        }

        printf("\n");
    }
}

std::vector<double> Solver::solve(const FemMesh &m, double (*function)(double, double)) {
    auto K_F = assemble(m, function);
    printf("assembled!\n");
    SquareMatrix K = K_F.first;
    std::vector<double> F = K_F.second;

    return CG(K, F);
}
