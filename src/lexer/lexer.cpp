#include "lexer.hpp"
#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"comptime", TokenType::Comptime},
    {"in", TokenType::In},
    {"return", TokenType::Return},
    {"is", TokenType::Is},
    {"none", TokenType::None},
    {"null", TokenType::Null},
    {"Ok", TokenType::Ok},
    {"Err", TokenType::Err},
    {"for", TokenType::For},
    {"continue", TokenType::Continue},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"break", TokenType::Break},
    {"const", TokenType::Const},
    {"as", TokenType::As},
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"enum", TokenType::Enum},
    {"mut", TokenType::Mut},
    {"static", TokenType::Static},
    {"extern", TokenType::Extern},
    {"unsafe", TokenType::Unsafe},
    {"packed", TokenType::Packed},
    {"align", TokenType::Align},
    {"section", TokenType::Section},
        {"fn", TokenType::Fn},
        {"threadLocal", TokenType::ThreadLocal},
};

Lexer::Lexer(std::string source) : src(std::move(source)) {}

char Lexer::peek(int offset) const {
    const size_t idx = pos + static_cast<size_t>(offset);
    return idx < src.size() ? src[idx] : '\0';
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = src[pos++];
    if (c == '\r') {
        if (peek() == '\n') ++pos;
        ++line;
        column = 1;
    } else if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return c;
}

bool Lexer::isAtEnd() const { return pos >= src.size(); }

void Lexer::addToken(TokenType type, const std::string& lexeme, int startLine, int startColumn) {
    tokens.push_back(Token{type, lexeme, startLine, startColumn});
}

[[noreturn]] void Lexer::error(const std::string& message, int errLine, int errColumn) const {
    throw std::runtime_error("Lex error at " + std::to_string(errLine) + ":" +
                             std::to_string(errColumn) + ": " + message);
}

void Lexer::skipInlineWhitespace() {
    while (!isAtEnd() && (peek() == ' ' || peek() == '\t')) advance();
}

void Lexer::skipComment() {
    while (!isAtEnd() && peek() != '\n' && peek() != '\r') advance();
}

void Lexer::lexLineStart() {
    if (!atLineStart) return;

    // Blank and comment-only lines must be fully transparent to the token
    // stream: they carry no indentation semantics AND must not emit their
    // own Newline. The Newline that separates the previous real content line
    // from the next one is already emitted by the caller that advanced past
    // that line's own terminator (see the trailing-comment/newline handling
    // elsewhere, which fires once per real content line). Emitting a second
    // Newline here for every blank/comment line in between corrupts the
    // Newline+NewlineIndent (or Newline+NewlineDedent) pairing parseBlock and
    // parseStmt expect -- most visibly when such a line is the very first
    // line of a new block, where it used to produce two Newlines before any
    // NewlineIndent and fail with "expected an indented block". Looping here
    // and only comparing indentation once real content is found (or EOF)
    // fixes blank/comment lines anywhere, not just mid-block.
    while (true) {
        int spaces = 0;
        while (peek() == ' ') {
            ++spaces;
            advance();
        }
        if (peek() == '\t') {
            error("tabs are not allowed for indentation", line, column);
        }

        if (peek() == '\0') return;
        if (peek() == '#') {
            skipComment();
            if (isAtEnd()) return;
            if (peek() == '\r' || peek() == '\n') advance();
            continue;
        }
        if (peek() == '\n' || peek() == '\r') {
            advance();
            continue;
        }

        const int startLine = line;
        const int startColumn = 1;
        if (spaces > indentStack.back()) {
            indentStack.push_back(spaces);
            addToken(TokenType::NewlineIndent, "", startLine, startColumn);
        } else if (spaces < indentStack.back()) {
            while (indentStack.size() > 1 && spaces < indentStack.back()) {
                indentStack.pop_back();
                addToken(TokenType::NewlineDedent, "", startLine, startColumn);
            }
            if (spaces != indentStack.back()) {
                error("inconsistent indentation", startLine, startColumn);
            }
        }
        atLineStart = false;
        return;
    }
}

void Lexer::handleNewline() {
    const int startLine = line;
    const int startColumn = column;
    if (peek() == '\r' || peek() == '\n') advance();
    addToken(TokenType::Newline, "", startLine, startColumn);
    atLineStart = true;
}

void Lexer::scanIdentifierOrKeyword() {
    const int startLine = line;
    const int startColumn = column;
    const size_t start = pos;
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) advance();
    const std::string text = src.substr(start, pos - start);
    const auto it = keywords.find(text);
    addToken(it == keywords.end() ? TokenType::Identifier : it->second, text, startLine, startColumn);
}

// Lanner 3.0.0: `1_000_000`, `0xFF_FF`, `0b1010_0101` digit separators.
static std::string stripSeparators(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    return text;
}

void Lexer::scanNumber() {
    const int startLine = line;
    const int startColumn = column;
    const size_t start = pos;
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
        advance(); advance();
        const size_t digits = pos;
        while (!isAtEnd() && (std::isxdigit(static_cast<unsigned char>(peek())) ||
                              (peek() == '_' && std::isxdigit(static_cast<unsigned char>(peek(1)))))) advance();
        if (pos == digits) error("expected hexadecimal digits after 0x", startLine, startColumn);
        addToken(TokenType::IntLiteral, stripSeparators(src.substr(start, pos - start)), startLine, startColumn);
        return;
    }
    if (peek() == '0' && (peek(1) == 'b' || peek(1) == 'B')) {
        advance(); advance();
        const size_t digits = pos;
        while (!isAtEnd() && (peek() == '0' || peek() == '1' ||
                              (peek() == '_' && (peek(1) == '0' || peek(1) == '1')))) advance();
        if (pos == digits) error("expected binary digits after 0b", startLine, startColumn);
        addToken(TokenType::IntLiteral, stripSeparators(src.substr(start, pos - start)), startLine, startColumn);
        return;
    }
    while (!isAtEnd() && (std::isdigit(static_cast<unsigned char>(peek())) ||
                          (peek() == '_' && std::isdigit(static_cast<unsigned char>(peek(1)))))) advance();
    bool isFloat = false;
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        isFloat = true;
        advance();
        while (!isAtEnd() && (std::isdigit(static_cast<unsigned char>(peek())) ||
                              (peek() == '_' && std::isdigit(static_cast<unsigned char>(peek(1)))))) advance();
    }
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        advance();
        if (peek() == '+' || peek() == '-') advance();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) error("malformed floating-point exponent", line, column);
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    addToken(isFloat ? TokenType::FloatLiteral : TokenType::IntLiteral,
             stripSeparators(src.substr(start, pos - start)), startLine, startColumn);
}

void Lexer::scanString() {
    const int startLine = line;
    const int startColumn = column;
    advance(); // opening quote
    std::string out;
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n' || peek() == '\r') {
            error("newline in string literal", line, column);
        }
        if (peek() != '\\') {
            out.push_back(advance());
            continue;
        }

        advance(); // backslash
        if (isAtEnd()) error("unterminated escape sequence", line, column);
        switch (peek()) {
            case 'n': advance(); out.push_back('\n'); break;
            case 'r': advance(); out.push_back('\r'); break;
            case 't': advance(); out.push_back('\t'); break;
            case '"': advance(); out.push_back('"'); break;
            case '\\': advance(); out.push_back('\\'); break;
            default:
                error("unknown string escape", line, column);
        }
    }
    if (isAtEnd()) error("unterminated string literal", startLine, startColumn);
    advance(); // closing quote
    addToken(TokenType::StringLiteral, out, startLine, startColumn);
}

void Lexer::scanToken() {
    lexLineStart();
    if (isAtEnd()) return;
    if (atLineStart) return;

    skipInlineWhitespace();
    if (isAtEnd()) return;

    if (peek() == '#' ) {
        skipComment();
        if (!isAtEnd()) handleNewline();
        return;
    }
    if (peek() == '\n' || peek() == '\r') {
        handleNewline();
        return;
    }

    const int startLine = line;
    const int startColumn = column;
    const char c = peek();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') { scanIdentifierOrKeyword(); return; }
    if (std::isdigit(static_cast<unsigned char>(c))) { scanNumber(); return; }
    if (c == '"') { scanString(); return; }

    advance();
    switch (c) {
        case '(': addToken(TokenType::LParen, "(", startLine, startColumn); break;
        case ')': addToken(TokenType::RParen, ")", startLine, startColumn); break;
        case '[': addToken(TokenType::LBracket, "[", startLine, startColumn); break;
        case ']': addToken(TokenType::RBracket, "]", startLine, startColumn); break;
        case '{': addToken(TokenType::LBrace, "{", startLine, startColumn); break;
        case '}': addToken(TokenType::RBrace, "}", startLine, startColumn); break;
        case ':': addToken(TokenType::Colon, ":", startLine, startColumn); break;
        case ',': addToken(TokenType::Comma, ",", startLine, startColumn); break;
        case '.':
            if (peek() == '.') {
                advance();
                if (peek() == '=') { advance(); addToken(TokenType::DotDotEq, "..=", startLine, startColumn); }
                else addToken(TokenType::DotDot, "..", startLine, startColumn);
            } else addToken(TokenType::Dot, ".", startLine, startColumn);
            break;
        case '?': addToken(TokenType::Question, "?", startLine, startColumn); break;
        case '+':
            if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "+=", startLine, startColumn); }
            else addToken(TokenType::Plus, "+", startLine, startColumn);
            break;
        case '*':
            if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "*=", startLine, startColumn); }
            else addToken(TokenType::Star, "*", startLine, startColumn);
            break;
        case '/':
            if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "/=", startLine, startColumn); }
            else addToken(TokenType::Slash, "/", startLine, startColumn);
            break;
        case '%':
            if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "%=", startLine, startColumn); }
            else addToken(TokenType::Percent, "%", startLine, startColumn);
            break;
        case '|':
            if (peek() == '|') { advance(); addToken(TokenType::OrOr, "||", startLine, startColumn); }
            else if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "|=", startLine, startColumn); }
            else addToken(TokenType::Pipe, "|", startLine, startColumn);
            break;
        case '^':
            if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "^=", startLine, startColumn); }
            else addToken(TokenType::Caret, "^", startLine, startColumn);
            break;
        case '~': addToken(TokenType::Tilde, "~", startLine, startColumn); break;
        case '&':
            if (peek() == '&') { advance(); addToken(TokenType::AndAnd, "&&", startLine, startColumn); }
            else if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "&=", startLine, startColumn); }
            else addToken(TokenType::Ampersand, "&", startLine, startColumn);
            break;
        case '!':
            if (peek() == '=') { advance(); addToken(TokenType::NotEq, "!=", startLine, startColumn); }
            else addToken(TokenType::Bang, "!", startLine, startColumn);
            break;
        case '=':
            if (peek() == '=') { advance(); addToken(TokenType::EqEq, "==", startLine, startColumn); }
            else addToken(TokenType::Equals, "=", startLine, startColumn);
            break;
        case '<':
            if (peek() == '<') {
                advance();
                if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "<<=", startLine, startColumn); }
                else addToken(TokenType::ShiftLeft, "<<", startLine, startColumn);
            }
            else if (peek() == '=') { advance(); addToken(TokenType::LessEq, "<=", startLine, startColumn); }
            else addToken(TokenType::Less, "<", startLine, startColumn);
            break;
        case '>':
            if (peek() == '>') {
                advance();
                if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, ">>=", startLine, startColumn); }
                else addToken(TokenType::ShiftRight, ">>", startLine, startColumn);
            }
            else if (peek() == '=') { advance(); addToken(TokenType::GreaterEq, ">=", startLine, startColumn); }
            else addToken(TokenType::Greater, ">", startLine, startColumn);
            break;
        case '-':
            if (peek() == '>') { advance(); addToken(TokenType::Arrow, "->", startLine, startColumn); }
            else if (peek() == '=') { advance(); addToken(TokenType::CompoundAssign, "-=", startLine, startColumn); }
            else addToken(TokenType::Minus, "-", startLine, startColumn);
            break;
        default:
            error(std::string("unexpected character '") + c + "'", startLine, startColumn);
    }
}

void Lexer::closeIndentsAtEOF() {
    const int eofLine = line;
    const int eofColumn = column;
    while (indentStack.size() > 1) {
        indentStack.pop_back();
        addToken(TokenType::NewlineDedent, "", eofLine, eofColumn);
    }
}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) scanToken();
    closeIndentsAtEOF();
    addToken(TokenType::EOFToken, "", line, column);
    return tokens;
}
