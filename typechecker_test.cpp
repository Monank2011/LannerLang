#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/sema/typechecker.hpp"
#include <iostream>

std::string describeType(const TypeNode* t) {
    if (!t) return "<null>";
    std::string s = t->isReference ? "&" : "";
    if (t->isArray) s += "[]";
    s += t->name;
    if (t->isOptional) s += "?";
    if (t->origin.kind == lanner::memory::StorageOriginKind::Arena) s += " (arena: " + t->origin.binding + ")";
    return s;
}

void runFunctionTest(const std::string& label, const std::string& src,
                      const std::vector<std::string>& namesToCheck) {
    std::cout << "=== " << label << " ===\n";
    try {
        Lexer lex(src);
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        Program prog = parser.parseProgram();

        auto& fn = prog.decls[0]->fn;

        SymbolTable symtab;
        TypeChecker tc(symtab);

        std::vector<std::unique_ptr<TypeNode>> paramTypes;
        for (auto& p : fn->params) {
            auto t = std::make_unique<TypeNode>();
            t->name = p.type->name;
            t->isArray = p.type->isArray;
            t->isOptional = p.type->isOptional;
            paramTypes.push_back(std::move(t));

            Symbol sym;
            sym.name = p.name;
            sym.type = paramTypes.back().get();
            symtab.declare(p.name, sym);
        }

        for (auto& stmt : fn->body) {
            tc.checkStmt(stmt.get());
        }

        for (auto& name : namesToCheck) {
            Symbol* sym = symtab.resolve(name);
            if (!sym) std::cout << "  " << name << " = <not found>\n";
            else std::cout << "  " << name << " : " << describeType(sym->type)
                            << (sym->memory.isMoved() ? " [MOVED]" : "") << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << "\n";
    }
    std::cout << "\n";
}

int main() {
    runFunctionTest("literal + binary arithmetic infers i32",
        "f() i32:\n"
        "    x = 5 + 3\n",
        {"x"});

    runFunctionTest("type mismatch in arithmetic throws",
        "f() i32:\n"
        "    x = 5 + 1.5\n",
        {"x"});

    runFunctionTest("comparison infers bool",
        "f() i32:\n"
        "    flag = 5 > 3\n",
        {"flag"});

    runFunctionTest("assigning a bare identifier moves it, later use throws",
        "f(board: Board, arena: Arena) i32:\n"
        "    y = board\n"
        "    board\n",
        {"y", "board"});

    runFunctionTest("reference does not move (no error on reuse)",
        "f(board: Board, arena: Arena) i32:\n"
        "    y = &board\n"
        "    board\n",
        {"y", "board"});

    runFunctionTest("arena alloc tags the resulting type with the arena name",
        "f(board: Board, arena: Arena) i32:\n"
        "    moves = []Move in arena\n",
        {"moves"});

    runFunctionTest("arena target that isn't an Arena throws",
        "f(board: Board) i32:\n"
        "    moves = []Move in board\n",
        {"moves"});

    return 0;
}