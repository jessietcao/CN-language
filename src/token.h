#pragma once
#include <string>

enum class TokenType {
    KW_INT,       // 整数
    KW_DOUBLE,    // 小数
    KW_CHAR,      // 字
    KW_STRING,    // 字符串
    KW_BOOL,      // 是非
    KW_TRUE,      // 真
    KW_FALSE,     // 假
    KW_IF,        // 如果
    KW_ELSE,      // 否则
    KW_WHILE,     // 只要
    KW_RETURN,    // 返回
    KW_MAIN,      // 主函数
    KW_INPUT,     // 输入
    KW_OUTPUT,    // 输出

    // Literals
    IDENTIFIER,
    INT_LITERAL,
    DOUBLE_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,

    // Punctuation
    LPAREN, RPAREN,       // ( )
    LBRACE, RBRACE,       // { }
    COMMA,                // ，or ,
    PERIOD,                // 。  - statement terminator
    ASSIGN,                // =
    PLUS, MINUS, STAR, SLASH,
    LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    EQUAL_EQUAL, NOT_EQUAL,
    PLUS_PLUS, MINUS_MINUS, //++, --

    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::u32string text;   // raw text matched, in original form
    int line;
    int column;
};

// For debugging / printing tokens while testing the lexer
std::string tokenTypeName(TokenType type);