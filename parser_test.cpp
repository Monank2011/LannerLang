#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include <iostream>

void indent(int n) { for (int i = 0; i < n; i++) std::cout << "  "; }

void printType(const TypeNode* t, int d) {
    if (!t) { indent(d); std::cout << "<null type>\n"; return; }
    indent(d);
    std::cout << "Type: " << (t->isArray ? "[]" : "") << t->name << (t->isOptional ? "?" : "");
    if (t->origin.kind == lanner::memory::StorageOriginKind::Arena) std::cout << " (arena: " << t->origin.binding << ")";
    std::cout << "\n";
    for (auto& g : t->generics) printType(g.get(), d + 1);
}

void printExpr(const Expr* e, int d) {
    if (!e) { indent(d); std::cout << "<null expr>\n"; return; }
    indent(d);
    switch (e->kind) {
        case ExprKind::IntLit: std::cout << "IntLit " << e->strValue << "\n"; break;
        case ExprKind::FloatLit: std::cout << "FloatLit " << e->strValue << "\n"; break;
        case ExprKind::StringLit: std::cout << "StringLit \"" << e->strValue << "\"\n"; break;
        case ExprKind::Identifier: std::cout << "Identifier " << e->strValue << "\n"; break;
        case ExprKind::BinaryOp:
            std::cout << "BinaryOp '" << e->op << "'\n";
            printExpr(e->left.get(), d + 1);
            printExpr(e->right.get(), d + 1);
            break;
        case ExprKind::Call:
            std::cout << "Call\n";
            indent(d + 1); std::cout << "callee:\n";
            printExpr(e->callee.get(), d + 2);
            indent(d + 1); std::cout << "args:\n";
            for (auto& a : e->args) printExpr(a.get(), d + 2);
            break;
        case ExprKind::FieldAccess:
            std::cout << "FieldAccess ." << e->field << "\n";
            printExpr(e->target.get(), d + 1);
            break;
        case ExprKind::StructLit:
            std::cout << "StructLit " << e->structName << "\n";
            for (auto& f : e->fields) {
                indent(d + 1); std::cout << f.first << ":\n";
                printExpr(f.second.get(), d + 2);
            }
            break;
        case ExprKind::ArrayLit:
            if (!e->elementTypeName.empty())
                std::cout << "ArrayLit (empty, elementType=" << e->elementTypeName << ")\n";
            else {
                std::cout << "ArrayLit\n";
                for (auto& a : e->args) printExpr(a.get(), d + 1);
            }
            break;
        case ExprKind::ArenaAlloc:
            std::cout << "ArenaAlloc\n";
            indent(d + 1); std::cout << "value:\n";
            printExpr(e->value.get(), d + 2);
            indent(d + 1); std::cout << "arena:\n";
            printExpr(e->arena.get(), d + 2);
            break;
        case ExprKind::Reference:
            std::cout << "Reference\n";
            printExpr(e->value.get(), d + 1);
            break;
        case ExprKind::NoneLit: std::cout << "NoneLit\n"; break;
        case ExprKind::OkLit:
            std::cout << "OkLit\n";
            printExpr(e->value.get(), d + 1);
            break;
        case ExprKind::ErrLit:
            std::cout << "ErrLit\n";
            printExpr(e->value.get(), d + 1);
            break;
        case ExprKind::IsMatch:
            std::cout << "IsMatch " << e->matchKind << "(" << e->bindingName << ")\n";
            printExpr(e->left.get(), d + 1);
            break;
        default: std::cout << "UnaryOp?\n"; break;
    }
}

void printStmt(const Stmt* s, int d);

void printStmtList(const std::vector<std::unique_ptr<Stmt>>& stmts, int d) {
    for (auto& s : stmts) printStmt(s.get(), d);
}

void printStmt(const Stmt* s, int d) {
    indent(d);
    switch (s->kind) {
        case StmtKind::ExprStmt:
            std::cout << "ExprStmt\n";
            printExpr(s->expr.get(), d + 1);
            break;
        case StmtKind::Assign:
            std::cout << "Assign " << s->assignTarget << " =\n";
            printExpr(s->assignValue.get(), d + 1);
            break;
        case StmtKind::ConstAssign:
            std::cout << "ConstAssign " << s->assignTarget << " =\n";
            printExpr(s->assignValue.get(), d + 1);
            break;
        case StmtKind::Return:
            std::cout << "Return\n";
            if (s->expr) printExpr(s->expr.get(), d + 1);
            break;
        case StmtKind::Continue:
            std::cout << "Continue\n";
            break;
        case StmtKind::Break:
            std::cout << "Break\n";
            break;
        case StmtKind::If:
            std::cout << "If\n";
            printExpr(s->expr.get(), d + 1);
            indent(d); std::cout << "then:\n";
            printStmtList(s->body, d + 1);
            if (!s->elseBody.empty()) {
                indent(d); std::cout << "else:\n";
                printStmtList(s->elseBody, d + 1);
            }
            break;
        case StmtKind::While:
            std::cout << "While\n";
            printExpr(s->expr.get(), d + 1);
            indent(d); std::cout << "body:\n";
            printStmtList(s->body, d + 1);
            break;
        case StmtKind::For:
            std::cout << "For " << s->loopVar << " in:\n";
            printExpr(s->iterable.get(), d + 1);
            indent(d); std::cout << "body:\n";
            printStmtList(s->body, d + 1);
            break;
        case StmtKind::Guard:
            std::cout << "Guard condition:\n";
            printExpr(s->guardCondition.get(), d + 1);
            indent(d); std::cout << "then:\n";
            printStmt(s->guardBody.get(), d + 1);
            break;
        case StmtKind::ComptimeDecl:
            std::cout << "ComptimeDecl " << s->comptimeName << " =\n";
            printExpr(s->comptimeValue.get(), d + 1);
            break;
        case StmtKind::UnsafeBlock:
            std::cout << "UnsafeBlock\n";
            printStmtList(s->body, d + 1);
            break;
    }
}

void runTest(const std::string& label, const std::string& src) {
    std::cout << "=== " << label << " ===\n";
    try {
        Lexer lex(src);
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        Program prog = parser.parseProgram();

        for (auto& d : prog.decls) {
            if (d->kind == DeclKind::Function) {
                std::cout << "Function " << d->fn->name << "\n";
                for (auto& p : d->fn->params) {
                    indent(1); std::cout << "param " << p.name << ":\n";
                    printType(p.type.get(), 2);
                }
                indent(1); std::cout << "returns:\n";
                printType(d->fn->returnType.get(), 2);
                indent(1); std::cout << "body:\n";
                printStmtList(d->fn->body, 2);
            } else if (d->kind == DeclKind::Struct) {
                std::cout << "Struct " << d->st->name << "\n";
                for (auto& f : d->st->fields) {
                    indent(1); std::cout << f.name << ":\n";
                    printType(f.type.get(), 2);
                }
            } else if (d->kind == DeclKind::ComptimeGlobal) {
                std::cout << "ComptimeGlobal\n";
                printStmt(d->comptimeGlobal.get(), 1);
            }
        }
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << "\n";
    }
    std::cout << "\n";
}

int main() {
    runTest("struct decl", "Square[ file: i8, rank: i8 ]\n");

    runTest("function with binary expr and return",
        "add(a: i32, b: i32) i32:\n"
        "    return a + b * 2\n"
    );

    runTest("guard + arena alloc + generics + full generateMoves",
        "generateMoves(board: Board, arena: Arena) Result[[]Move, MoveError]:\n"
        "    moves = []Move in arena\n"
        "    for piece in board.pieces:\n"
        "        moves.push(Move[from: piece.square, to: piece.square, promo: none])\n"
        "    moves.isEmpty() -> return Err(MoveError.NoLegalMoves)\n"
        "    return Ok(moves)\n"
    );

    runTest("comptime global", "comptime BOARD_SIZE = 8\n");

    runTest("is-match in guard",
        "check(result: Result[i32, MoveError]) i32:\n"
        "    result is Err(e) -> return 0\n"
        "    return 1\n"
    );

    return 0;
}