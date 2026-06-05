
#pragma once
#include "femmesh.h"


struct mVertex {
    glm::vec2 pos;
    float value;
};

struct AssembledRow {
    std::vector<unsigned int> indices;
    std::vector<double> values;
};

class CompactSquareMatrix {
    public:
        std::vector<unsigned int> rowStarts;
        std::vector<unsigned int> indices;
        std::vector<double> values;
};

class SquareMatrix {
    public:
        std::vector<AssembledRow> rows;

        SquareMatrix();
        SquareMatrix(unsigned int rowCapacity);
        void put(unsigned int row, unsigned int col, double val);
};

class Solver {

    public:
        std::vector<double> solve(const FemMesh &mesh, double (*function)(double, double));
        std::vector<double> estimateError(const FemMesh &mesh, const std::vector<double> &solution,
                const SkeletonMesh &estmesh, double (*f)(double, double), double h);
    private:

        std::pair<SquareMatrix, std::vector<double>> assemble(const FemMesh &mesh, double (*function)(double, double));
};
