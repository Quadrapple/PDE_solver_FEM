
#pragma once
#include "femmesh.h"


struct mVertex {
    glm::vec2 pos;
    glm::vec3 color;
};

struct AssembledRow {
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
        std::vector<mVertex> estimateError(const FemMesh &mesh, const std::vector<double> &solution,
                int size, double range, double (*f)(double, double), double h = 0.0);
    private:

        std::pair<SquareMatrix, std::vector<double>> assemble(const FemMesh &mesh, double (*function)(double, double));
};
