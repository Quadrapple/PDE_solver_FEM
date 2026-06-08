#include "exprtk_wrapper.h"
#include "exprtk.hpp"

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;

#define exprtk ((ExprImpl*)exprimpl)

struct ExprImpl {
    expression_t expr;
    symbol_table_t exprSymbolTable;
};

double RuntimeExpression::eval(double x, double y) const {
    exprValues[0] = x;
    exprValues[1] = y;

    return exprtk->expr.value();
}

double RuntimeExpression::operator()(double x, double y) const {
    return this->eval(x, y);
}

RuntimeExpression::RuntimeExpression(std::string exprStr) {
    this->exprStr = exprStr;
    this->exprimpl = new ExprImpl();

    exprValues[0] = 0.0;
    exprValues[1] = 0.0;

    exprtk->exprSymbolTable.add_variable("x", exprValues[0]);
    exprtk->exprSymbolTable.add_variable("y", exprValues[1]);

    exprtk->expr.register_symbol_table(exprtk->exprSymbolTable);
    update();
}

bool RuntimeExpression::update() {
    parser_t parser;
    bool res = parser.compile(exprStr, exprtk->expr);
    if(res) {
        exprtk->expr.register_symbol_table(exprtk->exprSymbolTable);
        return true;
    }
    return false;
}

bool RuntimeExpression::update(std::string exprStr) {
    parser_t parser;
    this->exprStr = exprStr;

    bool res = parser.compile(exprStr, exprtk->expr);
    if(res) {
        exprtk->expr.register_symbol_table(exprtk->exprSymbolTable);
        return true;
    }
    return false;
}

RuntimeExpression::~RuntimeExpression() {
    delete exprtk;
}
