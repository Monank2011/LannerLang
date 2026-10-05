
#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/sema/symbol_table.hpp"
#include "src/sema/typechecker.hpp"
#include <iostream>
#include <stdexcept>

static void checkProgram(const std::string& source, const char* label, bool shouldPass) {
    try {
        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        Program program = parser.parseProgram();
        SymbolTable symbols;
        TypeChecker checker(symbols);
        checker.checkProgram(program);
        if (!shouldPass) throw std::runtime_error(std::string(label) + ": expected failure, got success");
        std::cout << "PASS: " << label << "\n";
    } catch (const std::exception& e) {
        if (shouldPass) {
            std::cerr << "FAIL: " << label << ": " << e.what() << "\n";
            std::exit(1);
        }
        std::cout << "PASS: " << label << " (rejected)\n";
    }
}

int main() {
    checkProgram(
        "add(a: i32, b: i32) i32:\n"
        "    x = a + b * 2\n"
        "    return x\n",
        "scalar function", true);

    checkProgram(
        "Box[value: i32]\n"
        "main() i32:\n"
        "    x = Box[value: 3]\n"
        "    y = &x\n"
        "    return y.value\n",
        "struct + reference", true);

    checkProgram(
        "Piece[square: i8]\n"
        "Move[from: i8, to: i8, promo: i8]\n"
        "Board[pieces: []Piece]\n"
        "MoveError[code: i32]\n"
        "generateMoves(board: Board, arena: Arena) Result[[]Move, MoveError]:\n"
        "    moves = []Move in arena\n"
        "    for piece in board.pieces:\n"
        "        moves.push(Move[from: piece.square, to: piece.square, promo: 0])\n"
        "    moves.isEmpty() -> return Err(MoveError.NoLegalMoves)\n"
        "    return Ok(moves)\n",
        "arena + loop + Result", true);

    checkProgram(
        "main() i32:\n"
        "    x = 1\n"
        "    x = x\n"
        "    return 0\n",
        "copy self-assignment", true);

    checkProgram(
        "Box[value: i32]\n"
        "main() i32:\n"
        "    x = Box[value: 3]\n"
        "    x = x\n"
        "    return 0\n",
        "owned self-move", false);

    checkProgram(
        "main() i32:\n"
        "    return 1\n"
        "    x = 2\n",
        "unreachable after return", false);

    checkProgram(
        "main() i32:\n"
        "    a: f32 = -0.9\n"
        "    b: f32 = 1.25\n"
        "    c: f32 = a * b + 2.0\n"
        "    return (c as i32) + 42\n",
        "typed negative f32 literal context", true);

    checkProgram(
        "main() i32:\n"
        "    s: i64 = -918273645\n"
        "    t: i64 = s + 2\n"
        "    return (((t as u64) & 255) as i32)\n",
        "typed negative i64 literal context", true);

    return 0;
}
