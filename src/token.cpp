#include "token.h"

std::string tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_DOUBLE: return "KW_DOUBLE";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_STRING: return "KW_STRING";
        case TokenType::KW_BOOL: return "KW_BOOL";
        case TokenType::KW_TRUE: return "KW_TRUE";
        case TokenType::KW_FALSE: return "KW_FALSE";
        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_MAIN: return "KW_MAIN";
        case TokenType::KW_INPUT: return "KW_INPUT";
        case TokenType::KW_OUTPUT: return "KW_OUTPUT";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::PERIOD: return "PERIOD";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::PLUS_PLUS: return "PLUS_PLUS";
        case TokenType::MINUS_MINUS: return "MINUS_MINUS";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}