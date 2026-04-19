#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    void analyze(ProgramNode& prog);

    std::vector<std::string> errors;

private:
    // symbol table: name -> type string
    std::unordered_map<std::string, std::string> symbols_;

    void addError(const std::string& msg, int line, int col);

    void analyzeStmt(Stmt& stmt);
    // returns the type string of the expression ("int","char","bool","")
    std::string analyzeExpr(Expr& expr);
};
