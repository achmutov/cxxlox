cmake_minimum_required(VERSION 4.1.4)

# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
# set(CMAKE_CXX_MODULE_STD)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(cxxlox VERSION 1.0.0)

add_executable(lox)
target_sources(lox PUBLIC ./src/main.cpp)
target_sources(
    lox
    PUBLIC
        FILE_SET modules
            TYPE CXX_MODULES
            FILES
                src/modules/Scanner.cppm
                src/modules/Token.cppm
                src/modules/Ast.cppm
                src/modules/AstPrinter.cppm
                src/modules/utils.cppm
                src/modules/TokenType.cppm
                src/modules/Parser.cppm
)
target_compile_options(
    lox
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
)
target_link_options(
    lox
    PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
)

include_directories(include)
