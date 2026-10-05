#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/comptime/comptime_eval.hpp"
#include <iostream>

std::string describe(const ComptimeValue& v) {
    if (std::holds_alternative<int64_t>(v)) return "int64 " + std::to_string(std::get<int64_t>(v));
    if (std::holds_alternative<double>(v)) return "double " + std::to_string(std::get<double>(v));
    if (std::holds_alternative<bool>(v)) return std::string("bool ") + (std::get<bool>(v) ? "true" : "false");
    if (std::holds_alternative<std::string>(v)) return "string \"" + std::get<std::string>(v) + "\"";
    return "?";
}

void runProgramTest(const std::string& label, const std::string& src, const std::vector<std::string>& namesToCheck) {
    std::cout << "=== " << label << " ===\n";
    try {
        Lexer lex(src);
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        Program prog = parser.parseProgram();

        SymbolTable symtab;
        ComptimeEvaluator eval(symtab);

        for (auto& d : prog.decls) {
            if (d->kind == DeclKind::ComptimeGlobal) {
                eval.evaluateAndDeclare(d->comptimeGlobal.get());
            }
        }

        for (auto& name : namesToCheck) {
            Symbol* sym = symtab.resolve(name);
            if (!sym || !sym->comptimeValue.has_value()) {
                std::cout << "  " << name << " = <not found>\n";
            } else {
                std::cout << "  " << name << " = " << describe(*sym->comptimeValue) << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << "\n";
    }
    std::cout << "\n";
}

int main() {
    runProgramTest("simple int literal", "comptime BOARD_SIZE = 8\n", {"BOARD_SIZE"});
    runProgramTest("arithmetic with precedence", "comptime AREA = 4 * 2 + 1\n", {"AREA"});
    runProgramTest("chained comptime reference",
        "comptime A = 5\n"
        "comptime B = A * 2\n",
        {"A", "B"});
    runProgramTest("integer division truncates", "comptime APPROX = 22 / 7\n", {"APPROX"});
    runProgramTest("float arithmetic promotes to double", "comptime F = 1.5 + 2.5\n", {"F"});
    runProgramTest("comparison produces bool", "comptime FLAG = 5 > 3\n", {"FLAG"});
    runProgramTest("division by zero throws", "comptime BAD = 5 / 0\n", {"BAD"});
    runProgramTest("unknown identifier throws", "comptime X = UNKNOWN_VAR\n", {"X"});
    runProgramTest("string plus int throws (type mismatch)", "comptime C = \"hello\" + 5\n", {"C"});
    return 0;
}