#pragma once
#include "token.h"
#include <vector>
#include <string>
#include <unordered_map>

class Lexer {
public:
    explicit Lexer(std::u32string source);
    std::vector<Token> tokenize();

private:
    std::u32string src;
    size_t pos = 0;
    int line = 1;
    int col = 1;

    // Longest keyword is checked first — see lexIdentifierOrKeyword().
    static const std::unordered_map<std::u32string, TokenType> keywords;

    char32_t peek(int offset = 0) const;
    char32_t advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();

    bool isWordChar(char32_t c) const;   // CJK ideograph, ASCII letter/digit, underscore
    bool isDigit(char32_t c) const;

    Token makeToken(TokenType type, const std::u32string& text, int startLine, int startCol);

    Token lexIdentifierOrKeyword();
    Token lexNumber();
    Token lexString();
    Token lexChar();
    Token lexSymbol();
};

// Decode a UTF-8 encoded std::string into a std::u32string of codepoints.
std::u32string utf8_to_u32(const std::string& input);

// Encode a std::u32string back into UTF-8, for printing to the terminal.
std::string u32_to_utf8(const std::u32string& input);