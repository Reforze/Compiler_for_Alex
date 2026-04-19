#pragma once
#include "token.h"
#include "ast.h"
#include <vector>
#include <string>
#include <memory>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    ProgramNode parse();

    std::vector<std::string> errors;

private:
    std::vector<Token> tokens_;
    std::size_t pos_ = 0;

    // ── Token navigation ──────────────────────────────────────────────────────
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    bool check(TokenKind k) const;
    bool match(TokenKind k);
    Token consume(TokenKind k, const std::string& what);
    Token advance();

    // ── Error recovery ────────────────────────────────────────────────────────
    void addError(const std::string& msg);
    void synchronize();

    // ── Statement parsers ─────────────────────────────────────────────────────
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseVarDecl(const std::string& typeName);
    std::unique_ptr<Stmt> parseAssign(const std::string& name, int line, int col);
    std::unique_ptr<Stmt> parseIf();
    std::unique_ptr<Stmt> parseWhile();
    std::unique_ptr<Stmt> parseBlock();
    std::unique_ptr<Stmt> parsePrint();
    std::unique_ptr<Stmt> parseRead();
    std::unique_ptr<Stmt> parseExprStmt();

    // ── Expression parsers (recursive-descent) ────────────────────────────────
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseLogicOr();
    std::unique_ptr<Expr> parseLogicAnd();
    std::unique_ptr<Expr> parseBitwiseOr();
    std::unique_ptr<Expr> parseBitwiseXor();
    std::unique_ptr<Expr> parseBitwiseAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
};
