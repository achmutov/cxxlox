#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

import Token;
import Scanner;
import Ast;
// import AstPrinter;
import Parser;
import utils;
import Interpreter;

static void run(std::string_view script, lox::Interpreter &interpreter,
                bool isRepl) {
  const std::vector<lox::Token> tokens = lox::Scanner(script).scanTokens();
  for (auto &token : tokens)
    std::println("{}", token.toString());
  auto parser = lox::Parser{tokens, isRepl};
  auto stmts = parser.parse();
  if (lox::hadError)
    return;

  // auto astPrinter = lox::AstPrinter();
  // std::println("{}", expr->accept(astPrinter));
  interpreter.interpret(stmts);
}

static void runFile(std::string_view path) {
  std::ifstream f(path.data());
  if (!f.is_open()) {
    std::println(stderr, "Can't open file");
    return;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  lox::Interpreter interpreter;
  run(ss.str(), interpreter, false);

  if (lox::hadError)
    exit(65);
  else if (lox::hadRuntimeError) {
    exit(70);
  }
}

static void runPrompt() {
  lox::Interpreter interpreter;
  while (true) {
    std::print("> ");
    std::string line;
    if (!std::getline(std::cin, line))
      break;
    run(line, interpreter, true);
    lox::hadError = false;
  }
  std::println();
}

auto main(int argc, char *argv[]) -> int {
  // auto ast_printer = lox::AstPrinter();
  // auto b = lox::BinaryExpr(
  //     {lox::TokenType::PLUS, "+", std::nullopt, 0},
  //     std::make_unique<lox::LiteralExpr>(std::string("123")),
  //     std::make_unique<lox::GroupingExpr>(std::make_unique<lox::BinaryExpr>(
  //         lox::Token{lox::TokenType::PLUS, "+", std::nullopt, 0},
  //         std::make_unique<lox::LiteralExpr>(true),
  //         std::make_unique<lox::LiteralExpr>(123.456))));
  // std::println("{}", b.accept(ast_printer));

  if (argc > 2) {
    std::println(stderr, "Usage: lox [script]");
    exit(64);
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    runPrompt();
  }
  return 0;
}
