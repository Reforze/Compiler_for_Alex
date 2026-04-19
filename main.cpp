#include "common.h"
#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

// ─── CLI helpers ──────────────────────────────────────────────────────────────

static void printUsage(const char* argv0) {
    std::cerr << "AlexLang Compiler (alexc)\n"
              << "Usage: " << argv0 << " <source.al> [-o out.asm] [--tokens] [--ast]\n";
}

static std::string tokenKindName(TokenKind k) {
    switch (k) {
    case TokenKind::IntLit:   return "IntLit";
    case TokenKind::CharLit:  return "CharLit";
    case TokenKind::Ident:    return "Ident";
    case TokenKind::KwInt:    return "KwInt";
    case TokenKind::KwChar:   return "KwChar";
    case TokenKind::KwBool:   return "KwBool";
    case TokenKind::KwTrue:   return "KwTrue";
    case TokenKind::KwFalse:  return "KwFalse";
    case TokenKind::KwIf:     return "KwIf";
    case TokenKind::KwElse:   return "KwElse";
    case TokenKind::KwWhile:  return "KwWhile";
    case TokenKind::KwPrint:  return "KwPrint";
    case TokenKind::KwRead:   return "KwRead";
    case TokenKind::Plus:     return "Plus";
    case TokenKind::Minus:    return "Minus";
    case TokenKind::Star:     return "Star";
    case TokenKind::Slash:    return "Slash";
    case TokenKind::Percent:  return "Percent";
    case TokenKind::Amp:      return "Amp";
    case TokenKind::AndAnd:   return "AndAnd";
    case TokenKind::Pipe:     return "Pipe";
    case TokenKind::OrOr:     return "OrOr";
    case TokenKind::Caret:    return "Caret";
    case TokenKind::Bang:     return "Bang";
    case TokenKind::Eq:       return "Eq";
    case TokenKind::NotEq:    return "NotEq";
    case TokenKind::Lt:       return "Lt";
    case TokenKind::LtEq:     return "LtEq";
    case TokenKind::Gt:       return "Gt";
    case TokenKind::GtEq:     return "GtEq";
    case TokenKind::Assign:   return "Assign";
    case TokenKind::Semicolon:return "Semicolon";
    case TokenKind::Comma:    return "Comma";
    case TokenKind::LParen:   return "LParen";
    case TokenKind::RParen:   return "RParen";
    case TokenKind::LBrace:   return "LBrace";
    case TokenKind::RBrace:   return "RBrace";
    case TokenKind::Eof:      return "Eof";
    }
    return "?";
}

static void printTokens(const std::vector<Token>& tokens) {
    std::cout << "=== Tokens ===\n";
    for (const auto& t : tokens) {
        std::cout << "[" << t.line << ":" << t.col << "] "
                  << tokenKindName(t.kind) << " \"" << t.text << "\"\n";
    }
    std::cout << "\n";
}

// Simple AST printer
static void printExpr(const Expr& e, int indent) {
    std::string pad(indent * 2, ' ');
    switch (e.kind) {
    case ExprKind::IntLit:
        std::cout << pad << "IntLit(" << e.intVal << ")\n"; break;
    case ExprKind::CharLit:
        std::cout << pad << "CharLit('" << e.charVal << "')\n"; break;
    case ExprKind::BoolLit:
        std::cout << pad << "BoolLit(" << (e.boolVal ? "true" : "false") << ")\n"; break;
    case ExprKind::VarRef:
        std::cout << pad << "VarRef(" << e.name << ")\n"; break;
    case ExprKind::Binary:
        std::cout << pad << "Binary(" << e.name << ")\n";
        if (e.left)  printExpr(*e.left,  indent + 1);
        if (e.right) printExpr(*e.right, indent + 1);
        break;
    case ExprKind::Unary:
        std::cout << pad << "Unary(" << e.name << ")\n";
        if (e.operand) printExpr(*e.operand, indent + 1);
        break;
    }
}

static void printStmt(const Stmt& s, int indent) {
    std::string pad(indent * 2, ' ');
    switch (s.kind) {
    case StmtKind::VarDecl:
        std::cout << pad << "VarDecl(" << s.typeName << " " << s.varName << ")\n";
        if (s.initExpr) printExpr(*s.initExpr, indent + 1);
        break;
    case StmtKind::Assign:
        std::cout << pad << "Assign(" << s.varName << ")\n";
        if (s.valueExpr) printExpr(*s.valueExpr, indent + 1);
        break;
    case StmtKind::If:
        std::cout << pad << "If\n";
        if (s.condExpr)  printExpr(*s.condExpr,  indent + 1);
        if (s.thenStmt)  printStmt(*s.thenStmt,  indent + 1);
        if (s.elseStmt) { std::cout << pad << "Else\n"; printStmt(*s.elseStmt, indent + 1); }
        break;
    case StmtKind::While:
        std::cout << pad << "While\n";
        if (s.condExpr) printExpr(*s.condExpr, indent + 1);
        if (s.bodyStmt) printStmt(*s.bodyStmt, indent + 1);
        break;
    case StmtKind::Block:
        std::cout << pad << "Block\n";
        for (const auto& bs : s.blockStmts) printStmt(*bs, indent + 1);
        break;
    case StmtKind::Print:
        std::cout << pad << "Print\n";
        if (s.valueExpr) printExpr(*s.valueExpr, indent + 1);
        break;
    case StmtKind::Read:
        std::cout << pad << "Read(" << s.varName << ")\n"; break;
    case StmtKind::ExprStmt:
        std::cout << pad << "ExprStmt\n";
        if (s.exprValue) printExpr(*s.exprValue, indent + 1);
        break;
    }
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string sourceFile;
    std::string outputFile;
    bool showTokens = false;
    bool showAst    = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--tokens") {
            showTokens = true;
        } else if (arg == "--ast") {
            showAst = true;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg[0] != '-') {
            sourceFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (sourceFile.empty()) {
        std::cerr << "Error: no source file specified.\n";
        printUsage(argv[0]);
        return 1;
    }

    // Default output file: replace extension with .asm
    if (outputFile.empty()) {
        outputFile = sourceFile;
        auto dot = outputFile.rfind('.');
        if (dot != std::string::npos) outputFile = outputFile.substr(0, dot);
        outputFile += ".asm";
    }

    // ── Read source ──────────────────────────────────────────────────────────
    std::ifstream fin(sourceFile);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open source file '" << sourceFile << "'\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(fin)),
                        std::istreambuf_iterator<char>());
    fin.close();

    std::vector<std::string> allErrors;

    // ── Lex ──────────────────────────────────────────────────────────────────
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    allErrors.insert(allErrors.end(), lexer.errors.begin(), lexer.errors.end());

    if (showTokens) printTokens(tokens);

    // ── Parse ────────────────────────────────────────────────────────────────
    Parser parser(std::move(tokens));
    ProgramNode prog = parser.parse();
    allErrors.insert(allErrors.end(), parser.errors.begin(), parser.errors.end());

    if (showAst) {
        std::cout << "=== AST ===\n";
        for (const auto& stmt : prog.statements) printStmt(*stmt, 0);
        std::cout << "\n";
    }

    // ── Semantic analysis ────────────────────────────────────────────────────
    SemanticAnalyzer sem;
    sem.analyze(prog);
    allErrors.insert(allErrors.end(), sem.errors.begin(), sem.errors.end());

    // ── Report all errors ────────────────────────────────────────────────────
    if (!allErrors.empty()) {
        std::cerr << "\nFound " << allErrors.size() << " error(s):\n";
        for (const auto& err : allErrors) {
            std::cerr << "  " << err << "\n";
        }
        return 1;
    }

    // ── Code generation ──────────────────────────────────────────────────────
    std::ofstream fout(outputFile);
    if (!fout.is_open()) {
        std::cerr << "Error: cannot open output file '" << outputFile << "'\n";
        return 1;
    }

    CodeGen cg;
    cg.generate(prog, fout);
    fout.close();

    std::cout << "Compiled '" << sourceFile << "' -> '" << outputFile << "'\n";
    return 0;
}
