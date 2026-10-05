#include "parser.hpp"
#include <charconv>
#include <limits>
#include <stdexcept>
#include <utility>

Parser::Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

const Token& Parser::peek(int offset) const {
    const size_t idx = pos + static_cast<size_t>(offset);
    return idx < tokens.size() ? tokens[idx] : tokens.back();
}

const Token& Parser::advance() {
    if (isAtEnd()) return tokens.back();
    return tokens[pos++];
}

bool Parser::check(TokenType type) const { return peek().type == type; }

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

const Token& Parser::expect(TokenType type, const std::string& errMsg) {
    if (check(type)) return advance();
    throw std::runtime_error("Parse error at " + std::to_string(peek().line) + ":" +
                             std::to_string(peek().column) + ": " + errMsg);
}

bool Parser::isAtEnd() const { return peek().type == TokenType::EOFToken; }

void Parser::skipNewlines() {
    while (match(TokenType::Newline)) {}
}

std::uint64_t Parser::parseUnsignedLiteral(const Token& token) {
    const std::string& s = token.lexeme;
    int base = 10;
    size_t start = 0;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        start = 2;
    } else if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        base = 2;
        start = 2;
    }

    std::uint64_t value = 0;
    const auto* first = s.data() + start;
    const auto* last = s.data() + s.size();
    const auto [ptr, ec] = std::from_chars(first, last, value, base);
    if (ec != std::errc{} || ptr != last) {
        throw std::runtime_error("Invalid integer literal '" + s + "' at " +
                                 std::to_string(token.line) + ":" + std::to_string(token.column));
    }
    return value;
}

std::int64_t Parser::parseSignedLiteral(const Token& token) {
    const std::uint64_t u = parseUnsignedLiteral(token);
    if (u > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        // Negative integer syntax is parsed as unary '-' over a literal. Keeping
        // the magnitude unsigned lets the type checker diagnose overflow later.
        throw std::runtime_error("integer literal magnitude exceeds signed 64-bit range at " +
                                 std::to_string(token.line) + ":" + std::to_string(token.column));
    }
    return static_cast<std::int64_t>(u);
}

Program Parser::parseProgram() {
    Program p;
    skipNewlines();
    while (!isAtEnd()) {
        if (check(TokenType::NewlineDedent) || check(TokenType::NewlineIndent)) {
            throw std::runtime_error("Parse error at " + std::to_string(peek().line) +
                                     ": stray indentation token at top level");
        }
        p.decls.push_back(parseTopLevelDecl());
        skipNewlines();
    }
    return p;
}

std::unique_ptr<TopLevelDecl> Parser::parseTopLevelDecl() {
    auto d = std::make_unique<TopLevelDecl>();
    if (check(TokenType::Comptime)) {
        d->kind = DeclKind::ComptimeGlobal;
        d->comptimeGlobal = parseComptimeDecl();
        return d;
    }
    if (check(TokenType::Extern)) {
        if (peek(1).type == TokenType::Static) {
            d->kind = DeclKind::Static;
            d->staticDecl = parseExternStatic();
        } else {
            d->kind = DeclKind::Function;
            d->fn = parseExternFunction();
        }
        return d;
    }
    if (check(TokenType::Packed)) {
        d->kind = DeclKind::Struct;
        advance();
        d->st = parseStructDecl(true);
        return d;
    }
    if (check(TokenType::Static)) {
        d->kind = DeclKind::Static;
        d->staticDecl = parseStaticDecl();
        return d;
    }
    if (check(TokenType::Enum)) {
        d->kind = DeclKind::Enum;
        d->en = parseEnumDecl();
        return d;
    }
    if (check(TokenType::Identifier) && peek(1).type == TokenType::LBracket) {
        d->kind = DeclKind::Struct;
        d->st = parseStructDecl();
        return d;
    }
    d->kind = DeclKind::Function;
    d->fn = parseFunctionDecl();
    return d;
}

std::unique_ptr<FunctionDecl> Parser::parseExternFunction() {
    auto fn = std::make_unique<FunctionDecl>();
    const Token externTok = advance();
    fn->isExtern = true;
    fn->isUnsafe = match(TokenType::Unsafe);
    fn->line = externTok.line;
    fn->column = externTok.column;
    fn->name = expect(TokenType::Identifier, "expected extern function name").lexeme;
    expect(TokenType::LParen, "expected '(' after extern function name");
    if (!match(TokenType::RParen)) {
        while (true) {
            Param p;
            p.name = expect(TokenType::Identifier, "expected parameter name").lexeme;
            expect(TokenType::Colon, "expected ':' after parameter name");
            p.type = parseType();
            fn->params.push_back(std::move(p));
            if (match(TokenType::RParen)) break;
            expect(TokenType::Comma, "expected ',' between extern parameters");
        }
    }
    fn->returnType = parseType();
    return fn;
}

std::unique_ptr<StaticDecl> Parser::parseExternStatic() {
    auto st = std::make_unique<StaticDecl>();
    const Token externTok = advance();
    st->isExtern = true;
    expect(TokenType::Static, "expected 'static' after extern");
    st->line = externTok.line;
    st->column = externTok.column;
    st->isThreadLocal = match(TokenType::ThreadLocal);
    st->isMutable = match(TokenType::Mut);
    st->isThreadLocal = st->isThreadLocal || match(TokenType::ThreadLocal);
    st->name = expect(TokenType::Identifier, "expected extern static name").lexeme;
    expect(TokenType::Colon, "expected ':' after extern static name");
    st->type = parseType();
    return st;
}

std::unique_ptr<StaticDecl> Parser::parseStaticDecl() {
    auto st = std::make_unique<StaticDecl>();
    const Token staticTok = advance();
    st->line = staticTok.line;
    st->column = staticTok.column;
    st->isThreadLocal = match(TokenType::ThreadLocal);
    st->isMutable = match(TokenType::Mut);
    st->isThreadLocal = st->isThreadLocal || match(TokenType::ThreadLocal);

    const auto parseAttribute = [&]() {
        if (match(TokenType::Align)) {
            expect(TokenType::LParen, "expected '(' after align");
            const Token& a = expect(TokenType::IntLiteral, "expected alignment in bytes");
            const auto value = parseUnsignedLiteral(a);
            if (value == 0 || (value & (value - 1)) != 0) {
                throw std::runtime_error("alignment must be a positive power of two at " +
                                         std::to_string(a.line) + ":" + std::to_string(a.column));
            }
            st->alignment = static_cast<std::uint32_t>(value);
            expect(TokenType::RParen, "expected ')' after alignment");
            return true;
        }
        if (match(TokenType::Section)) {
            st->section = expect(TokenType::StringLiteral, "expected section name string").lexeme;
            return true;
        }
        return false;
    };

    while (parseAttribute()) {}
    st->name = expect(TokenType::Identifier, "expected static name").lexeme;
    while (parseAttribute()) {}
    expect(TokenType::Colon, "expected ':' after static name");
    st->type = parseType();
    expect(TokenType::Equals, "expected '=' in static declaration");
    st->value = parseExpr();
    return st;
}

std::unique_ptr<StructDecl> Parser::parseStructDecl(bool packed) {
    auto st = std::make_unique<StructDecl>();
    st->line = peek().line;
    st->column = peek().column;
    st->isPacked = packed;
    st->name = expect(TokenType::Identifier, "expected struct name").lexeme;
    expect(TokenType::LBracket, "expected '[' after struct name");
    if (match(TokenType::RBracket)) return st;

    while (true) {
        StructField f;
        f.name = expect(TokenType::Identifier, "expected field name").lexeme;
        expect(TokenType::Colon, "expected ':' after field name");
        f.type = parseType();
        st->fields.push_back(std::move(f));
        if (match(TokenType::RBracket)) break;
        expect(TokenType::Comma, "expected ',' between fields");
    }
    return st;
}

std::unique_ptr<EnumDecl> Parser::parseEnumDecl() {
    auto en = std::make_unique<EnumDecl>();
    const Token enumTok = advance();
    en->line = enumTok.line;
    en->column = enumTok.column;
    en->name = expect(TokenType::Identifier, "expected enum name").lexeme;
    expect(TokenType::Colon, "expected ':' after enum name");
    expect(TokenType::Newline, "expected newline before enum body");
    expect(TokenType::NewlineIndent, "expected an indented enum body");

    std::int64_t nextValue = 0;
    skipNewlines();
    while (!check(TokenType::NewlineDedent) && !isAtEnd()) {
        const Token& nameTok = expect(TokenType::Identifier, "expected enum variant name");
        EnumVariant variant;
        variant.name = nameTok.lexeme;
        variant.value = nextValue;
        if (match(TokenType::Equals)) {
            const Token& valueTok = expect(TokenType::IntLiteral, "enum values must be integer literals");
            const std::uint64_t raw = parseUnsignedLiteral(valueTok);
            if (raw > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                throw std::runtime_error("enum value exceeds signed 64-bit range at " +
                                         std::to_string(valueTok.line) + ":" +
                                         std::to_string(valueTok.column));
            }
            variant.value = static_cast<std::int64_t>(raw);
        }
        en->variants.push_back(std::move(variant));
        nextValue = en->variants.back().value + 1;
        skipNewlines();
    }

    expect(TokenType::NewlineDedent, "expected dedent to close enum body");
    return en;
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    auto fn = std::make_unique<FunctionDecl>();
    fn->isUnsafe = match(TokenType::Unsafe);
    fn->line = peek().line;
    fn->column = peek().column;
    fn->name = expect(TokenType::Identifier, "expected function name").lexeme;
    expect(TokenType::LParen, "expected '(' after function name");

    if (!match(TokenType::RParen)) {
        while (true) {
            Param p;
            p.name = expect(TokenType::Identifier, "expected parameter name").lexeme;
            expect(TokenType::Colon, "expected ':' after parameter name");
            p.type = parseType();
            fn->params.push_back(std::move(p));
            if (match(TokenType::RParen)) break;
            expect(TokenType::Comma, "expected ',' between parameters");
        }
    }

    fn->returnType = parseType();
    expect(TokenType::Colon, "expected ':' before function body");
    fn->body = parseBlock();
    return fn;
}

std::unique_ptr<TypeNode> Parser::parseType() {
    auto t = std::make_unique<TypeNode>();
    t->line = peek().line;
    t->column = peek().column;
    if (match(TokenType::Ampersand)) {
        t->isReference = true;
        t->isMutable = match(TokenType::Mut);
    } else if (match(TokenType::Star)) {
        t->isRawPointer = true;
        t->isMutable = match(TokenType::Mut);
    }

    if (check(TokenType::Unsafe)) {
        advance();
        if (!check(TokenType::Fn)) {
            throw std::runtime_error("Parse error at " + std::to_string(peek().line) + ":" +
                                     std::to_string(peek().column) + ": expected 'fn' after 'unsafe' in function-pointer type");
        }
        t->isUnsafeFunction = true;
    }
    if (match(TokenType::Fn)) {
        t->name = "fn";
        expect(TokenType::LParen, "expected '(' after fn in function-pointer type");
        if (!match(TokenType::RParen)) {
            while (true) {
                t->generics.push_back(parseType());
                if (match(TokenType::RParen)) break;
                expect(TokenType::Comma, "expected ',' between function-pointer parameters");
            }
        }
        t->generics.push_back(parseType());
        return t;
    }

    if (match(TokenType::LBracket)) {
        t->isArray = true;
        if (!match(TokenType::RBracket)) {
            if (check(TokenType::IntLiteral)) {
                const Token& sizeTok = advance();
                t->fixedArraySize = parseUnsignedLiteral(sizeTok);
            } else {
                // Comptime expressions are resolved by semantic analysis, which has
                // access to the current lexical comptime environment.
                t->fixedArraySizeExpr = parseExpr();
            }
            expect(TokenType::RBracket, "expected ']' after fixed array size");
        }
    }

    t->name = expect(TokenType::Identifier, "expected type name").lexeme;

    if (match(TokenType::LBracket)) {
        if (check(TokenType::RBracket)) {
            throw std::runtime_error("Parse error at " + std::to_string(peek().line) +
                                     ": empty generic argument list");
        }
        while (true) {
            t->generics.push_back(parseType());
            if (match(TokenType::RBracket)) break;
            expect(TokenType::Comma, "expected ',' in generic type arguments");
        }
    }

    if (match(TokenType::Question)) t->isOptional = true;
    return t;
}

std::vector<std::unique_ptr<Stmt>> Parser::parseBlock() {
    expect(TokenType::Newline, "expected newline before indented block");
    expect(TokenType::NewlineIndent, "expected an indented block");
    std::vector<std::unique_ptr<Stmt>> b;
    skipNewlines();
    while (!check(TokenType::NewlineDedent) && !isAtEnd()) {
        b.push_back(parseStmt());
        skipNewlines();
    }
    expect(TokenType::NewlineDedent, "expected dedent to close block");
    return b;
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (check(TokenType::Unsafe)) return parseUnsafeBlock();
    if (check(TokenType::If)) return parseIfStmt();
    if (check(TokenType::While)) return parseWhileStmt();
    if (check(TokenType::For)) return parseForStmt();
    if (check(TokenType::Comptime)) return parseComptimeDecl();

    if (check(TokenType::Const)) {
        advance();
        return parseAssignmentOrExpr(true);
    }

    if (check(TokenType::Return)) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Return;
        const Token returnTok = advance();
        s->line = returnTok.line; s->column = returnTok.column;
        if (!check(TokenType::Newline) && !check(TokenType::NewlineDedent) && !check(TokenType::EOFToken)) {
            s->expr = parseExpr();
        }
        return s;
    }

    if (check(TokenType::Continue)) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Continue;
        const Token tok = advance();
        s->line = tok.line; s->column = tok.column;
        return s;
    }

    if (check(TokenType::Break)) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Break;
        const Token tok = advance();
        s->line = tok.line; s->column = tok.column;
        return s;
    }

    return parseAssignmentOrExpr(false);
}


std::unique_ptr<Stmt> Parser::parseUnsafeBlock() {
    auto s = std::make_unique<Stmt>();
    const Token tok = advance();
    s->kind = StmtKind::UnsafeBlock;
    s->line = tok.line;
    s->column = tok.column;
    expect(TokenType::Colon, "expected ':' after unsafe");
    s->body = parseBlock();
    return s;
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::If;
    const Token ifTok = advance();
    s->line = ifTok.line; s->column = ifTok.column;
    s->expr = parseExpr();
    expect(TokenType::Colon, "expected ':' after if condition");
    s->body = parseBlock();

    if (match(TokenType::Else)) {
        if (check(TokenType::If)) {
            auto nested = parseIfStmt();
            s->elseBody.push_back(std::move(nested));
        } else {
            expect(TokenType::Colon, "expected ':' after else");
            s->elseBody = parseBlock();
        }
    }
    return s;
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::While;
    const Token whileTok = advance();
    s->line = whileTok.line; s->column = whileTok.column;
    s->expr = parseExpr();
    expect(TokenType::Colon, "expected ':' after while condition");
    s->body = parseBlock();
    return s;
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::For;
    const Token forTok = advance();
    s->line = forTok.line; s->column = forTok.column;
    s->loopVar = expect(TokenType::Identifier, "expected loop variable").lexeme;
    expect(TokenType::In, "expected 'in' in for loop");
    s->iterable = parseExpr();
    expect(TokenType::Colon, "expected ':' before for body");
    s->body = parseBlock();
    return s;
}

std::unique_ptr<Stmt> Parser::parseComptimeDecl() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::ComptimeDecl;
    const Token comptimeTok = advance();
    s->line = comptimeTok.line; s->column = comptimeTok.column;
    s->comptimeName = expect(TokenType::Identifier, "expected name after 'comptime'").lexeme;
    expect(TokenType::Equals, "expected '=' in comptime declaration");
    s->comptimeValue = parseExpr();
    return s;
}

std::unique_ptr<Stmt> Parser::parseAssignmentOrExpr(bool isConst) {
    const int line = peek().line;
    if (isConst && !check(TokenType::Identifier)) {
        throw std::runtime_error("Parse error at " + std::to_string(peek().line) +
                                 ": expected name after 'const'");
    }

    if (check(TokenType::Identifier)) {
        const bool typed = peek(1).type == TokenType::Colon;
        const bool plain = peek(1).type == TokenType::Equals;
        if (plain || typed || isConst) {
            auto s = std::make_unique<Stmt>();
            s->kind = isConst ? StmtKind::ConstAssign : StmtKind::Assign;
            s->line = line;
            s->column = peek().column;
            s->assignTarget = advance().lexeme;
            if (match(TokenType::Colon)) s->declaredType = parseType();
            expect(TokenType::Equals, "expected '=' in assignment");
            s->assignValue = parseExpr();
            return s;
        }
    }

    auto targetOrExpr = parseExpr();
    if (!isConst && match(TokenType::Equals)) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Assign;
        s->line = line;
        s->column = targetOrExpr ? targetOrExpr->column : 1;
        s->expr = std::move(targetOrExpr);
        s->assignValue = parseExpr();
        return s;
    }
    return parseStmtOrGuardTail(std::move(targetOrExpr));
}

std::unique_ptr<Stmt> Parser::parseStmtOrGuardTail(std::unique_ptr<Expr> e) {
    if (match(TokenType::Arrow)) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Guard;
        s->line = e->line;
        s->column = e->column;
        s->guardCondition = std::move(e);
        s->guardBody = parseStmt();
        return s;
    }

    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::ExprStmt;
    s->line = e->line;
    s->column = e->column;
    s->expr = std::move(e);
    return s;
}

std::unique_ptr<Expr> Parser::parseExpr() { return parseIsExpr(); }

std::unique_ptr<Expr> Parser::parseIsExpr() {
    auto left = parseBinary(0);
    if (!match(TokenType::Is)) return left;

    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::IsMatch;
    e->line = left->line;
    e->column = left->column;
    e->left = std::move(left);

    if (check(TokenType::Ok) || check(TokenType::Err)) {
        e->matchKind = advance().lexeme;
    } else {
        throw std::runtime_error("Parse error at " + std::to_string(peek().line) +
                                 ": expected Ok or Err after 'is'");
    }

    expect(TokenType::LParen, "expected '(' after match kind");
    e->bindingName = expect(TokenType::Identifier, "expected binding name").lexeme;
    expect(TokenType::RParen, "expected ')' after binding name");
    return e;
}

int Parser::binaryPrecedence(const std::string& op) const {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=") return 3;
    if (op == "<" || op == ">" || op == "<=" || op == ">=") return 4;
    if (op == "|") return 5;
    if (op == "^") return 6;
    if (op == "&") return 7;
    if (op == "<<" || op == ">>") return 8;
    if (op == "+" || op == "-") return 9;
    if (op == "*" || op == "/" || op == "%") return 10;
    return -1;
}

std::unique_ptr<Expr> Parser::parseBinary(int minPrec) {
    auto left = parseUnary();
    while (true) {
        const int prec = binaryPrecedence(peek().lexeme);
        if (prec < minPrec || prec == -1) break;

        const std::string op = advance().lexeme;
        auto right = parseBinary(prec + 1);
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::BinaryOp;
        e->line = left->line;
        e->column = left->column;
        e->op = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::Ampersand) || check(TokenType::Star) || check(TokenType::Minus) || check(TokenType::Plus) ||
        check(TokenType::Bang) || check(TokenType::Tilde)) {
        const Token tok = advance();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::UnaryOp;
        e->line = tok.line;
        e->column = tok.column;
        e->op = tok.lexeme;
        if (tok.type == TokenType::Ampersand) {
            if (match(TokenType::Mut)) e->op = "&mut";
            else if (check(TokenType::Identifier) && peek().lexeme == "raw") { advance(); e->op = "&raw"; }
        }
        e->value = parseUnary();
        return e;
    }
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (true) {
        if (match(TokenType::Dot)) {
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::FieldAccess;
            e->line = expr->line;
            e->column = expr->column;
            e->target = std::move(expr);
            e->field = expect(TokenType::Identifier, "expected field name after '.'").lexeme;
            expr = std::move(e);
        } else if (match(TokenType::LParen)) {
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::Call;
            e->line = expr->line;
            e->column = expr->column;
            e->callee = std::move(expr);
            if (!match(TokenType::RParen)) {
                while (true) {
                    e->args.push_back(parseExpr());
                    if (match(TokenType::RParen)) break;
                    expect(TokenType::Comma, "expected ',' between arguments");
                }
            }
            expr = std::move(e);
        } else if (match(TokenType::LBracket)) {
            auto e = std::make_unique<Expr>();
            e->line = expr->line;
            e->column = expr->column;
            e->target = std::move(expr);
            std::unique_ptr<Expr> first;
            if (!check(TokenType::Colon)) first = parseExpr();
            if (match(TokenType::Colon)) {
                e->kind = ExprKind::Slice;
                e->args.push_back(std::move(first));
                if (!check(TokenType::RBracket)) e->args.push_back(parseExpr());
                else e->args.push_back(nullptr);
                expect(TokenType::RBracket, "expected ']' after slice");
            } else {
                e->kind = ExprKind::Index;
                if (!first) throw std::runtime_error("Parse error at " + std::to_string(e->line) + ": slice or index expression expected");
                e->args.push_back(std::move(first));
                expect(TokenType::RBracket, "expected ']' after index");
            }
            expr = std::move(e);
        } else if (match(TokenType::In)) {
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::ArenaAlloc;
            e->line = expr->line;
            e->column = expr->column;
            e->value = std::move(expr);
            e->arena = parseExpr();
            expr = std::move(e);
        } else if (match(TokenType::As)) {
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::Cast;
            e->line = expr->line;
            e->column = expr->column;
            e->value = std::move(expr);
            e->castType = parseType();
            expr = std::move(e);
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    const int line = peek().line;

    if (check(TokenType::IntLiteral) || check(TokenType::FloatLiteral)) {
        auto e = std::make_unique<Expr>();
        e->kind = check(TokenType::IntLiteral) ? ExprKind::IntLit : ExprKind::FloatLit;
        e->line = line;
        e->column = peek().column;
        e->strValue = advance().lexeme;
        return e;
    }

    if (check(TokenType::StringLiteral)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::StringLit;
        e->line = line;
        e->column = peek().column;
        e->strValue = advance().lexeme;
        return e;
    }

    if (check(TokenType::True) || check(TokenType::False)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::BoolLit;
        e->line = line;
        e->column = peek().column;
        e->strValue = advance().lexeme;
        return e;
    }

    if (match(TokenType::Null)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::NoneLit;
        e->line = line;
        e->column = peek().column;
        e->strValue = "null";
        return e;
    }

    if (match(TokenType::None)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::NoneLit;
        e->line = line;
        e->column = peek().column;
        return e;
    }

    if (check(TokenType::Ok) || check(TokenType::Err)) {
        const bool ok = check(TokenType::Ok);
        advance();
        expect(TokenType::LParen, "expected '(' after Ok/Err");
        auto e = std::make_unique<Expr>();
        e->kind = ok ? ExprKind::OkLit : ExprKind::ErrLit;
        e->line = line;
        e->column = tokens[pos - 1].column;
        e->value = parseExpr();
        expect(TokenType::RParen, "expected ')' after Ok/Err value");
        return e;
    }

    if (match(TokenType::LBracket)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::ArrayLit;
        e->line = line;
        e->column = peek().column;
        if (match(TokenType::RBracket)) {
            e->elementTypeName = expect(TokenType::Identifier,
                                        "expected element type after '[]'").lexeme;
            return e;
        }
        while (true) {
            e->args.push_back(parseExpr());
            if (match(TokenType::RBracket)) break;
            expect(TokenType::Comma, "expected ',' in array literal");
        }
        return e;
    }

    if (match(TokenType::LParen)) {
        auto e = parseExpr();
        expect(TokenType::RParen, "expected ')' to close expression");
        return e;
    }

    if (check(TokenType::Identifier)) {
        const std::string name = advance().lexeme;
        if (check(TokenType::LBracket) &&
            peek(1).type == TokenType::Identifier && peek(2).type == TokenType::Colon) {
            advance(); // '['
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::StructLit;
            e->line = line;
            e->column = peek().column;
            e->structName = name;
            if (!match(TokenType::RBracket)) {
                while (true) {
                    const std::string field = expect(TokenType::Identifier,
                                                     "expected field name").lexeme;
                    expect(TokenType::Colon, "expected ':' after field name");
                    e->fields.emplace_back(field, parseExpr());
                    if (match(TokenType::RBracket)) break;
                    expect(TokenType::Comma, "expected ',' between fields");
                }
            }
            return e;
        }

        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Identifier;
        e->line = line;
        e->column = peek().column;
        e->strValue = name;
        return e;
    }

    throw std::runtime_error("Parse error at " + std::to_string(peek().line) + ":" +
                             std::to_string(peek().column) + ": unexpected token");
}
