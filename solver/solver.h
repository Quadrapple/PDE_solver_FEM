
#pragma once
#include "femmesh.h"


struct AssembledRow {
    std::vector<unsigned int> indices;
    std::vector<float> values;
};

class SquareMatrix {
    public:
        std::vector<AssembledRow> rows;

        SquareMatrix();
        SquareMatrix(unsigned int rowCapacity);
        void put(unsigned int row, unsigned int col, float val);
};

class Solver {

    public:
        std::vector<float> solve(const FemMesh &mesh, float (*function)(float, float));
    private:

        std::pair<SquareMatrix, std::vector<float>> assemble(const FemMesh &mesh, float (*function)(float, float));
};
