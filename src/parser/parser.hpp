#pragma once
#include "ast.hpp"
#include "../lexer/token.hpp"
#include <memory>
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program parseProgram();

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    const Token& peek(int offset = 0) const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& errMsg);
    bool isAtEnd() const;
    void skipNewlines();

    std::unique_ptr<TopLevelDecl> parseTopLevelDecl();
    std::unique_ptr<FunctionDecl> parseExternFunction();
    std::unique_ptr<StaticDecl> parseExternStatic();
    std::unique_ptr<StaticDecl> parseStaticDecl();
    std::unique_ptr<FunctionDecl> parseFunctionDecl();
    std::unique_ptr<StructDecl> parseStructDecl(bool packed = false);
    std::unique_ptr<EnumDecl> parseEnumDecl();
    std::unique_ptr<TypeNode> parseType();
    std::vector<std::unique_ptr<Stmt>> parseBlock();
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseComptimeDecl();
    std::unique_ptr<Stmt> parseUnsafeBlock();
    std::unique_ptr<Stmt> parseAssignmentOrExpr(bool isConst = false);
    std::unique_ptr<Stmt> parseStmtOrGuardTail(std::unique_ptr<Expr> alreadyParsed);
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseIsExpr();
    std::unique_ptr<Expr> parseBinary(int minPrec);
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
    int binaryPrecedence(const std::string& op) const;

    static std::uint64_t parseUnsignedLiteral(const Token& token);
    static std::int64_t parseSignedLiteral(const Token& token);
};
