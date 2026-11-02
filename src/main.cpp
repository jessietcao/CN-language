#include "lexer.h"
#include "token.h"
#include <fstream>
#include <sstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source-file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Could not open file: " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string rawUtf8 = buffer.str();

    std::u32string source = utf8_to_u32(rawUtf8);
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    for (const Token& tok : tokens) {
        std::cout << "[" << tok.line << ":" << tok.column << "] "
                  << tokenTypeName(tok.type)
                  << " \"" << u32_to_utf8(tok.text) << "\"\n";
    }

    return 0;
}