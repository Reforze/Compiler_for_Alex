#include "parser.h"
#include "../common.h"
#include <stdexcept>

// ─── helpers ─────────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::current() const {
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= tokens_.size()) return tokens_.back(); // Eof
    return tokens_[idx];
}

bool Parser::check(TokenKind k) const {
    return current().kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { ++pos_; return true; }
    return false;
}

Token Parser::advance() {
    Token t = current();
    if (pos_ + 1 < tokens_.size()) ++pos_;
    return t;
}

Token Parser::consume(TokenKind k, const std::string& what) {
    if (!check(k)) {
        addError("expected " + what + " but got '" + current().text + "'");
        // return a dummy token so parsing can limp along
        return Token(k, "", current().line, current().col);
    }
    return advance();
}

void Parser::addError(const std::string& msg) {
    errors.push_back("Parser error at line " + std::to_string(current().line) +
                     ", col " + std::to_string(current().col) + ": " + msg);
}

void Parser::synchronize() {
    // Skip tokens until we find a likely statement boundary
    while (!check(TokenKind::Eof)) {
        TokenKind k = current().kind;
        if (k == TokenKind::Semicolon) { advance(); return; }
        if (k == TokenKind::RBrace)    { return; }
        if (k == TokenKind::KwIf    || k == TokenKind::KwWhile ||
            k == TokenKind::KwInt   || k == TokenKind::KwChar  ||
            k == TokenKind::KwBool  || k == TokenKind::KwPrint ||
            k == TokenKind::KwRead) {
            return;
        }
        advance();
    }
}

// ─── Top-level ────────────────────────────────────────────────────────────────

ProgramNode Parser::parse() {
    ProgramNode prog;
    while (!check(TokenKind::Eof)) {
        try {
            auto stmt = parseStatement();
            if (stmt) prog.statements.push_back(std::move(stmt));
        } catch (const CompilerException&) {
            synchronize();
        }
    }
    return prog;
}

// ─── Statements ──────────────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::parseStatement() {
    int ln = current().line, cl = current().col;

    // type keywords => variable declaration
    if (check(TokenKind::KwInt) || check(TokenKind::KwChar) || check(TokenKind::KwBool)) {
        std::string typeName = current().text;
        advance();
        return parseVarDecl(typeName);
    }

    if (check(TokenKind::KwIf))    return parseIf();
    if (check(TokenKind::KwWhile)) return parseWhile();
    if (check(TokenKind::LBrace))  return parseBlock();
    if (check(TokenKind::KwPrint)) return parsePrint();
    if (check(TokenKind::KwRead))  return parseRead();

    // Identifier: either assignment or expression-statement
    if (check(TokenKind::Ident) && peek(1).kind == TokenKind::Assign) {
        std::string name = current().text;
        advance(); // consume ident
        advance(); // consume =
        return parseAssign(name, ln, cl);
    }

    return parseExprStmt();
}

std::unique_ptr<Stmt> Parser::parseVarDecl(const std::string& typeName) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::VarDecl;
    s->line = current().line; s->col = current().col;
    s->typeName = typeName;

    Token nameT = consume(TokenKind::Ident, "variable name");
    s->varName = nameT.text;
    s->line = nameT.line; s->col = nameT.col;

    if (match(TokenKind::Assign)) {
        s->initExpr = parseExpression();
    }
    consume(TokenKind::Semicolon, "';'");
    return s;
}

std::unique_ptr<Stmt> Parser::parseAssign(const std::string& name, int line, int col) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Assign;
    s->line = line; s->col = col;
    s->varName = name;
    s->valueExpr = parseExpression();
    consume(TokenKind::Semicolon, "';'");
    return s;
}

std::unique_ptr<Stmt> Parser::parseIf() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::If;
    s->line = current().line; s->col = current().col;
    consume(TokenKind::KwIf, "'if'");
    consume(TokenKind::LParen, "'('");
    s->condExpr = parseExpression();
    consume(TokenKind::RParen, "')'");
    s->thenStmt = parseStatement();
    if (check(TokenKind::KwElse)) {
        advance();
        s->elseStmt = parseStatement();
    }
    return s;
}

std::unique_ptr<Stmt> Parser::parseWhile() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::While;
    s->line = current().line; s->col = current().col;
    consume(TokenKind::KwWhile, "'while'");
    consume(TokenKind::LParen, "'('");
    s->condExpr = parseExpression();
    consume(TokenKind::RParen, "')'");
    s->bodyStmt = parseStatement();
    return s;
}

std::unique_ptr<Stmt> Parser::parseBlock() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Block;
    s->line = current().line; s->col = current().col;
    consume(TokenKind::LBrace, "'{'");
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        try {
            auto inner = parseStatement();
            if (inner) s->blockStmts.push_back(std::move(inner));
        } catch (const CompilerException&) {
            synchronize();
        }
    }
    consume(TokenKind::RBrace, "'}'");
    return s;
}

std::unique_ptr<Stmt> Parser::parsePrint() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Print;
    s->line = current().line; s->col = current().col;
    consume(TokenKind::KwPrint, "'print'");
    consume(TokenKind::LParen, "'('");
    s->valueExpr = parseExpression();
    consume(TokenKind::RParen, "')'");
    consume(TokenKind::Semicolon, "';'");
    return s;
}

std::unique_ptr<Stmt> Parser::parseRead() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Read;
    s->line = current().line; s->col = current().col;
    consume(TokenKind::KwRead, "'read'");
    consume(TokenKind::LParen, "'('");
    Token nameT = consume(TokenKind::Ident, "variable name");
    s->varName = nameT.text;
    consume(TokenKind::RParen, "')'");
    consume(TokenKind::Semicolon, "';'");
    return s;
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::ExprStmt;
    s->line = current().line; s->col = current().col;
    s->exprValue = parseExpression();
    consume(TokenKind::Semicolon, "';'");
    return s;
}

// ─── Expressions ─────────────────────────────────────────────────────────────

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseLogicOr();
}

std::unique_ptr<Expr> Parser::parseLogicOr() {
    auto left = parseLogicAnd();
    while (check(TokenKind::OrOr)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseLogicAnd();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicAnd() {
    auto left = parseBitwiseOr();
    while (check(TokenKind::AndAnd)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseBitwiseOr();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseOr() {
    auto left = parseBitwiseXor();
    while (check(TokenKind::Pipe)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseBitwiseXor();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseXor() {
    auto left = parseBitwiseAnd();
    while (check(TokenKind::Caret)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseBitwiseAnd();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseAnd() {
    auto left = parseEquality();
    while (check(TokenKind::Amp)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseEquality();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseRelational();
    while (check(TokenKind::Eq) || check(TokenKind::NotEq)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseRelational();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto left = parseAdditive();
    while (check(TokenKind::Lt) || check(TokenKind::LtEq) ||
           check(TokenKind::Gt) || check(TokenKind::GtEq)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseAdditive();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    auto left = parseTerm();
    while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseTerm();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto left = parseUnary();
    while (check(TokenKind::Star) || check(TokenKind::Slash) || check(TokenKind::Percent)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto right = parseUnary();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary; e->line = ln; e->col = cl;
        e->name = op;
        e->left = std::move(left);
        e->right = std::move(right);
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenKind::Minus) || check(TokenKind::Bang)) {
        int ln = current().line, cl = current().col;
        std::string op = current().text; advance();
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Unary; e->line = ln; e->col = cl;
        e->name = op;
        e->operand = parseUnary();
        return e;
    }
    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    int ln = current().line, cl = current().col;

    if (check(TokenKind::IntLit)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::IntLit; e->line = ln; e->col = cl;
        e->intVal = std::stoi(current().text);
        advance();
        return e;
    }

    if (check(TokenKind::CharLit)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::CharLit; e->line = ln; e->col = cl;
        e->charVal = current().text.empty() ? '\0' : current().text[0];
        advance();
        return e;
    }

    if (check(TokenKind::KwTrue)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::BoolLit; e->line = ln; e->col = cl;
        e->boolVal = true; advance();
        return e;
    }

    if (check(TokenKind::KwFalse)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::BoolLit; e->line = ln; e->col = cl;
        e->boolVal = false; advance();
        return e;
    }

    if (check(TokenKind::Ident)) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::VarRef; e->line = ln; e->col = cl;
        e->name = current().text; advance();
        return e;
    }

    if (check(TokenKind::LParen)) {
        advance(); // consume (
        auto e = parseExpression();
        consume(TokenKind::RParen, "')'");
        return e;
    }

    // Error: unexpected token
    addError("unexpected token '" + current().text + "' in expression");
    // Return a dummy node so we can continue
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::IntLit; e->line = ln; e->col = cl;
    e->intVal = 0;
    if (!check(TokenKind::Eof)) advance();
    return e;
}
