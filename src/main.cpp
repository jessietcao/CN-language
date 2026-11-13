#include "lexer.h"
#include "parser.h"
#include "token.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    bool printTokens = false;
    const char* path = nullptr;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--tokens") == 0) printTokens = true;
        else path = argv[i];
    }
    if (!path) {
        std::cerr << "Usage: " << argv[0] << " [--tokens] <source-file>\n";
        return 1;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file: " << path << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::u32string source;
    try {
        source = utf8_to_u32(buffer.str());
    } catch (const std::exception& e) {
        std::cerr << path << ": " << e.what() << "\n";
        return 1;
    }

    std::vector<Token> tokens = Lexer(source).tokenize();

    if (printTokens) {
        for (const Token& tok : tokens) {
            std::cout << "[" << tok.line << ":" << tok.column << "] "
                      << tokenTypeName(tok.type)
                      << " \"" << u32_to_utf8(tok.text) << "\"\n";
        }
        return 0;
    }

    try {
        Parser parser(std::move(tokens));
        auto program = parser.parseProgram();
        program->print();
    } catch (const ParseError& e) {
        std::cerr << path << ":" << e.line << ":" << e.column << ": error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}