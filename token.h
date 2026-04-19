#pragma once
#include <string>

enum class TokenKind {
    // Literals
    IntLit, CharLit,
    // Identifiers & Keywords
    Ident,
    KwInt, KwChar, KwBool,
    KwTrue, KwFalse,
    KwIf, KwElse,
    KwWhile,
    KwPrint, KwRead,
    // Operators
    Plus, Minus, Star, Slash, Percent,
    Amp,       // &   bitwise AND
    AndAnd,    // &&  logical AND
    Pipe,      // |   bitwise OR
    OrOr,      // ||  logical OR
    Caret,     // ^   bitwise XOR
    Bang,      // !   logical NOT
    Eq,        // ==
    NotEq,     // !=
    Lt,        // <
    LtEq,      // <=
    Gt,        // >
    GtEq,      // >=
    Assign,    // =
    // Punctuation
    Semicolon, Comma,
    LParen, RParen,
    LBrace, RBrace,
    // Special
    Eof
};

struct Token {
    TokenKind kind;
    std::string text;
    int line;
    int col;

    Token() : kind(TokenKind::Eof), text(""), line(0), col(0) {}
    Token(TokenKind k, std::string t, int ln, int cl)
        : kind(k), text(std::move(t)), line(ln), col(cl) {}
};
