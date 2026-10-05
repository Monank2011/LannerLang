#pragma once
#include "token.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t pos = 0;
    int line = 1;
    int column = 1;
    bool atLineStart = true;

    std::vector<int> indentStack{0};
    std::vector<Token> tokens;

    static const std::unordered_map<std::string, TokenType> keywords;

    char peek(int offset = 0) const;
    char advance();
    bool isAtEnd() const;

    void lexLineStart();
    void skipInlineWhitespace();
    void skipComment();
    void scanToken();
    void scanIdentifierOrKeyword();
    void scanNumber();
    void scanString();
    void handleNewline();
    void closeIndentsAtEOF();
    void addToken(TokenType type, const std::string& lexeme, int startLine, int startColumn);
    [[noreturn]] void error(const std::string& message, int errLine, int errColumn) const;
};
