#pragma once
#include "ast.h"
#include "token.h"
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Thrown on the first syntax error. v1 stops at the first error; reporting
// several at once needs "synchronization" (skip ahead to a safe point), which
// is a good follow-up once the basic parser is solid.
struct ParseError : std::runtime_error {
    int line, column;
    ParseError(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line(line), column(column) {}
};

// Recursive-descent parser: one method per grammar rule.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<Program> parseProgram();
    ExprPtr parseExpression();       // public so tests can parse a bare expression

private:
    std::vector<Token> toks;
    size_t cur = 0;

    // --- token helpers ---
    const Token& peek(size_t offset = 0) const;   // clamps to END_OF_FILE
    const Token& previous() const;
    bool check(TokenType t) const;
    bool atEnd() const;
    const Token& advance();                        // never moves past END_OF_FILE
    bool match(TokenType t);                       // consume if it matches
    const Token& expect(TokenType t, const std::string& what);
    void expectTerminator();                       // the 。 that ends a statement
    static bool isTypeToken(TokenType t);
    static std::string describe(const Token& t);   // "'甲'" or "end of file"
    [[noreturn]] void error(const std::string& msg, const Token& at) const;

    template <class T>
    std::unique_ptr<T> make(const Token& at) const {
        auto n = std::make_unique<T>();
        n->line = at.line;
        n->column = at.column;
        return n;
    }

    // --- declarations and statements ---
    Function parseFunction();
    std::unique_ptr<Block> parseBlock();
    StmtPtr parseStatement();
    StmtPtr parseVarDeclCore();       // type name (= expr)?     no terminator
    StmtPtr parseAssignCore();        // name = expr             no terminator
    StmtPtr parseAssignOrIncDec();    // name = expr | name++    no terminator
    ExprPtr parseCondition();         // "(" expression ")"
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parseOutput();
    StmtPtr parseInput();

    // --- expressions, lowest precedence first ---
    ExprPtr parseBinaryLevel(std::initializer_list<TokenType> ops, ExprPtr (Parser::*next)());
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
};