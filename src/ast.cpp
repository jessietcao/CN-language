#include "ast.h"
#include "lexer.h"   // u32_to_utf8
#include <iostream>
#include <sstream>

static std::string u8(const std::u32string& s) { return u32_to_utf8(s); }
static std::string pad(int indent) { return std::string(indent * 2, ' '); }

std::string typeName(TokenType t) {
    switch (t) {
        case TokenType::KW_INT:    return "整数";
        case TokenType::KW_DOUBLE: return "小数";
        case TokenType::KW_CHAR:   return "字";
        case TokenType::KW_STRING: return "字符串";
        case TokenType::KW_BOOL:   return "是非";
        default:                   return "?";
    }
}

std::string opSymbol(TokenType t) {
    switch (t) {
        case TokenType::PLUS:          return "+";
        case TokenType::MINUS:         return "-";
        case TokenType::STAR:          return "*";
        case TokenType::SLASH:         return "/";
        case TokenType::LESS:          return "<";
        case TokenType::GREATER:       return ">";
        case TokenType::LESS_EQUAL:    return "<=";
        case TokenType::GREATER_EQUAL: return ">=";
        case TokenType::EQUAL_EQUAL:   return "==";
        case TokenType::NOT_EQUAL:     return "!=";
        case TokenType::PLUS_PLUS:     return "++";
        case TokenType::MINUS_MINUS:   return "--";
        default:                       return "?";
    }
}

// ---------- Expressions: one-line S-expressions ----------

std::string IntLiteral::str() const    { return std::to_string(value); }
std::string DoubleLiteral::str() const { std::ostringstream o; o << value; return o.str(); }
std::string StringLiteral::str() const { return "\"" + u8(value) + "\""; }
std::string CharLiteral::str() const   { return "'" + u8(value) + "'"; }
std::string BoolLiteral::str() const   { return value ? "真" : "假"; }
std::string VarRef::str() const        { return u8(name); }
std::string UnaryExpr::str() const     { return "(" + opSymbol(op) + " " + operand->str() + ")"; }
std::string BinaryExpr::str() const {
    return "(" + opSymbol(op) + " " + left->str() + " " + right->str() + ")";
}

// ---------- Statements: indented tree ----------

void Block::print(int indent) const {
    std::cout << pad(indent) << "Block\n";
    for (const auto& s : stmts) s->print(indent + 1);
}

void VarDecl::print(int indent) const {
    std::cout << pad(indent) << "VarDecl " << typeName(type) << " " << u8(name);
    if (init) std::cout << " = " << init->str();
    std::cout << "\n";
}

void AssignStmt::print(int indent) const {
    std::cout << pad(indent) << "Assign " << u8(name) << " = " << value->str() << "\n";
}

void IncDecStmt::print(int indent) const {
    std::cout << pad(indent) << "IncDec " << u8(name) << " " << opSymbol(op) << "\n";
}

void IfStmt::print(int indent) const {
    for (size_t i = 0; i < branches.size(); i++) {
        std::cout << pad(indent) << (i == 0 ? "If " : "Elif ") << branches[i].cond->str() << "\n";
        branches[i].block->print(indent + 1);
    }
    if (elseBlock) {
        std::cout << pad(indent) << "Else\n";
        elseBlock->print(indent + 1);
    }
}

void WhileStmt::print(int indent) const {
    std::cout << pad(indent) << "While " << cond->str() << "\n";
    body->print(indent + 1);
}

void ForStmt::print(int indent) const {
    std::cout << pad(indent) << "For\n";
    std::cout << pad(indent + 1) << "Init\n";
    if (init) init->print(indent + 2); else std::cout << pad(indent + 2) << "(none)\n";
    std::cout << pad(indent + 1) << "Cond " << (cond ? cond->str() : "(none)") << "\n";
    std::cout << pad(indent + 1) << "Step\n";
    if (step) step->print(indent + 2); else std::cout << pad(indent + 2) << "(none)\n";
    body->print(indent + 1);
}

void ReturnStmt::print(int indent) const {
    std::cout << pad(indent) << "Return";
    if (value) std::cout << " " << value->str();
    std::cout << "\n";
}

void OutputStmt::print(int indent) const {
    std::cout << pad(indent) << "Output " << value->str() << "\n";
}

void InputStmt::print(int indent) const {
    std::cout << pad(indent) << "Input " << u8(name) << "\n";
}

// ---------- Top level ----------

void Function::print(int indent) const {
    std::cout << pad(indent) << "Function " << typeName(returnType) << " " << u8(name) << "\n";
    body->print(indent + 1);
}

void Program::print(int indent) const {
    std::cout << pad(indent) << "Program\n";
    for (const auto& f : functions) f.print(indent + 1);
}