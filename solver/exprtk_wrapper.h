#pragma once

#include <string>
typedef double (*DoubleDouble_toDouble)(double, double);

class RuntimeExpression {
    public:
        RuntimeExpression(std::string exprStr);
        ~RuntimeExpression();

        RuntimeExpression operator=(const RuntimeExpression&) = delete;
        RuntimeExpression(const RuntimeExpression&) = delete;

        double operator()(double x, double y) const;

        std::string exprStr;

        inline static double exprValues[2];

        double eval(double x, double y) const;

        bool update();
        bool update(std::string exprStr);

    private:
        void *exprimpl;
};
