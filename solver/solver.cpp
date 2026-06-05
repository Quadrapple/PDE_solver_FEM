#include "solver.h"
#include "femmesh.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <glm/common.hpp>
#include <iterator>
#include <simd>
#include <omp.h>

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
    auto &indices = this->rows[row].indices;
    auto &values = this->rows[row].values;

    auto newpos = std::lower_bound(indices.begin(), indices.end(), col);
    auto index = std::distance(indices.begin(), newpos);

    if(newpos == indices.end() || *newpos.base() != col) {
        indices.insert(newpos, col);
        values.insert(values.begin() + index, val);
    } else {
        values[index] += val;
    }

    if(indices.size() != values.size()) {
        printf("indices and values diverge at row: %d, col: %d; with: indices.size: %zu, values.size: %zu", row, col, rows[row].indices.size(), rows[row].values.size());
        exit(0);
    }
}

std::pair<SquareMatrix, std::vector<double>> Solver::assemble(const FemMesh &mesh, double (*function)(double, double)) {
    const int sideLen = mesh.activeNodes.size();
    const int elementCount = mesh.elems.size();

    auto global_K = SquareMatrix(sideLen);
    auto global_F = std::vector<double>(sideLen, 0.0);


    for(int elInd = 0; elInd < elementCount; elInd++) {
        FiniteElement el = mesh.elems[elInd];
        //printf("element %d with %d, %d, %d!\n", el_ind, el.nodes[0], el.nodes[1], el.nodes[2]);

        Node n0 = mesh.nodeOfElement(elInd, 0);
        Node n1 = mesh.nodeOfElement(elInd, 1);
        Node n2 = mesh.nodeOfElement(elInd, 2);
        Node nodes[3] = {n0, n1, n2};

        unsigned int node_I;
        for(int i = 0; i < 3; i++) {
            switch(el.ntype[i]) {
                case active:
                case neumann:
                    node_I = el.nodes[i];

                    global_F[node_I] -= F_i(n0.position, n1.position, n2.position, function, i);

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

//  if(row.indices.size() != row.indices.size()) {
//      printf("indices and values diverge; with: indices.size: %zu, values.size: %zu", row.indices.size(), row.values.size());
//      exit(0);
//  }

    const unsigned int nnz = row.values.size();
    double res = 0.0;

    for(int i = 0; i < nnz; i++) {
        res += a[row.indices[i]] * row.values[i];
    }
    return res;
}

CompactSquareMatrix compactify(const SquareMatrix &sqm) {
    CompactSquareMatrix cSqm;
    for(int i = 0; i < sqm.rows.size(); i++) {
        cSqm.rowStarts.push_back(cSqm.values.size());

        for(int j = 0; j < sqm.rows[i].values.size(); j++) {
            cSqm.values.push_back(sqm.rows[i].values[j]);
            cSqm.indices.push_back(sqm.rows[i].indices[j]);
        }
    }
    cSqm.rowStarts.push_back(cSqm.values.size());
    return cSqm;
}

extern time_t gettime();

std::vector<double> CG(const CompactSquareMatrix &M, const std::vector<double> &F) {

    const time_t t0 = gettime();
    time_t time = 0;
    printf("t: %ld; Entered CG loop\n", time);

    const unsigned int len = F.size();

    std::vector<double> r(F);
    std::vector<double> p(F.size());
    std::vector<double> precond(F.size());

    double rTz = 0.0;

    for(int i = 0; i < len; i++) {
        double acc = 0.0;
        for(int j = M.rowStarts[i]; j < M.rowStarts[i+1]; j++) {

            acc += glm::abs(M.values[j]);
//          if(i == M.indices[j]) {
//              precond[i] = 1.0 / M.values[j];
//              break;
//          }
        }
        precond[i] = 1.0 / acc;

        p[i] = r[i] * precond[i];
        rTz += r[i] * r[i] * precond[i];
    }

    std::vector<double> x(F.size(), 0.0);

    double alpha = 0.0;
    double beta = 0.0;
    double rTz_n = 0.0;

    std::vector<double> Mp(len);

    double pMp = 0.0;
    int k = 0;


#pragma omp parallel if(F.size() > 10000)
    while(k < len) {

#pragma omp for reduction (+:pMp)
        for(int i = 0; i < len; i++) {
            double acc = 0.0;

            for(int j = M.rowStarts[i]; j < M.rowStarts[i+1]; j++) {
                acc += p[M.indices[j]] * M.values[j];
            }

            Mp[i] = acc;
            pMp += acc * p[i];
        }

#pragma omp single
        {
            alpha = rTz / pMp;
            rTz_n = 0;
        }

#pragma omp for reduction (+:rTz_n)
        for(int i = 0; i < len; i++) {
            x[i] += alpha * p[i];
            r[i] -= alpha * Mp[i];
            rTz_n += r[i] * r[i] * precond[i];
        }

#pragma omp single
        {
//          time = gettime() - t0;
//          printf("t: %ld; k = %d, r = %lf\n", time, k, rTz_n);

            if(rTz_n < 0.0000001) {
                k = len;

            } else {
                beta = rTz_n / rTz;
                rTz = rTz_n;
                k++;
                pMp = 0.0;
            }
        }

#pragma omp for 
        for(int i = 0; i < len; i++) {
            p[i] *= beta;
            p[i] += r[i] * precond[i];
        }
    }

    time = gettime() - t0;
    printf("t: %ld; Exited CG loop\n", time);

    return x;
}

std::vector<double> Solver::estimateError(const FemMesh &mesh, const std::vector<double> &solution,
        const SkeletonMesh &estmesh, double (*f)(double, double), double h) {

    std::vector<double> estimates;
    std::vector<glm::dvec2> estPoints;

    estimates.reserve(mesh.activeNodes.size());

    if(h <= 0.0) {
        h = 0.0001;
    }

    glm::dvec2 dx, dy, ndx, ndy;
    for(const auto &o : estmesh.vertices) {
        dx = {o.x + h, o.y};
        ndx = {o.x - h, o.y};
        dy = {o.x, o.y + h};
        ndy = {o.x, o.y - h};

        estPoints.push_back(o);
        estPoints.push_back(dx );
        estPoints.push_back(ndx);
        estPoints.push_back(dy );
        estPoints.push_back(ndy);
    }

    const auto evals = mesh.evaluate(solution, estPoints);
    double dx_val, dy_val, ndx_val, ndy_val, o_val;

    for(int i = 0; i < evals.size(); i += 5) {
        const glm::dvec2 &o = estPoints[i];

        o_val = evals[i];
        dx_val = evals[i+1];
        dy_val = evals[i+2];
        ndx_val = evals[i+3];
        ndy_val = evals[i+4];

        if(std::isnan(dx_val) || 
                std::isnan(dy_val) ||
                std::isnan(ndx_val) ||
                std::isnan(ndy_val) ||
                std::isnan(o_val)) {
            estimates.push_back(0);
            continue;
        }

        //estimate laplacian
        double estVal = (dx_val + dy_val + ndx_val + ndy_val - 4*o_val) / (h*h);
//      printf("(%f, %f) with %f\n", o.x, o.y, estVal);

        estimates.push_back(glm::abs(estVal - f(o.x, o.y)));
    }

    return estimates;
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
    CompactSquareMatrix K = compactify(K_F.first);
    std::vector<double> F = K_F.second;
    std::vector<double> solution = CG(K, F);

    return solution;
}
