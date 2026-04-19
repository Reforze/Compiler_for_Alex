#include "lexer.h"
#include "../common.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& source) : src_(source) {}

char Lexer::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= src_.size()) return '\0';
    return src_[idx];
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

void Lexer::addError(const std::string& msg) {
    errors.push_back("[" + std::to_string(line_) + ":" + std::to_string(col_) +
                     "] Лексическая ошибка: " + msg);
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < src_.size()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // single-line comment
            while (pos_ < src_.size() && peek() != '\n') advance();
        } else if (c == '/' && peek(1) == '*') {
            // multi-line comment
            int startLine = line_, startCol = col_;
            advance(); advance(); // consume /*
            bool closed = false;
            while (pos_ < src_.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                errors.push_back("[" + std::to_string(startLine) + ":" + std::to_string(startCol) +
                                 "] Лексическая ошибка: незакрытый блочный комментарий");
            }
        } else {
            break;
        }
    }
}

Token Lexer::readNumber() {
    int startLine = line_, startCol = col_;
    std::string num;
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
        num += advance();
    }
    return Token(TokenKind::IntLit, num, startLine, startCol);
}

Token Lexer::readChar() {
    int startLine = line_, startCol = col_;
    advance(); // consume opening '
    char val = '\0';
    if (pos_ >= src_.size()) {
        addError("незакрытый символьный литерал");
        return Token(TokenKind::CharLit, "", startLine, startCol);
    }
    if (peek() == '\\') {
        advance(); // consume backslash
        if (pos_ >= src_.size()) {
            addError("незакрытая escape-последовательность");
            return Token(TokenKind::CharLit, std::string(1, val), startLine, startCol);
        }
        char esc = advance();
        switch (esc) {
            case 'n':  val = '\n'; break;
            case 't':  val = '\t'; break;
            case 'r':  val = '\r'; break;
            case '\\': val = '\\'; break;
            case '\'': val = '\''; break;
            case '0':  val = '\0'; break;
            default:
                addError(std::string("неизвестная escape-последовательность '\\") + esc + "'");
                val = esc;
        }
    } else {
        val = advance();
    }
    if (pos_ >= src_.size() || peek() != '\'') {
        addError("ожидалась закрывающая ' в символьном литерале");
    } else {
        advance(); // consume closing '
    }
    return Token(TokenKind::CharLit, std::string(1, val), startLine, startCol);
}

Token Lexer::readIdentOrKeyword() {
    int startLine = line_, startCol = col_;
    std::string text;
    while (pos_ < src_.size() &&
           (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        text += advance();
    }
    // Keyword check
    if (text == "int")   return Token(TokenKind::KwInt,   text, startLine, startCol);
    if (text == "char")  return Token(TokenKind::KwChar,  text, startLine, startCol);
    if (text == "bool")  return Token(TokenKind::KwBool,  text, startLine, startCol);
    if (text == "true")  return Token(TokenKind::KwTrue,  text, startLine, startCol);
    if (text == "false") return Token(TokenKind::KwFalse, text, startLine, startCol);
    if (text == "if")    return Token(TokenKind::KwIf,    text, startLine, startCol);
    if (text == "else")  return Token(TokenKind::KwElse,  text, startLine, startCol);
    if (text == "while") return Token(TokenKind::KwWhile, text, startLine, startCol);
    if (text == "print") return Token(TokenKind::KwPrint, text, startLine, startCol);
    if (text == "read")  return Token(TokenKind::KwRead,  text, startLine, startCol);
    return Token(TokenKind::Ident, text, startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();
        if (pos_ >= src_.size()) {
            tokens.emplace_back(TokenKind::Eof, "", line_, col_);
            break;
        }

        int startLine = line_, startCol = col_;
        char c = peek();

        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(readNumber());
            continue;
        }
        if (c == '\'') {
            tokens.push_back(readChar());
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(readIdentOrKeyword());
            continue;
        }

        // Single/double-char operators & punctuation
        advance(); // consume c
        switch (c) {
            case '+': tokens.emplace_back(TokenKind::Plus,      "+", startLine, startCol); break;
            case '-': tokens.emplace_back(TokenKind::Minus,     "-", startLine, startCol); break;
            case '*': tokens.emplace_back(TokenKind::Star,      "*", startLine, startCol); break;
            case '/': tokens.emplace_back(TokenKind::Slash,     "/", startLine, startCol); break;
            case '%': tokens.emplace_back(TokenKind::Percent,   "%", startLine, startCol); break;
            case '^': tokens.emplace_back(TokenKind::Caret,     "^", startLine, startCol); break;
            case ';': tokens.emplace_back(TokenKind::Semicolon, ";", startLine, startCol); break;
            case ',': tokens.emplace_back(TokenKind::Comma,     ",", startLine, startCol); break;
            case '(': tokens.emplace_back(TokenKind::LParen,    "(", startLine, startCol); break;
            case ')': tokens.emplace_back(TokenKind::RParen,    ")", startLine, startCol); break;
            case '{': tokens.emplace_back(TokenKind::LBrace,    "{", startLine, startCol); break;
            case '}': tokens.emplace_back(TokenKind::RBrace,    "}", startLine, startCol); break;

            case '&':
                if (peek() == '&') { advance(); tokens.emplace_back(TokenKind::AndAnd, "&&", startLine, startCol); }
                else               {            tokens.emplace_back(TokenKind::Amp,    "&",  startLine, startCol); }
                break;
            case '|':
                if (peek() == '|') { advance(); tokens.emplace_back(TokenKind::OrOr, "||", startLine, startCol); }
                else               {            tokens.emplace_back(TokenKind::Pipe, "|",  startLine, startCol); }
                break;
            case '!':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenKind::NotEq, "!=", startLine, startCol); }
                else               {            tokens.emplace_back(TokenKind::Bang,  "!",  startLine, startCol); }
                break;
            case '=':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenKind::Eq,     "==", startLine, startCol); }
                else               {            tokens.emplace_back(TokenKind::Assign, "=",  startLine, startCol); }
                break;
            case '<':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenKind::LtEq, "<=", startLine, startCol); }
                else               {            tokens.emplace_back(TokenKind::Lt,   "<",  startLine, startCol); }
                break;
            case '>':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenKind::GtEq, ">=", startLine, startCol); }
                else               {            tokens.emplace_back(TokenKind::Gt,   ">",  startLine, startCol); }
                break;

            default:
                addError(std::string("неожиданный символ '") + c + "'");
                break;
        }
    }

    return tokens;
}
