#include "lexer.h"
#include <cstdint>
#include <stdexcept>

// ---------- UTF-8 <-> UTF-32 ----------
// Minimal, dependency-free decoder/encoder. std::codecvt is deprecated and
// we don't need anything fancier than this for reading source files.

std::u32string utf8_to_u32(const std::string& input) {
    std::u32string out;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c = input[i];
        char32_t codepoint = 0;
        int extraBytes = 0;

        if ((c & 0x80) == 0x00) {           // 0xxxxxxx — 1 byte
            codepoint = c;
            extraBytes = 0;
        } else if ((c & 0xE0) == 0xC0) {    // 110xxxxx — 2 bytes
            codepoint = c & 0x1F;
            extraBytes = 1;
        } else if ((c & 0xF0) == 0xE0) {    // 1110xxxx — 3 bytes (most CJK chars land here)
            codepoint = c & 0x0F;
            extraBytes = 2;
        } else if ((c & 0xF8) == 0xF0) {    // 11110xxx — 4 bytes
            codepoint = c & 0x07;
            extraBytes = 3;
        } else {
            throw std::runtime_error("Invalid UTF-8 byte in source file");
        }

        if (i + extraBytes >= input.size() + 1 && extraBytes > 0 && i + extraBytes >= input.size()) {
            throw std::runtime_error("Truncated UTF-8 sequence in source file");
        }

        for (int b = 1; b <= extraBytes; b++) { // continuation bytes: 10xxxxxx
            unsigned char cont = input[i + b];
            if ((cont & 0xC0) != 0x80) {
                throw std::runtime_error("Malformed UTF-8 continuation byte");
            }
            codepoint = (codepoint << 6) | (cont & 0x3F);
        }

        out.push_back(codepoint);
        i += extraBytes + 1;
    }
    return out;
}

std::string u32_to_utf8(const std::u32string& input) {
    std::string out;
    for (char32_t cp : input) {
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

// ---------- Keyword table ----------
// U-prefixed literals (U"...") are UTF-32 string literals — the compiler
// encodes the Chinese characters directly as char32_t codepoints for us.

const std::unordered_map<std::u32string, TokenType> Lexer::keywords = {
    {U"整数", TokenType::KW_INT},
    {U"小数", TokenType::KW_DOUBLE},
    {U"字",   TokenType::KW_CHAR},
    {U"字符串", TokenType::KW_STRING},
    {U"是非", TokenType::KW_BOOL},
    {U"真",   TokenType::KW_TRUE},
    {U"假",   TokenType::KW_FALSE},
    {U"如果", TokenType::KW_IF},
    {U"否则", TokenType::KW_ELSE},
    {U"只要", TokenType::KW_WHILE},
    {U"返回", TokenType::KW_RETURN},
    {U"主函数", TokenType::KW_MAIN},
    {U"输入", TokenType::KW_INPUT},
    {U"输出", TokenType::KW_OUTPUT},
};

// ---------- Lexer ----------

Lexer::Lexer(std::u32string source) : src(std::move(source)) {}

char32_t Lexer::peek(int offset) const {
    size_t p = pos + offset;
    if (p >= src.size()) return U'\0';
    return src[p];
}

char32_t Lexer::advance() {
    char32_t c = src[pos++];
    if (c == U'\n') { line++; col = 1; }
    else { col++; }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= src.size();
}

bool Lexer::isDigit(char32_t c) const {
    return c >= U'0' && c <= U'9';
}

bool Lexer::isWordChar(char32_t c) const {
    // ASCII letters/digits/underscore, or CJK Unified Ideographs (main block).
    // Extend this range later if you want CJK Extension A/B or other blocks.
    bool ascii = (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') ||
                 (c >= U'0' && c <= U'9') || c == U'_';
    bool cjk = (c >= 0x4E00 && c <= 0x9FFF);
    return ascii || cjk;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char32_t c = peek();
        if (c == U' ' || c == U'\t' || c == U'\r' || c == U'\n') {
            advance();
        } else if (c == U'/' && peek(1) == U'/') {
            // line comment, e.g. //注释
            while (!isAtEnd() && peek() != U'\n') advance();
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::u32string& text, int startLine, int startCol) {
    return Token{type, text, startLine, startCol};
}

Token Lexer::lexIdentifierOrKeyword() {
    int startLine = line, startCol = col;
    size_t start = pos;
    while (!isAtEnd() && isWordChar(peek())) advance();
    std::u32string text = src.substr(start, pos - start);

    auto it = keywords.find(text);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    return makeToken(type, text, startLine, startCol);
}

Token Lexer::lexNumber() {
    int startLine = line, startCol = col;
    size_t start = pos;
    bool isDouble = false;
    while (!isAtEnd() && isDigit(peek())) advance();
    if (peek() == U'.' && isDigit(peek(1))) {
        isDouble = true;
        advance(); // consume '.'
        while (!isAtEnd() && isDigit(peek())) advance();
    }
    std::u32string text = src.substr(start, pos - start);
    return makeToken(isDouble ? TokenType::DOUBLE_LITERAL : TokenType::INT_LITERAL,
                      text, startLine, startCol);
}

Token Lexer::lexString() {
    int startLine = line, startCol = col;
    advance(); // consume opening “
    size_t start = pos;
    while (!isAtEnd() && peek() != U'”') advance();
    std::u32string text = src.substr(start, pos - start);
    if (!isAtEnd()) advance(); // consume closing ”
    return makeToken(TokenType::STRING_LITERAL, text, startLine, startCol);
}

Token Lexer::lexChar() {
    int startLine = line, startCol = col;
    advance(); // consume opening ‘
    size_t start = pos;
    while (!isAtEnd() && peek() != U'’') advance();
    std::u32string text = src.substr(start, pos - start);
    if (!isAtEnd()) advance(); // consume closing ’
    return makeToken(TokenType::CHAR_LITERAL, text, startLine, startCol);
}

Token Lexer::lexSymbol() {
    int startLine = line, startCol = col;
    char32_t c = advance();

    switch (c) {
        case U'(': return makeToken(TokenType::LPAREN, U"(", startLine, startCol);
        case U')': return makeToken(TokenType::RPAREN, U")", startLine, startCol);
        case U'{': return makeToken(TokenType::LBRACE, U"{", startLine, startCol);
        case U'}': return makeToken(TokenType::RBRACE, U"}", startLine, startCol);
        case U',':
        case 0xFF0C: // ，(fullwidth comma)
            return makeToken(TokenType::COMMA, U",", startLine, startCol);
        case 0x3002: // 。(ideographic full stop) — statement terminator
            return makeToken(TokenType::PERIOD, U"。", startLine, startCol);
        case U'=':
            if (peek() == U'=') { advance(); return makeToken(TokenType::EQUAL_EQUAL, U"==", startLine, startCol); }
            return makeToken(TokenType::ASSIGN, U"=", startLine, startCol);
        case U'+':
            if (peek() == U'+') { advance(); return makeToken(TokenType::PLUS_PLUS, U"++", startLine, startCol); }
            return makeToken(TokenType::PLUS, U"+", startLine, startCol);
        case U'-':
            if (peek() == U'-') { advance(); return makeToken(TokenType::MINUS_MINUS, U"--", startLine, startCol); }
            return makeToken(TokenType::MINUS, U"-", startLine, startCol);
        case U'*': return makeToken(TokenType::STAR, U"*", startLine, startCol);
        case U'/': return makeToken(TokenType::SLASH, U"/", startLine, startCol);
        case U'<':
            if (peek() == U'=') { advance(); return makeToken(TokenType::LESS_EQUAL, U"<=", startLine, startCol); }
            return makeToken(TokenType::LESS, U"<", startLine, startCol);
        case U'>':
            if (peek() == U'=') { advance(); return makeToken(TokenType::GREATER_EQUAL, U">=", startLine, startCol); }
            return makeToken(TokenType::GREATER, U">", startLine, startCol);
        case U'!':
            if (peek() == U'=') { advance(); return makeToken(TokenType::NOT_EQUAL, U"!=", startLine, startCol); }
            return makeToken(TokenType::UNKNOWN, U"!", startLine, startCol);
        default: {
            std::u32string text(1, c);
            return makeToken(TokenType::UNKNOWN, text, startLine, startCol);
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();
        if (isAtEnd()) {
            tokens.push_back(makeToken(TokenType::END_OF_FILE, U"", line, col));
            break;
        }

        char32_t c = peek();
        if (isWordChar(c) && !isDigit(c)) {
            tokens.push_back(lexIdentifierOrKeyword());
        } else if (isDigit(c)) {
            tokens.push_back(lexNumber());
        } else if (c == U'“') {
            tokens.push_back(lexString());
        } else if (c == U'‘') {
            tokens.push_back(lexChar());
        } else {
            tokens.push_back(lexSymbol());
        }
    }

    return tokens;
}