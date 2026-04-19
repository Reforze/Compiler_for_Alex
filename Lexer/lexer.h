#pragma once
#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();

    std::vector<std::string> errors;

private:
    std::string src_;
    std::size_t pos_ = 0;
    int line_ = 1, col_ = 1;

    char peek(int offset = 0) const;
    char advance();

    void skipWhitespaceAndComments();

    Token readNumber();
    Token readChar();
    Token readIdentOrKeyword();

    void addError(const std::string& msg);
};
