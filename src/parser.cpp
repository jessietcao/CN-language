#include "parser.h"
#include "lexer.h"   // u32_to_utf8
#include <stdexcept>
#include <string>

// ===============================================================
// Grammar (the spec this file implements)
//
// program     → function* EOF
// function    → type (IDENTIFIER | 主函数) "(" ")" block
// block       → "{" statement* "}"
// statement   → varDecl | assignStmt | incStmt | ifStmt | whileStmt | forStmt
//             | returnStmt | outputStmt | inputStmt | block
//
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
//
// expression  → equality
// equality    → comparison (("==" | "!=") comparison)*
// comparison  → term (("<" | "<=" | ">" | ">=") term)*
// term        → factor (("+" | "-") factor)*
// factor      → unary (("*" | "/") unary)*
// unary       → "-" unary | primary
// primary     → INT | DOUBLE | STRING | CHAR | 真 | 假 | IDENTIFIER | "(" expression ")"
// ===============================================================

using T = TokenType;

static std::string u8(const std::u32string& s) { return u32_to_utf8(s); }

// ---------- token helpers ----------

Parser::Parser(std::vector<Token> tokens) : toks(std::move(tokens)) {
    // Guarantee the stream ends in END_OF_FILE so peek() is always safe.
    if (toks.empty() || toks.back().type != T::END_OF_FILE) {
        int line = toks.empty() ? 1 : toks.back().line;
        int col = toks.empty() ? 1 : toks.back().column;
        toks.push_back(Token{T::END_OF_FILE, U"", line, col});
    }
}

const Token& Parser::peek(size_t offset) const {
    size_t i = cur + offset;
    if (i >= toks.size()) i = toks.size() - 1;
    return toks[i];
}

const Token& Parser::previous() const {
    return toks[cur == 0 ? 0 : cur - 1];
}

bool Parser::check(TokenType t) const { return peek().type == t; }
bool Parser::atEnd() const { return check(T::END_OF_FILE); }

const Token& Parser::advance() {
    if (!atEnd()) cur++;
    return previous();
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

std::string Parser::describe(const Token& t) {
    switch (t.type) {
        case T::END_OF_FILE:    return "end of file";
        case T::STRING_LITERAL: return "\"" + u8(t.text) + "\"";
        case T::CHAR_LITERAL:   return "'" + u8(t.text) + "'";
        default:                return "'" + u8(t.text) + "'";
    }
}

void Parser::error(const std::string& msg, const Token& at) const {
    throw ParseError(msg, at.line, at.column);
}

const Token& Parser::expect(TokenType t, const std::string& what) {
    if (check(t)) return advance();
    error("expected " + what + ", found " + describe(peek()), peek());
}

// A missing 。 is reported at the END of the statement (the previous token),
// not at whatever token happens to come next, which may be on another line.
void Parser::expectTerminator() {
    if (check(T::PERIOD)) { advance(); return; }
    error("expected '。' to end this statement", previous());
}

bool Parser::isTypeToken(TokenType t) {
    return t == T::KW_INT || t == T::KW_DOUBLE || t == T::KW_CHAR ||
           t == T::KW_STRING || t == T::KW_BOOL;
}

// ---------- program / function / block ----------

std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();
    while (!atEnd()) prog->functions.push_back(parseFunction());
    return prog;
}

Function Parser::parseFunction() {
    const Token& start = peek();
    if (!isTypeToken(start.type)) {
        error("expected a function to start with its return type (整数, 小数, 字, 字符串 or 是非), found "
              + describe(start), start);
    }
    Function f;
    f.line = start.line;
    f.column = start.column;
    f.returnType = advance().type;

    const Token& nameTok = peek();
    if (nameTok.type != T::IDENTIFIER && nameTok.type != T::KW_MAIN) {
        error("expected a function name, found " + describe(nameTok), nameTok);
    }
    f.name = advance().text;

    expect(T::LPAREN, "'('");
    expect(T::RPAREN, "')' (function parameters are not supported yet)");
    f.body = parseBlock();
    return f;
}

std::unique_ptr<Block> Parser::parseBlock() {
    const Token& open = expect(T::LBRACE, "'{'");
    auto block = make<Block>(open);
    while (!check(T::RBRACE) && !atEnd()) {
        block->stmts.push_back(parseStatement());
    }
    if (atEnd()) error("this '{' is never closed (missing '}')", open);
    advance();  // the '}'
    return block;
}

// ---------- statements ----------

StmtPtr Parser::parseStatement() {
    const Token& t = peek();

    if (isTypeToken(t.type)) {
        StmtPtr s = parseVarDeclCore();
        expectTerminator();
        return s;
    }

    switch (t.type) {
        case T::KW_IF:     return parseIf();
        case T::KW_WHILE:  return parseWhile();
        case T::KW_FOR:    return parseFor();
        case T::KW_RETURN: return parseReturn();
        case T::KW_OUTPUT: return parseOutput();
        case T::KW_INPUT:  return parseInput();
        case T::LBRACE:    return parseBlock();
        case T::IDENTIFIER: {
            StmtPtr s = parseAssignOrIncDec();
            expectTerminator();
            return s;
        }
        case T::KW_ELIF:
        case T::KW_ELSE:
            error(describe(t) + " has no matching '如果' before it", t);
        default:
            error("expected a statement, found " + describe(t), t);
    }
}

StmtPtr Parser::parseVarDeclCore() {
    const Token& typeTok = advance();
    auto n = make<VarDecl>(typeTok);
    n->type = typeTok.type;
    n->name = expect(T::IDENTIFIER, "a variable name").text;
    if (match(T::ASSIGN)) n->init = parseExpression();
    return n;
}

StmtPtr Parser::parseAssignCore() {
    const Token& id = expect(T::IDENTIFIER, "a variable name");
    auto n = make<AssignStmt>(id);
    n->name = id.text;
    expect(T::ASSIGN, "'='");
    n->value = parseExpression();
    return n;
}

StmtPtr Parser::parseAssignOrIncDec() {
    const Token& id = expect(T::IDENTIFIER, "a variable name");
    if (check(T::ASSIGN)) {
        advance();
        auto n = make<AssignStmt>(id);
        n->name = id.text;
        n->value = parseExpression();
        return n;
    }
    if (check(T::PLUS_PLUS) || check(T::MINUS_MINUS)) {
        auto n = make<IncDecStmt>(id);
        n->name = id.text;
        n->op = advance().type;
        return n;
    }
    error("expected '=' or '++'/'--' after " + describe(id) + ", found " + describe(peek()), peek());
}

ExprPtr Parser::parseCondition() {
    expect(T::LPAREN, "'(' before the condition");
    ExprPtr e = parseExpression();
    expect(T::RPAREN, "')' after the condition");
    return e;
}

StmtPtr Parser::parseIf() {
    const Token& kw = advance();  // 如果
    auto node = make<IfStmt>(kw);

    IfBranch first;
    first.cond = parseCondition();
    first.block = parseBlock();
    node->branches.push_back(std::move(first));

    while (match(T::KW_ELIF)) {
        if (check(T::LBRACE)) {
            error("'否则' needs a condition in parentheses; use '其他' for the final branch", previous());
        }
        IfBranch b;
        b.cond = parseCondition();
        b.block = parseBlock();
        node->branches.push_back(std::move(b));
    }

    if (match(T::KW_ELSE)) node->elseBlock = parseBlock();
    return node;
}

StmtPtr Parser::parseWhile() {
    const Token& kw = advance();  // 当
    auto node = make<WhileStmt>(kw);
    node->cond = parseCondition();
    node->body = parseBlock();
    return node;
}

StmtPtr Parser::parseFor() {
    const Token& kw = advance();  // 循环
    auto node = make<ForStmt>(kw);
    expect(T::LPAREN, "'(' after '循环'");

    if (!check(T::SEMICOLON)) {
        node->init = isTypeToken(peek().type) ? parseVarDeclCore() : parseAssignCore();
    }
    expect(T::SEMICOLON, "';' after the loop initializer");

    if (!check(T::SEMICOLON)) node->cond = parseExpression();
    expect(T::SEMICOLON, "';' after the loop condition");

    if (!check(T::RPAREN)) node->step = parseAssignOrIncDec();
    expect(T::RPAREN, "')' to close the loop header");

    node->body = parseBlock();
    return node;
}

StmtPtr Parser::parseReturn() {
    const Token& kw = advance();  // 返回
    auto node = make<ReturnStmt>(kw);
    if (!check(T::PERIOD)) node->value = parseExpression();
    expectTerminator();
    return node;
}

StmtPtr Parser::parseOutput() {
    const Token& kw = advance();  // 输出
    auto node = make<OutputStmt>(kw);
    expect(T::LPAREN, "'(' after '输出'");
    node->value = parseExpression();
    expect(T::RPAREN, "')'");
    expectTerminator();
    return node;
}

StmtPtr Parser::parseInput() {
    const Token& kw = advance();  // 输入
    auto node = make<InputStmt>(kw);
    expect(T::LPAREN, "'(' after '输入'");
    node->name = expect(T::IDENTIFIER, "a variable name").text;
    expect(T::RPAREN, "')'");
    expectTerminator();
    return node;
}

// ---------- expressions ----------

ExprPtr Parser::parseExpression() { return parseEquality(); }

// One precedence level: operand (op operand)*, left-associative.
// `next` is the rule for the next-tighter level (a pointer to a member function).
ExprPtr Parser::parseBinaryLevel(std::initializer_list<TokenType> ops, ExprPtr (Parser::*next)()) {
    ExprPtr left = (this->*next)();
    while (true) {
        bool isOp = false;
        for (TokenType op : ops) {
            if (check(op)) { isOp = true; break; }
        }
        if (!isOp) break;

        const Token& opTok = advance();
        ExprPtr right = (this->*next)();
        auto node = make<BinaryExpr>(opTok);
        node->op = opTok.type;
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

ExprPtr Parser::parseEquality()   { return parseBinaryLevel({T::EQUAL_EQUAL, T::NOT_EQUAL}, &Parser::parseComparison); }
ExprPtr Parser::parseComparison() { return parseBinaryLevel({T::LESS, T::LESS_EQUAL, T::GREATER, T::GREATER_EQUAL}, &Parser::parseTerm); }
ExprPtr Parser::parseTerm()       { return parseBinaryLevel({T::PLUS, T::MINUS}, &Parser::parseFactor); }
ExprPtr Parser::parseFactor()     { return parseBinaryLevel({T::STAR, T::SLASH}, &Parser::parseUnary); }

ExprPtr Parser::parseUnary() {
    if (check(T::MINUS)) {
        const Token& op = advance();
        auto node = make<UnaryExpr>(op);
        node->op = T::MINUS;
        node->operand = parseUnary();
        return node;
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    const Token& t = peek();
    switch (t.type) {
        case T::INT_LITERAL: {
            advance();
            auto n = make<IntLiteral>(t);
            try {
                n->value = std::stoll(u8(t.text));
            } catch (const std::out_of_range&) {
                error("integer literal is too large", t);
            }
            return n;
        }
        case T::DOUBLE_LITERAL: {
            advance();
            auto n = make<DoubleLiteral>(t);
            n->value = std::stod(u8(t.text));
            return n;
        }
        case T::STRING_LITERAL: {
            advance();
            auto n = make<StringLiteral>(t);
            n->value = t.text;
            return n;
        }
        case T::CHAR_LITERAL: {
            advance();
            auto n = make<CharLiteral>(t);
            n->value = t.text;
            return n;
        }
        case T::KW_TRUE:
        case T::KW_FALSE: {
            advance();
            auto n = make<BoolLiteral>(t);
            n->value = (t.type == T::KW_TRUE);
            return n;
        }
        case T::IDENTIFIER: {
            advance();
            auto n = make<VarRef>(t);
            n->name = t.text;
            return n;
        }
        case T::LPAREN: {
            advance();
            ExprPtr e = parseExpression();
            expect(T::RPAREN, "')' to close the parenthesized expression");
            return e;
        }
        default:
            error("expected an expression, found " + describe(t), t);
    }
}