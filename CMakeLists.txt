cmake_minimum_required(VERSION 4.1.4)
set(CMAKE_CXX_COMPILER clang++)
project(cxxlox VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

add_executable(lox)
target_sources(
    lox
    PUBLIC
        ./src/main.cpp
        ./src/Scanner.cpp
        ./src/Token.cpp
        ./src/utils.cpp
        ./src/AstPrinter.cpp
        ./src/Parser.cpp
        ./src/Ast.cpp
)
target_compile_options(lox PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

include_directories(include)
