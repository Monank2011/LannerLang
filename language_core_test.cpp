#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/sema/typechecker.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

static bool checkSource(const std::string& source, bool shouldPass, const std::string& label) {
    try {
        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        Program program = parser.parseProgram();
        SymbolTable symbols;
        TypeChecker checker(symbols);
        checker.checkProgram(program);
        if (!shouldPass) {
            std::cerr << "FAIL: " << label << ": expected rejection\n";
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

    ok &= checkSource(
        "main() i32:\n"
        "    a: [4]u64 = [1, 2, 4, 8]\n"
        "    i: usize = 2\n"
        "    a[i] = 32\n"
        "    return a[i] as i32\n",
        true, "fixed array + usize index + mutation");

    ok &= checkSource(
        "comptime N = 2 + 1\n"
        "main() i32:\n"
        "    a: [N]i32 = [1, 2, 3]\n"
        "    return a[2]\n",
        true, "comptime expression as fixed-array size");

    ok &= checkSource(
        "main() i32:\n"
        "    comptime N = 0x10 | 3\n"
        "    return (N >> 2) as i32\n",
        true, "local comptime hex/bitwise expression");

    ok &= checkSource(
        "enum Color:\n"
        "    White\n"
        "    Black = 7\n"
        "    Red\n"
        "main() i32:\n"
        "    if Color.Red == Color.White:\n"
        "        return 1\n"
        "    return 0\n",
        true, "enum declaration + qualified variants");

    ok &= checkSource(
        "sum(view: View[u64]) usize:\n"
        "    return view.len()\n",
        true, "non-owning View type");

    ok &= checkSource(
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v: EditView[i32] = a[:]\n"
        "    v[0] = 9\n"
        "    return v[0]\n",
        true, "exclusive mutable EditView");

    ok &= checkSource(
        "Point[x: i32, y: i32]\n"
        "main() i32:\n"
        "    p = Point[x: 1, y: 2]\n"
        "    p.x = 9\n"
        "    return p.x\n",
        true, "struct field lvalue assignment");

    ok &= checkSource(
        "main() i32:\n"
        "    const x: i32 = 4\n"
        "    x = 5\n"
        "    return x\n",
        false, "const mutation rejected");

    ok &= checkSource(
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    return a[2]\n",
        false, "constant fixed-array out-of-bounds rejected");

    ok &= checkSource(
        "main() i32:\n"
        "    x: u8 = 255\n"
        "    y: u16 = x as u16\n"
        "    z: i8 = y as i8\n"
        "    return z as i32\n",
        true, "integer width conversions");

    ok &= checkSource(
        "main() i32:\n"
        "    i: i32 = 0\n"
        "    while i < 3:\n"
        "        i = i + 1\n"
        "    if i == 3:\n"
        "        return 7\n"
        "    return 1\n",
        true, "plain assignment updates the nearest visible binding");

    ok &= checkSource(
        "identity(v: View[i32]) View[i32]:\n"
        "    return v\n",
        true, "View parameter may be returned");

    ok &= checkSource(
        "bad() View[i32]:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    return a[: ]\n",
        false, "View cannot escape a local fixed array");

    ok &= checkSource(
        "bad2() View[i32]:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v: View[i32] = a[:]\n"
        "    return v\n",
        false, "named local View cannot escape its owner");

    return ok ? 0 : 1;
}
