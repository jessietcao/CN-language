add_executable(test_lexer test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE lang_core)
add_test(NAME lexer COMMAND test_lexer)