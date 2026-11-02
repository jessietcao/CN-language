#include "../src/lexer.h"
#include <iostream>
#include <vector>

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

// Compare token types only, ignoring the trailing END_OF_FILE
static bool types(const std::string& src, std::vector<TokenType> expected) {
    auto toks = lex(src);
    expected.push_back(TokenType::END_OF_FILE);
    if (toks.size() != expected.size()) return false;
    for (size_t i = 0; i < toks.size(); i++)
        if (toks[i].type != expected[i]) return false;
    return true;
}

using T = TokenType;

int main() {
    // --- UTF-8 round trip ---
    {
        std::string s = "整数 abc 甲 \xF0\x9F\x98\x80";  // includes a 4-byte emoji
        CHECK(u32_to_utf8(utf8_to_u32(s)) == s, "utf8 -> u32 -> utf8 round trip");
        CHECK(utf8_to_u32("甲").size() == 1, "one Chinese char is one codepoint");
    }

    // --- keywords and identifiers ---
    CHECK(types("整数 如果 否则 只要 返回", {T::KW_INT, T::KW_IF, T::KW_ELSE, T::KW_WHILE, T::KW_RETURN}), "keywords");
    CHECK(types("甲", {T::IDENTIFIER}), "chinese identifier");
    CHECK(types("count", {T::IDENTIFIER}), "english identifier");
    CHECK(types("my_var2", {T::IDENTIFIER}), "identifier with underscore and digit");
    CHECK(types("整数 甲", {T::KW_INT, T::IDENTIFIER}), "keyword then identifier");
    CHECK(types("整数甲", {T::IDENTIFIER}), "no space: becomes ONE identifier (documented behavior)");
    CHECK(types("字 字符串", {T::KW_CHAR, T::KW_STRING}), "字 vs 字符串 are distinct keywords");
    CHECK(types("字符", {T::IDENTIFIER}), "字符 is not a keyword");

    // --- numbers ---
    CHECK(types("42", {T::INT_LITERAL}), "int literal");
    CHECK(types("19.99", {T::DOUBLE_LITERAL}), "double literal");
    CHECK(types("5。", {T::INT_LITERAL, T::PERIOD}), "int then 。");
    CHECK(types("1.", {T::INT_LITERAL, T::UNKNOWN}), "trailing dot is not part of the number");

    // --- operators ---
    CHECK(types("= == < <= > >= != ++ --", {T::ASSIGN, T::EQUAL_EQUAL, T::LESS, T::LESS_EQUAL,
          T::GREATER, T::GREATER_EQUAL, T::NOT_EQUAL, T::PLUS_PLUS, T::MINUS_MINUS}), "operators");
    CHECK(types("甲=5。", {T::IDENTIFIER, T::ASSIGN, T::INT_LITERAL, T::PERIOD}), "assignment statement");

    // --- strings, chars, comments ---
    {
        auto toks = lex("“你好【回车】”");
        CHECK(toks.size() == 2 && toks[0].type == T::STRING_LITERAL &&
              u32_to_utf8(toks[0].text) == "你好【回车】", "string literal keeps contents");
    }
    CHECK(types("‘甲’", {T::CHAR_LITERAL}), "char literal");
    CHECK(types("//注释 整数\n甲", {T::IDENTIFIER}), "line comment skipped");

    // --- line / column tracking ---
    {
        auto toks = lex("整数\n  甲");
        CHECK(toks[1].line == 2 && toks[1].column == 3, "line/column after newline");
    }

    // --- edge cases that may expose problems ---
    CHECK(types("", {}), "empty file");
    CHECK(types("   \n\t ", {}), "whitespace only");
    CHECK(types("（", {T::UNKNOWN}), "fullwidth paren is UNKNOWN");
    CHECK(types("\xEF\xBB\xBF" "整数", {T::KW_INT}), "UTF-8 BOM at file start is ignored");
    CHECK(types("“abc", {T::STRING_LITERAL}), "unterminated string (currently accepted silently)");

    std::cout << "\n" << (total - failures) << "/" << total << " passed\n";
    return failures ? 1 : 0;
}