cmake_minimum_required(VERSION 3.16)
project(CN++ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core compiler code, shared by the executable and the tests
add_library(lang_core
    src/lexer.cpp
    src/token.cpp
)
target_include_directories(lang_core PUBLIC include)

add_executable(CN++ src/main.cpp)
target_link_libraries(CN++ PRIVATE lang_core)

enable_testing()
add_subdirectory(tests)