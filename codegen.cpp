#include "codegen.h"
#include <sstream>
#include <stdexcept>

CodeGen::CodeGen() {}

std::string CodeGen::newLabel(const std::string& tag) {
    return "AL_" + tag + "_" + std::to_string(labelCounter_++);
}

// ─── Variable collection pass ────────────────────────────────────────────────

void CodeGen::collectVars(ProgramNode& prog) {
    for (auto& stmt : prog.statements) {
        collectVarsFromStmt(*stmt);
    }
}

void CodeGen::collectVarsFromStmt(Stmt& stmt) {
    switch (stmt.kind) {
    case StmtKind::VarDecl:
        symbols_[stmt.varName] = stmt.typeName;
        break;
    case StmtKind::Block:
        for (auto& s : stmt.blockStmts) collectVarsFromStmt(*s);
        break;
    case StmtKind::If:
        if (stmt.thenStmt) collectVarsFromStmt(*stmt.thenStmt);
        if (stmt.elseStmt) collectVarsFromStmt(*stmt.elseStmt);
        break;
    case StmtKind::While:
        if (stmt.bodyStmt) collectVarsFromStmt(*stmt.bodyStmt);
        break;
    default:
        break;
    }
}

// ─── Main generate entry point ───────────────────────────────────────────────

void CodeGen::generate(ProgramNode& prog, std::ostream& out) {
    collectVars(prog);
    emitHeader(out);
    emitDataSection(out, prog);
    emitBuiltins(out);
    emitCodeSection(out, prog);
    emitFooter(out);
}

// ─── Header / directives ─────────────────────────────────────────────────────

void CodeGen::emitHeader(std::ostream& out) {
    out <<
        "; AlexLang compiler output\n"
        "; Build:\n"
        ";   ml /c /coff program.asm\n"
        ";   link /subsystem:console program.obj kernel32.lib\n"
        "\n"
        ".486\n"
        ".model flat, stdcall\n"
        "option casemap:none\n"
        "\n"
        "include windows.inc\n"
        "include kernel32.inc\n"
        "includelib kernel32.lib\n"
        "\n";
}

// ─── Data section ─────────────────────────────────────────────────────────────

void CodeGen::emitDataSection(std::ostream& out, ProgramNode& prog) {
    out << ".data\n";
    // Buffers for I/O
    out << "    AL__buf        BYTE 32 DUP(0)\n";
    out << "    AL__bufLen     DWORD 0\n";
    out << "    AL__hStdOut    DWORD 0\n";
    out << "    AL__hStdIn     DWORD 0\n";
    out << "    AL__written    DWORD 0\n";
    out << "    AL__read       DWORD 0\n";
    out << "    AL__newline    BYTE 13, 10, 0\n";
    out << "\n";
    // User variables
    for (auto& [name, type] : symbols_) {
        out << "    al_" << name << "    DWORD 0\n";
    }
    out << "\n";
}

// ─── Built-in procedures ──────────────────────────────────────────────────────

void CodeGen::emitBuiltins(std::ostream& out) {
    out << ".code\n\n";

    // ── AL_PrintInt: print integer in EAX to stdout ──────────────────────────
    out <<
        "; AL_PrintInt: EAX = integer to print\n"
        "AL_PrintInt PROC\n"
        "    push  ebx\n"
        "    push  ecx\n"
        "    push  edx\n"
        "    push  edi\n"
        "    ; handle negative numbers\n"
        "    mov   edi, 0\n"
        "    test  eax, eax\n"
        "    jge   AL_PI_positive\n"
        "    neg   eax\n"
        "    mov   edi, 1\n"
        "AL_PI_positive:\n"
        "    ; convert integer to decimal string in AL__buf (reversed)\n"
        "    lea   ebx, AL__buf\n"
        "    mov   ecx, 0          ; digit count\n"
        "    mov   edx, 10\n"
        "AL_PI_loop:\n"
        "    xor   edx, edx\n"
        "    mov   edx, 10\n"
        "    div   edx             ; EAX=quotient, EDX=remainder\n"
        "    add   dl, '0'\n"
        "    mov   [ebx + ecx], dl\n"
        "    inc   ecx\n"
        "    test  eax, eax\n"
        "    jnz   AL_PI_loop\n"
        "    ; prepend '-' if negative\n"
        "    test  edi, edi\n"
        "    jz    AL_PI_reverse\n"
        "    mov   BYTE PTR [ebx + ecx], '-'\n"
        "    inc   ecx\n"
        "AL_PI_reverse:\n"
        "    ; reverse the string\n"
        "    mov   AL__bufLen, ecx\n"
        "    push  esi\n"
        "    lea   esi, AL__buf\n"
        "    mov   edi, ecx\n"
        "    dec   edi\n"
        "    mov   ecx, AL__bufLen\n"
        "    shr   ecx, 1\n"
        "AL_PI_rev_loop:\n"
        "    test  ecx, ecx\n"
        "    jz    AL_PI_rev_done\n"
        "    mov   al, [esi]\n"
        "    mov   ah, [esi + edi]\n"
        "    mov   [esi], ah\n"
        "    mov   [esi + edi], al\n"
        "    inc   esi\n"
        "    dec   edi\n"
        "    dec   ecx\n"
        "    jmp   AL_PI_rev_loop\n"
        "AL_PI_rev_done:\n"
        "    pop   esi\n"
        "    ; write to stdout\n"
        "    invoke WriteConsoleA, AL__hStdOut, ADDR AL__buf, AL__bufLen, ADDR AL__written, 0\n"
        "    ; write newline\n"
        "    invoke WriteConsoleA, AL__hStdOut, ADDR AL__newline, 2, ADDR AL__written, 0\n"
        "    pop   edi\n"
        "    pop   edx\n"
        "    pop   ecx\n"
        "    pop   ebx\n"
        "    ret\n"
        "AL_PrintInt ENDP\n\n";

    // ── AL_PrintChar: print character in EAX to stdout ───────────────────────
    out <<
        "; AL_PrintChar: EAX = character to print\n"
        "AL_PrintChar PROC\n"
        "    push  ebx\n"
        "    push  ecx\n"
        "    mov   BYTE PTR AL__buf, al\n"
        "    mov   BYTE PTR AL__buf+1, 0\n"
        "    invoke WriteConsoleA, AL__hStdOut, ADDR AL__buf, 1, ADDR AL__written, 0\n"
        "    ; write newline\n"
        "    invoke WriteConsoleA, AL__hStdOut, ADDR AL__newline, 2, ADDR AL__written, 0\n"
        "    pop   ecx\n"
        "    pop   ebx\n"
        "    ret\n"
        "AL_PrintChar ENDP\n\n";

    // ── AL_ReadInt: read integer from stdin into EAX ─────────────────────────
    out <<
        "; AL_ReadInt: returns integer in EAX\n"
        "AL_ReadInt PROC\n"
        "    push  ebx\n"
        "    push  ecx\n"
        "    push  edx\n"
        "    invoke ReadConsoleA, AL__hStdIn, ADDR AL__buf, 31, ADDR AL__read, 0\n"
        "    ; parse integer\n"
        "    lea   ebx, AL__buf\n"
        "    mov   eax, 0\n"
        "    mov   ecx, 0    ; negative flag\n"
        "    mov   al, [ebx]\n"
        "    cmp   al, '-'\n"
        "    jne   AL_RI_loop\n"
        "    mov   ecx, 1\n"
        "    inc   ebx\n"
        "    mov   eax, 0\n"
        "AL_RI_loop:\n"
        "    mov   al, [ebx]\n"
        "    cmp   al, '0'\n"
        "    jl    AL_RI_done\n"
        "    cmp   al, '9'\n"
        "    jg    AL_RI_done\n"
        "    sub   al, '0'\n"
        "    movzx edx, al\n"
        "    push  edx\n"
        "    mov   eax, [esp+4]  ; reload accumulator\n"
        "    imul  eax, 10\n"
        "    pop   edx\n"
        "    add   eax, edx\n"
        "    mov   [esp], eax    ; store back\n"  // trick: use stack
        "    inc   ebx\n"
        "    jmp   AL_RI_loop\n"
        "AL_RI_done:\n"
        "    test  ecx, ecx\n"
        "    jz    AL_RI_positive\n"
        "    neg   eax\n"
        "AL_RI_positive:\n"
        "    pop   edx\n"
        "    pop   ecx\n"
        "    pop   ebx\n"
        "    ret\n"
        "AL_ReadInt ENDP\n\n";

    // ── AL_ReadChar: read one char from stdin into EAX ───────────────────────
    out <<
        "; AL_ReadChar: returns character in EAX (low byte)\n"
        "AL_ReadChar PROC\n"
        "    push  ebx\n"
        "    invoke ReadConsoleA, AL__hStdIn, ADDR AL__buf, 31, ADDR AL__read, 0\n"
        "    movzx eax, BYTE PTR AL__buf\n"
        "    pop   ebx\n"
        "    ret\n"
        "AL_ReadChar ENDP\n\n";
}

// ─── Code section (main procedure) ───────────────────────────────────────────

void CodeGen::emitCodeSection(std::ostream& out, ProgramNode& prog) {
    out << "main PROC\n";
    // Obtain console handles
    out << "    invoke GetStdHandle, STD_OUTPUT_HANDLE\n";
    out << "    mov   AL__hStdOut, eax\n";
    out << "    invoke GetStdHandle, STD_INPUT_HANDLE\n";
    out << "    mov   AL__hStdIn, eax\n\n";

    for (auto& stmt : prog.statements) {
        genStmt(*stmt, out);
    }

    out << "\n    invoke ExitProcess, 0\n";
    out << "main ENDP\n";
}

void CodeGen::emitFooter(std::ostream& out) {
    out << "\nEND main\n";
}

// ─── Statement code gen ───────────────────────────────────────────────────────

void CodeGen::genStmt(Stmt& stmt, std::ostream& out) {
    switch (stmt.kind) {

    case StmtKind::VarDecl: {
        if (stmt.initExpr) {
            genExpr(*stmt.initExpr, out);
            out << "    mov   al_" << stmt.varName << ", eax\n";
        }
        break;
    }

    case StmtKind::Assign: {
        genExpr(*stmt.valueExpr, out);
        out << "    mov   al_" << stmt.varName << ", eax\n";
        break;
    }

    case StmtKind::If: {
        std::string lblElse = newLabel("ELSE");
        std::string lblEnd  = newLabel("ENDIF");

        genExpr(*stmt.condExpr, out);
        out << "    test  eax, eax\n";
        out << "    jz    " << lblElse << "\n";
        genStmt(*stmt.thenStmt, out);
        out << "    jmp   " << lblEnd << "\n";
        out << lblElse << ":\n";
        if (stmt.elseStmt) genStmt(*stmt.elseStmt, out);
        out << lblEnd << ":\n";
        break;
    }

    case StmtKind::While: {
        std::string lblTop  = newLabel("WLOOP");
        std::string lblEnd  = newLabel("WENDL");

        out << lblTop << ":\n";
        genExpr(*stmt.condExpr, out);
        out << "    test  eax, eax\n";
        out << "    jz    " << lblEnd << "\n";
        genStmt(*stmt.bodyStmt, out);
        out << "    jmp   " << lblTop << "\n";
        out << lblEnd << ":\n";
        break;
    }

    case StmtKind::Block: {
        for (auto& s : stmt.blockStmts) genStmt(*s, out);
        break;
    }

    case StmtKind::Print: {
        genExpr(*stmt.valueExpr, out);
        // Decide PrintChar vs PrintInt based on expression type
        if (isCharExpr(*stmt.valueExpr)) {
            out << "    call  AL_PrintChar\n";
        } else {
            out << "    call  AL_PrintInt\n";
        }
        break;
    }

    case StmtKind::Read: {
        std::string varType = "int";
        auto it = symbols_.find(stmt.varName);
        if (it != symbols_.end()) varType = it->second;

        if (varType == "char") {
            out << "    call  AL_ReadChar\n";
        } else {
            out << "    call  AL_ReadInt\n";
        }
        out << "    mov   al_" << stmt.varName << ", eax\n";
        break;
    }

    case StmtKind::ExprStmt: {
        genExpr(*stmt.exprValue, out);
        break;
    }
    }
}

// ─── Expression code gen: result in EAX ──────────────────────────────────────

void CodeGen::genExpr(Expr& expr, std::ostream& out) {
    switch (expr.kind) {

    case ExprKind::IntLit:
        out << "    mov   eax, " << expr.intVal << "\n";
        break;

    case ExprKind::CharLit: {
        int val = static_cast<unsigned char>(expr.charVal);
        out << "    mov   eax, " << val << "\n";
        break;
    }

    case ExprKind::BoolLit:
        out << "    mov   eax, " << (expr.boolVal ? 1 : 0) << "\n";
        break;

    case ExprKind::VarRef:
        out << "    mov   eax, al_" << expr.name << "\n";
        break;

    case ExprKind::Unary: {
        genExpr(*expr.operand, out);
        if (expr.name == "-") {
            out << "    neg   eax\n";
        } else if (expr.name == "!") {
            out << "    test  eax, eax\n";
            std::string lblZero = newLabel("NOT0");
            std::string lblDone = newLabel("NOTD");
            out << "    jz    " << lblZero << "\n";
            out << "    mov   eax, 0\n";
            out << "    jmp   " << lblDone << "\n";
            out << lblZero << ":\n";
            out << "    mov   eax, 1\n";
            out << lblDone << ":\n";
        }
        break;
    }

    case ExprKind::Binary: {
        const std::string& op = expr.name;

        // Short-circuit logical operators
        if (op == "&&") {
            std::string lblFalse = newLabel("ANDF");
            std::string lblDone  = newLabel("ANDD");
            genExpr(*expr.left, out);
            out << "    test  eax, eax\n";
            out << "    jz    " << lblFalse << "\n";
            genExpr(*expr.right, out);
            out << "    test  eax, eax\n";
            out << "    jz    " << lblFalse << "\n";
            out << "    mov   eax, 1\n";
            out << "    jmp   " << lblDone << "\n";
            out << lblFalse << ":\n";
            out << "    mov   eax, 0\n";
            out << lblDone << ":\n";
            return;
        }

        if (op == "||") {
            std::string lblTrue = newLabel("ORR1");
            std::string lblDone = newLabel("ORRD");
            genExpr(*expr.left, out);
            out << "    test  eax, eax\n";
            out << "    jnz   " << lblTrue << "\n";
            genExpr(*expr.right, out);
            out << "    test  eax, eax\n";
            out << "    jnz   " << lblTrue << "\n";
            out << "    mov   eax, 0\n";
            out << "    jmp   " << lblDone << "\n";
            out << lblTrue << ":\n";
            out << "    mov   eax, 1\n";
            out << lblDone << ":\n";
            return;
        }

        // General: evaluate left into EAX, push; evaluate right into EAX
        genExpr(*expr.left, out);
        out << "    push  eax\n";
        genExpr(*expr.right, out);
        out << "    mov   ebx, eax\n";   // right operand in EBX
        out << "    pop   eax\n";        // left operand in EAX

        if (op == "+") {
            out << "    add   eax, ebx\n";
        } else if (op == "-") {
            out << "    sub   eax, ebx\n";
        } else if (op == "*") {
            out << "    imul  eax, ebx\n";
        } else if (op == "/") {
            out << "    cdq\n";
            out << "    idiv  ebx\n";
        } else if (op == "%") {
            out << "    cdq\n";
            out << "    idiv  ebx\n";
            out << "    mov   eax, edx\n"; // remainder
        } else if (op == "&") {
            out << "    and   eax, ebx\n";
        } else if (op == "|") {
            out << "    or    eax, ebx\n";
        } else if (op == "^") {
            out << "    xor   eax, ebx\n";
        } else {
            // Comparison operators: result is 0 or 1
            std::string lblTrue = newLabel("CMP1");
            std::string lblDone = newLabel("CMPD");
            out << "    cmp   eax, ebx\n";
            if      (op == "==") out << "    je    " << lblTrue << "\n";
            else if (op == "!=") out << "    jne   " << lblTrue << "\n";
            else if (op == "<")  out << "    jl    " << lblTrue << "\n";
            else if (op == "<=") out << "    jle   " << lblTrue << "\n";
            else if (op == ">")  out << "    jg    " << lblTrue << "\n";
            else if (op == ">=") out << "    jge   " << lblTrue << "\n";
            out << "    mov   eax, 0\n";
            out << "    jmp   " << lblDone << "\n";
            out << lblTrue << ":\n";
            out << "    mov   eax, 1\n";
            out << lblDone << ":\n";
        }
        break;
    }
    }
}

bool CodeGen::isCharExpr(Expr& expr) {
    if (expr.kind == ExprKind::CharLit) return true;
    if (expr.kind == ExprKind::VarRef) {
        auto it = symbols_.find(expr.name);
        if (it != symbols_.end()) return it->second == "char";
    }
    return false;
}
