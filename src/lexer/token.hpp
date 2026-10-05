#pragma once
#include <string>

enum class TokenType {
    Identifier, IntLiteral, FloatLiteral, StringLiteral,

    Comptime, In, Return, Is, None, Null, Ok, Err, For, Continue, If, Else, While, Break, Const, As, True, False, Enum, Mut, Static, Extern, Unsafe, Packed, Align, Section, Fn, ThreadLocal,

    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Colon, Comma, Dot, Arrow,
    Equals, Question,
    Plus, Minus, Star, Slash, Percent, Pipe, Caret, Tilde, ShiftLeft, ShiftRight, AndAnd, OrOr,
    EqEq, NotEq, Less, Greater, LessEq, GreaterEq,
    Ampersand, Bang,

    Newline, NewlineIndent, NewlineDedent,
    EOFToken
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line = 1;
    int column = 1;
};
