
#pragma once
#include "femmesh.h"
class Solver {

    public:
        std::vector<float> solve(const FemMesh &mesh, float (*function)(float, float));
    private:

        void integrate();
        std::pair<std::vector<float>, std::vector<float>> assemble(const FemMesh &mesh, float (*function)(float, float));
};
