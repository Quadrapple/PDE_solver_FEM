
#pragma once
#include "exprtk_wrapper.h"
#include "femmesh.h"


struct mVertex {
    glm::vec2 pos;
    float value;
};

struct AssembledRow {
    std::vector<unsigned int> indices;
    std::vector<double> values;
};

struct CompactSquareMatrix {
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
        std::vector<double> solve(const FemMesh &mesh, const RuntimeExpression &function);
        std::vector<double> estimateError(const FemMesh &mesh, const std::vector<double> &solution,
                const SkeletonMesh &estmesh, const RuntimeExpression &function, double h);
    private:

        std::pair<SquareMatrix, std::vector<double>> assemble(const FemMesh &mesh, const RuntimeExpression &function);
};
