#include "semantic.h"
#include <set>

SemanticAnalyzer::SemanticAnalyzer() {}

void SemanticAnalyzer::addError(const std::string& msg, int line, int col) {
    errors.push_back("[" + std::to_string(line) + ":" + std::to_string(col) +
                     "] Семантическая ошибка: " + msg);
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
            addError("переменная '" + stmt.varName + "' уже объявлена",
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
                    addError("несовпадение типов в инициализаторе: ожидался '" + stmt.typeName +
                             "', получен '" + initType + "'",
                             stmt.initExpr->line, stmt.initExpr->col);
                }
            }
        }
        break;
    }

    case StmtKind::Assign: {
        if (!symbols_.count(stmt.varName)) {
            addError("необъявленная переменная '" + stmt.varName + "'",
                     stmt.line, stmt.col);
        }
        if (stmt.valueExpr) {
            std::string rhs = analyzeExpr(*stmt.valueExpr);
            if (symbols_.count(stmt.varName) && !rhs.empty()) {
                std::string lhsType = symbols_[stmt.varName];
                if (rhs != lhsType) {
                    if (!((lhsType == "int"  && rhs == "bool") ||
                          (lhsType == "bool" && rhs == "int"))) {
                        addError("несовпадение типов в присваивании: переменная '" + stmt.varName +
                                 "' имеет тип '" + lhsType + "', получен '" + rhs + "'",
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
            addError("необъявленная переменная '" + stmt.varName + "'",
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
            addError("необъявленная переменная '" + expr.name + "'",
                     expr.line, expr.col);
            return "";
        }
        return it->second;
    }

    case ExprKind::Unary: {
        std::string opType = analyzeExpr(*expr.operand);
        if (expr.name == "-") {
            if (!opType.empty() && opType != "int") {
                addError("унарный '-' требует операнд типа int, получен '" + opType + "'",
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
                addError("арифметический оператор '" + op + "' требует операнды int, получен '" + lType + "'",
                         expr.line, expr.col);
            if (!rType.empty() && rType != "int")
                addError("арифметический оператор '" + op + "' требует операнды int, получен '" + rType + "'",
                         expr.line, expr.col);
            return "int";
        }

        // Bitwise operators (individual task)
        if (op == "&" || op == "|" || op == "^") {
            if (!lType.empty() && lType != "int")
                addError("побитовый оператор '" + op + "' требует операнды int, получен '" + lType + "'",
                         expr.line, expr.col);
            if (!rType.empty() && rType != "int")
                addError("побитовый оператор '" + op + "' требует операнды int, получен '" + rType + "'",
                         expr.line, expr.col);
            return "int";
        }

        // Relational operators
        if (op == "<" || op == "<=" || op == ">" || op == ">=") {
            if (!lType.empty() && lType != "int")
                addError("оператор сравнения '" + op + "' требует операнды int, получен '" + lType + "'",
                         expr.line, expr.col);
            if (!rType.empty() && rType != "int")
                addError("оператор сравнения '" + op + "' требует операнды int, получен '" + rType + "'",
                         expr.line, expr.col);
            return "bool";
        }

        // Equality operators
        if (op == "==" || op == "!=") {
            if (!lType.empty() && !rType.empty() && lType != rType) {
                // Allow bool/int comparison
                if (!((lType == "int" && rType == "bool") ||
                      (lType == "bool" && rType == "int"))) {
                    addError("оператор равенства '" + op + "': несовпадение типов '" +
                             lType + "' и '" + rType + "'",
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
