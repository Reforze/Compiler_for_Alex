#pragma once
#include "../Parser/ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <ostream>

class CodeGen {
public:
    CodeGen();

    // Generate MASM x86 32-bit assembly from the AST
    void generate(ProgramNode& prog, std::ostream& out);

private:
    // Symbol table: variable name -> type string
    std::unordered_map<std::string, std::string> symbols_;

    // Counter for unique labels
    int labelCounter_ = 0;

    std::string newLabel(const std::string& tag);

    // Emit sections
    void emitHeader(std::ostream& out);
    void emitDataSection(std::ostream& out, ProgramNode& prog);
    void emitBuiltins(std::ostream& out);
    void emitCodeSection(std::ostream& out, ProgramNode& prog);
    void emitFooter(std::ostream& out);

    // Collect all variable declarations from AST
    void collectVars(ProgramNode& prog);
    void collectVarsFromStmt(Stmt& stmt);

    // Statement code generation (emits to out)
    void genStmt(Stmt& stmt, std::ostream& out);
    // Expression code generation: result in EAX
    void genExpr(Expr& expr, std::ostream& out);

    // Helper: determine if an expression refers to a char-typed variable
    bool isCharExpr(Expr& expr);
};
