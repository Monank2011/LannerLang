#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/sema/typechecker.hpp"
#include "src/ir/hir_lowerer.hpp"
#include "src/ir/hir_llvm_codegen.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    const std::string source =
        "comptime BASE = 41\n"
        "comptime NAME = \"stable\"\n"
        "main() i32:\n"
        "    x: i32 = BASE + 1\n"
        "    return x\n";
    try {
        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        Program program = parser.parseProgram();
        SymbolTable symbols;
        TypeChecker checker(symbols);
        checker.checkProgram(program);

        stable::hir::Lowerer lowerer;
        const auto module = lowerer.lowerProgram(program);
        if (module.functions.size() != 1) throw std::runtime_error("expected one HIR function");
        bool sawBase = false;
        for (const auto& block : module.functions[0].blocks) {
            for (const auto& inst : block.instructions) {
                if (inst.op == stable::hir::Opcode::ConstInt && inst.intValue == 41) sawBase = true;
            }
        }
        if (!sawBase) throw std::runtime_error("comptime BASE was not materialized as an HIR constant");

        stable::hir::LLVMCodegen codegen;
        const auto llvm = codegen.generate(module);
        if (llvm.find("add i32") == std::string::npos) throw std::runtime_error("HIR LLVM output lost the runtime addition");
        if (llvm.find("41") == std::string::npos) throw std::runtime_error("HIR LLVM output lost the comptime constant");
        std::cout << "hir_comptime_test: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "hir_comptime_test: FAIL: " << e.what() << "\n";
        return 1;
    }
}
