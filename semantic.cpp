#include "semantic.h"
#include <set>

SemanticAnalyzer::SemanticAnalyzer() {}

void SemanticAnalyzer::addError(const std::string& msg, int line, int col) {
    errors.push_back("Semantic error at line " + std::to_string(line) +
                     ", col " + std::to_string(col) + ": " + msg);
}

void SemanticAnalyzer::analyze(ProgramNode& prog) {
    for (auto& stmt : prog.statements) {
        try {
            analyzeStmt(*stmt);
        } catch (...) {
            // continue with next statement
        }
    }
}

void SemanticAnalyzer::analyzeStmt(Stmt& stmt) {
    switch (stmt.kind) {

    case StmtKind::VarDecl: {
        if (symbols_.count(stmt.varName)) {
            addError("variable '" + stmt.varName + "' already declared",
                     stmt.line, stmt.col);
        }
        symbols_[stmt.varName] = stmt.typeName;
        if (stmt.initExpr) {
            std::string initType = analyzeExpr(*stmt.initExpr);
            // Allow assignment from compatible types (bool/int are interchangeable for init)
            if (!initType.empty() && initType != stmt.typeName) {
                // Allow int<->bool assignment silently (common pattern)
                if (!((stmt.typeName == "int"  && initType == "bool") ||
                      (stmt.typeName == "bool" && initType == "int"))) {
                    addError("type mismatch in initialiser: expected '" + stmt.typeName +
                             "', got '" + initType + "'",
                             stmt.initExpr->line, stmt.initExpr->col);
                }
            }
        }
        break;
    }

    case StmtKind::Assign: {
        if (!symbols_.count(stmt.varName)) {
            addError("undeclared variable '" + stmt.varName + "'",
                     stmt.line, stmt.col);
        }
        if (stmt.valueExpr) {
            std::string rhs = analyzeExpr(*stmt.valueExpr);
            if (symbols_.count(stmt.varName) && !rhs.empty()) {
                std::string lhsType = symbols_[stmt.varName];
                if (rhs != lhsType) {
                    if (!((lhsType == "int"  && rhs == "bool") ||
                          (lhsType == "bool" && rhs == "int"))) {
                        addError("type mismatch in assignment: variable '" + stmt.varName +
                                 "' is '" + lhsType + "', got '" + rhs + "'",
                                 stmt.valueExpr->line, stmt.valueExpr->col);
                    }
                }
            }
        }
        break;
    }

    case StmtKind::If: {
        if (stmt.condExpr) analyzeExpr(*stmt.condExpr);
        if (stmt.thenStmt) analyzeStmt(*stmt.thenStmt);
        if (stmt.elseStmt) analyzeStmt(*stmt.elseStmt);
        break;
    }

    case StmtKind::While: {
        if (stmt.condExpr) analyzeExpr(*stmt.condExpr);
        if (stmt.bodyStmt) analyzeStmt(*stmt.bodyStmt);
        break;
    }

    case StmtKind::Block: {
        for (auto& s : stmt.blockStmts) {
            try {
                analyzeStmt(*s);
            } catch (...) {}
        }
        break;
    }

    case StmtKind::Print: {
        if (stmt.valueExpr) analyzeExpr(*stmt.valueExpr);
        break;
    }

    case StmtKind::Read: {
        if (!symbols_.count(stmt.varName)) {
            addError("undeclared variable '" + stmt.varName + "'",
                     stmt.line, stmt.col);
        }
        break;
    }

    case StmtKind::ExprStmt: {
        if (stmt.exprValue) analyzeExpr(*stmt.exprValue);
        break;
    }
    }
}

std::string SemanticAnalyzer::analyzeExpr(Expr& expr) {
    switch (expr.kind) {

    case ExprKind::IntLit:  return "int";
    case ExprKind::CharLit: return "char";
    case ExprKind::BoolLit: return "bool";

    case ExprKind::VarRef: {
        auto it = symbols_.find(expr.name);
        if (it == symbols_.end()) {
            addError("undeclared variable '" + expr.name + "'",
                     expr.line, expr.col);
            return "";
        }
        return it->second;
    }

    case ExprKind::Unary: {
        std::string opType = analyzeExpr(*expr.operand);
        if (expr.name == "-") {
            if (!opType.empty() && opType != "int") {
                addError("unary '-' requires int operand, got '" + opType + "'",
                         expr.line, expr.col);
            }
            return "int";
        }
        if (expr.name == "!") {
            // any type is allowed; result is int (0 or 1)
            return "int";
        }
        return opType;
    }

    case ExprKind::Binary: {
        std::string lType = analyzeExpr(*expr.left);
        std::string rType = analyzeExpr(*expr.right);
        const std::string& op = expr.name;

        // Arithmetic operators
        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (!lType.empty() && lType != "int")
                addError("arithmetic operator '" + op + "' requires int operands, got '" + lType + "'",
                         expr.line, expr.col);
            if (!rType.empty() && rType != "int")
                addError("arithmetic operator '" + op + "' requires int operands, got '" + rType + "'",
                         expr.line, expr.col);
            return "int";
        }

        // Bitwise operators (individual task)
        if (op == "&" || op == "|" || op == "^") {
            if (!lType.empty() && lType != "int")
                addError("bitwise operator '" + op + "' requires int operands, got '" + lType + "'",
                         expr.line, expr.col);
            if (!rType.empty() && rType != "int")
                addError("bitwise operator '" + op + "' requires int operands, got '" + rType + "'",
                         expr.line, expr.col);
            return "int";
        }

        // Relational operators
        if (op == "<" || op == "<=" || op == ">" || op == ">=") {
            if (!lType.empty() && lType != "int")
                addError("relational operator '" + op + "' requires int operands, got '" + lType + "'",
                         expr.line, expr.col);
            if (!rType.empty() && rType != "int")
                addError("relational operator '" + op + "' requires int operands, got '" + rType + "'",
                         expr.line, expr.col);
            return "bool";
        }

        // Equality operators
        if (op == "==" || op == "!=") {
            if (!lType.empty() && !rType.empty() && lType != rType) {
                // Allow bool/int comparison
                if (!((lType == "int" && rType == "bool") ||
                      (lType == "bool" && rType == "int"))) {
                    addError("equality operator '" + op + "': type mismatch '" +
                             lType + "' vs '" + rType + "'",
                             expr.line, expr.col);
                }
            }
            return "bool";
        }

        // Logical operators - any type accepted
        if (op == "&&" || op == "||") {
            return "bool";
        }

        return "";
    }
    }
    return "";
}
