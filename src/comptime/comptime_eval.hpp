#pragma once
#include "../parser/ast.hpp"
#include "../sema/comptime_value.hpp"
#include "../sema/symbol_table.hpp"

class ComptimeEvaluator {
public:
    explicit ComptimeEvaluator(SymbolTable& symtab) : symbols(symtab) {}
    ComptimeValue evaluate(const Expr* expr);
    void evaluateAndDeclare(const Stmt* comptimeDecl);

private:
    SymbolTable& symbols;
    ComptimeValue evalBinary(const Expr* expr);
    ComptimeValue evalUnary(const Expr* expr);
    int64_t checkedIntOp(int64_t l, int64_t r, const std::string& op, int line);
    double asFloat(const ComptimeValue& v, int line);
    bool isNumeric(const ComptimeValue& v);
};
