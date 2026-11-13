// program     → function* EOF
// function    → type (IDENTIFIER | 主函数) "(" ")" block
// block       → "{" statement* "}"
// statement   → varDecl | assignStmt | incStmt | ifStmt | whileStmt | forStmt
//             | returnStmt | outputStmt | inputStmt | block

// varDecl     → type IDENTIFIER ("=" expression)? "。"
// assignStmt  → IDENTIFIER "=" expression "。"
// incStmt     → IDENTIFIER ("++" | "--") "。"
// ifStmt      → 如果 "(" expression ")" block
//               ( 否则 "(" expression ")" block )*
//               ( 其他 block )?
// whileStmt   → 当 "(" expression ")" block
// forStmt     → 循环 "(" forInit? ";" expression? ";" forStep? ")" block
// forInit     → type IDENTIFIER ("=" expression)?  |  IDENTIFIER "=" expression
// forStep     → IDENTIFIER ("++" | "--")  |  IDENTIFIER "=" expression
// returnStmt  → 返回 expression? "。"
// outputStmt  → 输出 "(" expression ")" "。"
// inputStmt   → 输入 "(" IDENTIFIER ")" "。"

// expression  → equality
// equality    → comparison (("==" | "!=") comparison)*
// comparison  → term (("<" | "<=" | ">" | ">=") term)*
// term        → factor (("+" | "-") factor)*
// factor      → unary (("*" | "/") unary)*
// unary       → "-" unary | primary
// primary     → INT | DOUBLE | STRING | CHAR | 真 | 假 | IDENTIFIER | "(" expression ")"
#pragma once
#include "token.h"
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------
// AST for the language. Plain structs owned through unique_ptr.
//
//  - Expr::str()     one-line S-expression, e.g. (+ 1 (* 2 3)).
//                    Handy for tests: precedence bugs show up immediately.
//  - Stmt::print(n)  indented tree, for eyeballing whole programs.
//
// Later passes (type checker, codegen) can switch on the node type with
// dynamic_cast, or you can add a visitor once you know what they need.
// ---------------------------------------------------------------

// Source names for a type keyword / an operator token, for printing.
std::string typeName(TokenType t);
std::string opSymbol(TokenType t);

// ---------- Expressions ----------

struct Expr {
    int line = 0, column = 0;
    virtual ~Expr() = default;
    virtual std::string str() const = 0;
};
using ExprPtr = std::unique_ptr<Expr>;

struct IntLiteral : Expr {
    long long value = 0;
    std::string str() const override;
};
struct DoubleLiteral : Expr {
    double value = 0;
    std::string str() const override;
};
struct StringLiteral : Expr {
    std::u32string value;
    std::string str() const override;
};
struct CharLiteral : Expr {
    std::u32string value;
    std::string str() const override;
};
struct BoolLiteral : Expr {
    bool value = false;
    std::string str() const override;
};
struct VarRef : Expr {
    std::u32string name;
    std::string str() const override;
};
struct UnaryExpr : Expr {
    TokenType op = TokenType::MINUS;
    ExprPtr operand;
    std::string str() const override;
};
struct BinaryExpr : Expr {
    TokenType op = TokenType::PLUS;
    ExprPtr left, right;
    std::string str() const override;
};

// ---------- Statements ----------

struct Stmt {
    int line = 0, column = 0;
    virtual ~Stmt() = default;
    virtual void print(int indent) const = 0;
};
using StmtPtr = std::unique_ptr<Stmt>;

struct Block : Stmt {
    std::vector<StmtPtr> stmts;
    void print(int indent) const override;
};

struct VarDecl : Stmt {            // 整数 甲=5。
    TokenType type = TokenType::KW_INT;
    std::u32string name;
    ExprPtr init;                  // may be null
    void print(int indent) const override;
};

struct AssignStmt : Stmt {         // 甲=5。
    std::u32string name;
    ExprPtr value;
    void print(int indent) const override;
};

struct IncDecStmt : Stmt {         // 甲++。   (a statement, not an expression)
    std::u32string name;
    TokenType op = TokenType::PLUS_PLUS;
    void print(int indent) const override;
};

struct IfBranch {                  // one 如果 or 否则 arm
    ExprPtr cond;
    std::unique_ptr<Block> block;
};
struct IfStmt : Stmt {             // 如果 {} 否则 () {} ... 其他 {}
    std::vector<IfBranch> branches;        // first is 如果, rest are 否则
    std::unique_ptr<Block> elseBlock;      // 其他, may be null
    void print(int indent) const override;
};

struct WhileStmt : Stmt {          // 当 (cond) {}
    ExprPtr cond;
    std::unique_ptr<Block> body;
    void print(int indent) const override;
};

struct ForStmt : Stmt {            // 循环 (init; cond; step) {}
    StmtPtr init;                  // VarDecl or AssignStmt, may be null
    ExprPtr cond;                  // may be null (means "forever")
    StmtPtr step;                  // AssignStmt or IncDecStmt, may be null
    std::unique_ptr<Block> body;
    void print(int indent) const override;
};

struct ReturnStmt : Stmt {
    ExprPtr value;                 // may be null
    void print(int indent) const override;
};
struct OutputStmt : Stmt {
    ExprPtr value;
    void print(int indent) const override;
};
struct InputStmt : Stmt {
    std::u32string name;
    void print(int indent) const override;
};

// ---------- Top level ----------

struct Function {
    int line = 0, column = 0;
    TokenType returnType = TokenType::KW_INT;
    std::u32string name;           // "主函数" for the entry point
    std::unique_ptr<Block> body;
    void print(int indent) const;
};

struct Program {
    std::vector<Function> functions;
    void print(int indent = 0) const;
};