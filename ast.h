#pragma once
#include <string>
#include <memory>
#include <vector>

// ─── Expression nodes ───────────────────────────────────────────────────────

enum class ExprKind {
    IntLit,   // integer literal
    CharLit,  // character literal
    BoolLit,  // true / false
    VarRef,   // variable reference
    Binary,   // binary operation  (left op right)
    Unary     // unary operation   (op operand)
};

struct Expr {
    ExprKind kind;
    int line = 0, col = 0;

    // Literal values
    int  intVal  = 0;
    char charVal = 0;
    bool boolVal = false;

    // VarRef: name of variable
    // Binary: operator string ("+", "-", "*", "/", "%", "&", "|", "^", "==", "!=", "<", "<=", ">", ">=", "&&", "||")
    // Unary:  operator string ("-", "!")
    std::string name;

    // Binary children
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    // Unary child
    std::unique_ptr<Expr> operand;
};

// ─── Statement nodes ─────────────────────────────────────────────────────────

enum class StmtKind {
    VarDecl,   // type name [= expr] ;
    Assign,    // name = expr ;
    If,        // if (cond) then [else alt]
    While,     // while (cond) body
    Block,     // { stmts }
    Print,     // print(expr) ;
    Read,      // read(name) ;
    ExprStmt   // expr ;
};

struct Stmt {
    StmtKind kind;
    int line = 0, col = 0;

    std::string typeName;  // VarDecl: "int" | "char" | "bool"
    std::string varName;   // VarDecl name / Assign target / Read target

    std::unique_ptr<Expr> initExpr;   // VarDecl optional initialiser
    std::unique_ptr<Expr> valueExpr;  // Assign RHS / Print argument
    std::unique_ptr<Expr> condExpr;   // If / While condition
    std::unique_ptr<Expr> exprValue;  // ExprStmt expression

    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt;
    std::unique_ptr<Stmt> bodyStmt;

    std::vector<std::unique_ptr<Stmt>> blockStmts;
};

// ─── Top-level program ───────────────────────────────────────────────────────

struct ProgramNode {
    std::vector<std::unique_ptr<Stmt>> statements;
};
