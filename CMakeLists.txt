cmake_minimum_required(VERSION 3.16)
project(CN++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CN++

    src/main.cpp
    src/lexer.cpp
    src/token.cpp
)

target_include_directories(CN++ PRIVATE include)

# Once we get to codegen, this is where LLVM gets linked in:
# find_package(LLVM REQUIRED CONFIG)
# target_link_libraries(CN++ LLVM)