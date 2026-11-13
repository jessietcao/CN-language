#include "lexer.h"
#include "parser.h"
#include <iostream>

static int failures = 0;
static int total = 0;

#define CHECK(cond, name) do { \
    total++; \
    if (!(cond)) { failures++; std::cout << "FAIL: " << name << "\n"; } \
    else { std::cout << "ok:   " << name << "\n"; } \
} while (0)

static std::vector<Token> lex(const std::string& utf8) {
    return Lexer(utf8_to_u32(utf8)).tokenize();
}

// Parse a bare expression and return its S-expression form.
static std::string expr(const std::string& src) {
    Parser p(lex(src));
    return p.parseExpression()->str();
}

// Parse a whole program; throws ParseError on bad syntax.
static std::unique_ptr<Program> prog(const std::string& src) {
    Parser p(lex(src));
    return p.parseProgram();
}

// Wrap statements in a main function.
static std::string wrap(const std::string& body) {
    return "整数 主函数() {\n" + body + "\n}";
}

struct Err { bool failed = false; std::string msg; int line = 0, col = 0; };
static Err errorOf(const std::string& src) {
    Err e;
    try { prog(src); }
    catch (const ParseError& pe) { e.failed = true; e.msg = pe.what(); e.line = pe.line; e.col = pe.column; }
    return e;
}
static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

// The statements inside the first function's body.
static const std::vector<StmtPtr>& stmts(const Program& p) { return p.functions[0].body->stmts; }

int main() {
    // ---------- expression precedence / associativity ----------
    CHECK(expr("1 + 2 * 3") == "(+ 1 (* 2 3))", "* binds tighter than +");
    CHECK(expr("(1 + 2) * 3") == "(* (+ 1 2) 3)", "parentheses override precedence");
    CHECK(expr("（1 + 2） * 3") == "(* (+ 1 2) 3)", "fullwidth parentheses");
    CHECK(expr("1 - 2 - 3") == "(- (- 1 2) 3)", "subtraction is left-associative");
    CHECK(expr("8 / 4 / 2") == "(/ (/ 8 4) 2)", "division is left-associative");
    CHECK(expr("-甲 * 2") == "(* (- 甲) 2)", "unary minus binds tighter than *");
    CHECK(expr("- - 5") == "(- (- 5))", "stacked unary minus");
    CHECK(expr("1 + 2 < 4") == "(< (+ 1 2) 4)", "arithmetic binds tighter than comparison");
    CHECK(expr("甲 < 3 == 乙 > 1") == "(== (< 甲 3) (> 乙 1))", "comparison binds tighter than ==");
    CHECK(expr("真") == "真" && expr("假") == "假", "bool literals");
    CHECK(expr("19.99") == "19.99", "double literal");
    CHECK(expr("\"你好\"") == "\"你好\"", "string literal");
    CHECK(expr("'甲'") == "'甲'", "char literal");
    CHECK(expr("甲 != 乙") == "(!= 甲 乙)", "!=");

    // ---------- statements ----------
    {
        auto p = prog(wrap("整数 甲=5。 整数 乙。"));
        CHECK(p->functions.size() == 1 && p->functions[0].name == U"主函数", "main function parsed");
        auto* a = dynamic_cast<VarDecl*>(stmts(*p)[0].get());
        auto* b = dynamic_cast<VarDecl*>(stmts(*p)[1].get());
        CHECK(a && a->init && a->init->str() == "5", "var decl with initializer");
        CHECK(b && !b->init, "var decl without initializer");
    }
    {
        auto p = prog(wrap("甲 = 甲 + 1。 甲++。 乙--。"));
        CHECK(dynamic_cast<AssignStmt*>(stmts(*p)[0].get()) != nullptr, "assignment statement");
        auto* inc = dynamic_cast<IncDecStmt*>(stmts(*p)[1].get());
        auto* dec = dynamic_cast<IncDecStmt*>(stmts(*p)[2].get());
        CHECK(inc && inc->op == TokenType::PLUS_PLUS && dec && dec->op == TokenType::MINUS_MINUS, "++ and -- statements");
    }
    {
        auto p = prog(wrap("如果 (真) { } 否则 (假) { } 否则 (甲) { } 其他 { }"));
        auto* i = dynamic_cast<IfStmt*>(stmts(*p)[0].get());
        CHECK(i && i->branches.size() == 3 && i->elseBlock != nullptr, "如果 / 否则 / 否则 / 其他 chain");
    }
    {
        auto p = prog(wrap("如果 (真) { }"));
        auto* i = dynamic_cast<IfStmt*>(stmts(*p)[0].get());
        CHECK(i && i->branches.size() == 1 && i->elseBlock == nullptr, "if without else");
    }
    {
        auto p = prog(wrap("当 (甲 < 3) { 甲++。 }"));
        auto* w = dynamic_cast<WhileStmt*>(stmts(*p)[0].get());
        CHECK(w && w->cond->str() == "(< 甲 3)" && w->body->stmts.size() == 1, "while loop");
    }
    {
        auto p = prog(wrap("循环 (整数 甲=0; 甲<=10; 甲++) { }"));
        auto* f = dynamic_cast<ForStmt*>(stmts(*p)[0].get());
        CHECK(f && f->init && f->cond && f->step, "for loop with all three clauses");
    }
    {
        auto p = prog(wrap("循环 (;;) { }"));
        auto* f = dynamic_cast<ForStmt*>(stmts(*p)[0].get());
        CHECK(f && !f->init && !f->cond && !f->step, "for loop with empty clauses");
    }
    {
        auto p = prog(wrap("循环 (甲=0； 甲<3； 甲=甲+1) { }"));
        auto* f = dynamic_cast<ForStmt*>(stmts(*p)[0].get());
        CHECK(f && f->init && f->step, "for loop with fullwidth ； and assignment step");
    }
    {
        auto p = prog(wrap("输出 (\"你好\")。 输入 (甲)。 返回。 返回 0。 { 整数 乙。 }"));
        CHECK(dynamic_cast<OutputStmt*>(stmts(*p)[0].get()) && dynamic_cast<InputStmt*>(stmts(*p)[1].get()), "output / input");
        auto* r1 = dynamic_cast<ReturnStmt*>(stmts(*p)[2].get());
        auto* r2 = dynamic_cast<ReturnStmt*>(stmts(*p)[3].get());
        CHECK(r1 && !r1->value && r2 && r2->value, "bare return and return with value");
        CHECK(dynamic_cast<Block*>(stmts(*p)[4].get()) != nullptr, "nested block statement");
    }
    {
        auto p = prog("整数 f() { 返回 1。 }\n小数 g() { 返回 2.5。 }");
        CHECK(p->functions.size() == 2 && p->functions[1].returnType == TokenType::KW_DOUBLE, "multiple functions");
    }
    CHECK(prog("")->functions.empty(), "empty program");

    // ---------- error messages and locations ----------
    {
        Err e = errorOf(wrap("整数 甲=5"));
        CHECK(e.failed && contains(e.msg, "'。'"), "missing 。 is reported");
    }
    {
        // Reported at the END of the statement (line 2), not at the next token's line.
        Err e = errorOf("整数 主函数() {\n整数 甲=5\n}");
        CHECK(e.failed && e.line == 2, "missing 。 points at the statement's own line");
    }
    {
        Err e = errorOf("整数 主函数() {");
        CHECK(e.failed && contains(e.msg, "never closed") && e.line == 1 && e.col == 10, "unclosed { points at the brace");
    }
    {
        Err e = errorOf(wrap("如果 (真) { } 否则 { }"));
        CHECK(e.failed && contains(e.msg, "其他"), "否则 without condition suggests 其他");
    }
    {
        Err e = errorOf(wrap("否则 (真) { }"));
        CHECK(e.failed && contains(e.msg, "no matching"), "stray 否则");
    }
    {
        Err e = errorOf(wrap("其他 { }"));
        CHECK(e.failed && contains(e.msg, "no matching"), "stray 其他");
    }
    {
        Err e = errorOf(wrap("当 真 { }"));
        CHECK(e.failed && contains(e.msg, "'('"), "while without parentheses");
    }
    {
        Err e = errorOf(wrap("循环 (整数 甲=0; 甲<3 甲++) { }"));
        CHECK(e.failed && contains(e.msg, "';'"), "for loop missing second ;");
    }
    {
        Err e = errorOf(wrap("甲 5。"));
        CHECK(e.failed && contains(e.msg, "'='"), "identifier followed by garbage");
    }
    {
        Err e = errorOf("输出 (\"x\")。");
        CHECK(e.failed && contains(e.msg, "return type"), "statement outside any function");
    }
    {
        Err e = errorOf(wrap("整数 甲 = 。"));
        CHECK(e.failed && contains(e.msg, "expected an expression"), "missing expression after =");
    }
    {
        Err e = errorOf(wrap("整数 甲 = (1 + 2。"));
        CHECK(e.failed && contains(e.msg, "')'"), "unclosed parenthesis in expression");
    }
    {
        Err e = errorOf(wrap("整数 =5。"));
        CHECK(e.failed && contains(e.msg, "variable name"), "declaration missing its name");
    }
    {
        Err e = errorOf("整数 主函数(整数 甲) { }");
        CHECK(e.failed && contains(e.msg, "not supported"), "parameters give a clear message");
    }
    {
        Err e = errorOf(wrap("整数 甲 = 99999999999999999999。"));
        CHECK(e.failed && contains(e.msg, "too large"), "integer literal overflow");
    }

    std::cout << "\n" << (total - failures) << "/" << total << " passed\n";
    return failures ? 1 : 0;
}