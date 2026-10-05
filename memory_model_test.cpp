#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/sema/typechecker.hpp"
#include <iostream>
#include <string>

static bool check(const std::string& label, const std::string& src, bool expected) {
    try {
        Lexer lexer(src);
        Parser parser(lexer.tokenize());
        Program program = parser.parseProgram();
        SymbolTable symbols;
        TypeChecker checker(symbols);
        checker.checkProgram(program);
        if (!expected) { std::cerr << "FAIL: " << label << " expected rejection\n"; return false; }
        std::cout << "PASS: " << label << "\n";
        return true;
    } catch (const std::exception& e) {
        if (expected) { std::cerr << "FAIL: " << label << ": " << e.what() << "\n"; return false; }
        std::cout << "PASS: " << label << " (rejected: " << e.what() << ")\n";
        return true;
    }
}

int main() {
    bool ok = true;
    ok &= check("shared Views may coexist",
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v1: View[i32] = a[:]\n"
        "    v2: View[i32] = a[:]\n"
        "    return v1[0]\n", true);

    ok &= check("read-only View cannot mutate",
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v: View[i32] = a[:]\n"
        "    v[0] = 9\n"
        "    return a[0]\n", false);

    ok &= check("owner cannot mutate while View is alive",
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v: View[i32] = a[:]\n"
        "    a[0] = 9\n"
        "    return v[0]\n", false);

    ok &= check("EditView gives exclusive mutation",
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v: EditView[i32] = a[:]\n"
        "    v[0] = 9\n"
        "    return v[0]\n", true);

    ok &= check("second mutable borrow is rejected",
        "main() i32:\n"
        "    a: [2]i32 = [1, 2]\n"
        "    v1: EditView[i32] = a[:]\n"
        "    v2: EditView[i32] = a[:]\n"
        "    return v1[0]\n", false);

    ok &= check("for-loop element borrow blocks array mutation",
        "main() i32:\n"
        "    a = [1, 2, 3]\n"
        "    for x in a:\n"
        "        a.push(4)\n"
        "    return 0\n", false);

    ok &= check("mutable reference can mutate through struct field",
        "Point[x: i32, y: i32]\n"
        "main() i32:\n"
        "    p = Point[x: 1, y: 2]\n"
        "    r: &mut Point = &mut p\n"
        "    r.x = 8\n"
        "    return p.x\n", true);

    ok &= check("shared reference blocks owner mutation",
        "Point[x: i32, y: i32]\n"
        "main() i32:\n"
        "    p = Point[x: 1, y: 2]\n"
        "    r: &Point = &p\n"
        "    p.x = r.x\n"
        "    return p.x\n", false);

    ok &= check("reference array indexing yields a value",
        "main() i32:\n"
        "    a: [2]i32 = [10, 20]\n"
        "    r: &[2]i32 = &a\n"
        "    x = r[0] + r[1]\n"
        "    return x\n", true);

    ok &= check("reference array type mismatch is rejected",
        "main() i32:\n"
        "    a = [10, 20]\n"
        "    r: &[2]i32 = &a\n"
        "    return 0\n", false);

    ok &= check("borrowed non-copy value cannot escape as an owner",
        "bad() []i32:\n"
        "    xs = []i32\n"
        "    xs.push(1)\n"
        "    r: &[]i32 = &xs\n"
        "    return r\n"
        "main() i32:\n"
        "    return 0\n", false);

    ok &= check("move of an owner with an outstanding shared borrow is rejected",
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    r: &[]i32 = &xs\n"
        "    ys: []i32 = xs\n"
        "    return r[0]\n", false);

    ok &= check("returned EditView keeps owner exclusively borrowed",
        "make_view(a: &mut []i32) EditView[i32]:\n"
        "    v: EditView[i32] = a[:]\n"
        "    return v\n"
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: EditView[i32] = make_view(&mut xs)\n"
        "    x: i32 = xs[0] + v[0]\n"
        "    return x\n", false);

    ok &= check("EditView rejects plain owner reads",
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: EditView[i32] = xs[:]\n"
        "    x: i32 = xs[0] + v[0]\n"
        "    return x\n", false);

    ok &= check("returned EditView can still be used exclusively",
        "make_view(a: &mut []i32) EditView[i32]:\n"
        "    v: EditView[i32] = a[:]\n"
        "    return v\n"
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: EditView[i32] = make_view(&mut xs)\n"
        "    v[0] = 9\n"
        "    return v[0]\n", true);


    ok &= check("shared View ends at last use",
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: View[i32] = xs[:]\n"
        "    x: i32 = v[0]\n"
        "    xs.push(2)\n"
        "    return x\n", true);

    ok &= check("unused View no longer blocks owner",
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: View[i32] = xs[:]\n"
        "    xs.push(2)\n"
        "    return xs[0]\n", true);

    ok &= check("moving exclusive view transfers the borrow token",
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v1: EditView[i32] = xs[:]\n"
        "    v2: EditView[i32] = v1\n"
        "    v2[0] = 9\n"
        "    return v2[0]\n", true);

    ok &= check("nested View cannot escape through a returned struct",
        "Holder[v: View[i32]]\n"
        "make() Holder:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: View[i32] = xs[:]\n"
        "    h = Holder[v: v]\n"
        "    return h\n"
        "main() i32:\n"
        "    return 0\n", false);

    ok &= check("nested View field cannot escape from an aggregate",
        "Holder[v: View[i32], x: i32]\n"
        "make() View[i32]:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: View[i32] = xs[:]\n"
        "    h = Holder[v: v, x: 7]\n"
        "    return h.v\n"
        "main() i32:\n"
        "    return 0\n", false);

    ok &= check("scalar field remains returnable from a borrowed aggregate",
        "Holder[v: View[i32], x: i32]\n"
        "make() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(1)\n"
        "    v: View[i32] = xs[:]\n"
        "    h = Holder[v: v, x: 7]\n"
        "    return h.x\n", true);

    ok &= check("arena-backed storage cannot escape through a returned struct",
        "Holder[data: []i32]\n"
        "make() Holder:\n"
        "    arena: Arena = Arena.create(64)\n"
        "    xs: []i32 = [1, 2, 3] in arena\n"
        "    h = Holder[data: xs]\n"
        "    return h\n"
        "main() i32:\n"
        "    return 0\n", false);

    ok &= check("arena-backed storage from an Arena parameter may be returned",
        "make(arena: Arena) []i32:\n"
        "    xs: []i32 = [1, 2, 3] in arena\n"
        "    return xs\n"
        "main() i32:\n"
        "    arena: Arena = Arena.create(64)\n"
        "    xs: []i32 = make(arena)\n"
        "    return xs[1]\n", true);

    ok &= check("arena-backed storage cannot escape through a pushed aggregate",
        "Holder[data: []i32]\n"
        "make() []Holder:\n"
        "    arena: Arena = Arena.create(64)\n"
        "    xs: []i32 = [1] in arena\n"
        "    h = Holder[data: xs]\n"
        "    hs: []Holder = []Holder\n"
        "    hs.push(h)\n"
        "    return hs\n"
        "main() i32:\n"
        "    return 0\n", false);

    ok &= check("borrowed aggregate remains usable locally when only a scalar escapes",
        "Holder[v: View[i32], x: i32]\n"
        "main() i32:\n"
        "    xs: []i32 = []i32\n"
        "    xs.push(5)\n"
        "    v: View[i32] = xs[:]\n"
        "    h = Holder[v: v, x: 7]\n"
        "    return h.x + h.v[0]\n", true);

    return ok ? 0 : 1;
}
