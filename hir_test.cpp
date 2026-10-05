#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/sema/typechecker.hpp"
#include "src/ir/hir.hpp"
#include "src/ir/hir_lowerer.hpp"
#include <iostream>
#include <string>

static Program parseAndCheck(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    Program program = parser.parseProgram();
    SymbolTable symbols;
    TypeChecker checker(symbols);
    checker.checkProgram(program);
    return program;
}

static bool expectLower(const std::string& label, const std::string& source, bool shouldPass) {
    try {
        Program program = parseAndCheck(source);
        stable::hir::Lowerer lowerer;
        auto module = lowerer.lowerProgram(program);
        if (!shouldPass) {
            std::cerr << "FAIL: " << label << " expected HIR rejection\n";
            return false;
        }
        if (module.functions.empty()) {
            std::cerr << "FAIL: " << label << " produced no functions\n";
            return false;
        }
        std::cout << "PASS: " << label << "\n";
        return true;
    } catch (const std::exception& e) {
        if (shouldPass) {
            std::cerr << "FAIL: " << label << ": " << e.what() << "\n";
            return false;
        }
        std::cout << "PASS: " << label << " (rejected: " << e.what() << ")\n";
        return true;
    }
}

int main() {
    bool ok = true;
    ok &= expectLower(
        "scalar arithmetic and calls",
        "add(a: i64, b: i64) i64:\n"
        "    return a + b\n"
        "main() i32:\n"
        "    x: i64 = add(20, 22)\n"
        "    return x as i32\n",
        true);

    ok &= expectLower(
        "while CFG and nearest-scope assignment",
        "main() i32:\n"
        "    i: i32 = 0\n"
        "    while i < 4:\n"
        "        i = i + 1\n"
        "    if i == 4:\n"
        "        return 7\n"
        "    return 1\n",
        true);

    ok &= expectLower(
        "enums and u64 constants",
        "enum Color:\n"
        "    White\n"
        "    Black = 7\n"
        "main() i32:\n"
        "    x: u64 = 0xFFFFFFFFFFFFFFFF\n"
        "    if Color.Black == Color.White:\n"
        "        return 1\n"
        "    if x == 0xFFFFFFFFFFFFFFFF:\n"
        "        return 7\n"
        "    return 2\n",
        true);

    ok &= expectLower(
        "struct aggregate field extraction",
        "Point[x: i32, y: i32]\n"
        "main() i32:\n"
        "    p = Point[x: 20, y: 22]\n"
        "    return p.x + p.y\n",
        true);

    ok &= expectLower(
        "fixed-array aggregate indexing",
        "main() i32:\n"
        "    a: [3]i32 = [10, 20, 12]\n"
        "    a[1] = 20\n"
        "    return a[0] + a[1] + a[2]\n",
        true);

    {
        const std::string source =
            "Holder[v: View[i32], x: i32]\n"
            "main() i32:\n"
            "    xs: []i32 = []i32\n"
            "    xs.push(5)\n"
            "    v: View[i32] = xs[:]\n"
            "    h = Holder[v: v, x: 7]\n"
            "    return h.x\n";
        try {
            Program program = parseAndCheck(source);
            stable::hir::Lowerer lowerer;
            auto module = lowerer.lowerProgram(program);
            if (module.functions.empty()) {
                std::cerr << "FAIL: HIR preserves nested provenance: no functions\n";
                ok = false;
            } else {
                const auto& locals = module.functions.front().locals;
                auto it = locals.end();
                for (auto candidate = locals.begin(); candidate != locals.end(); ++candidate) {
                    if (candidate->first == "h" || candidate->first.rfind("h.", 0) == 0) {
                        it = candidate;
                        break;
                    }
                }
                if (it == locals.end() || it->second.nestedOrigins.empty()) {
                    std::cerr << "FAIL: HIR preserves nested provenance: metadata missing\n";
                    ok = false;
                } else {
                    std::cout << "PASS: HIR preserves nested provenance\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "FAIL: HIR preserves nested provenance: " << e.what() << "\n";
            ok = false;
        }
    }

    return ok ? 0 : 1;
}
