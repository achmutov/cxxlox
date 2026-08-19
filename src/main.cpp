#include <lox/Ast.h>
#include <lox/AstPrinter.h>
#include <lox/Parser.h>
#include <lox/Scanner.h>
#include <lox/Token.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

static bool hadError = false;

static void run(std::string_view script) {
  const std::vector<lox::Token> tokens = lox::Scanner(script).scanTokens();
  for (auto &token : tokens)
    std::println("{}", token.toString());
  auto parser = lox::Parser{tokens};
  auto expr = parser.parse();

  auto astPrinter = lox::AstPrinter();
  std::println("{}", expr->accept(astPrinter));
}

static void runFile(std::string_view path) {
  std::ifstream f(path.data());
  if (!f.is_open()) {
    std::println(stderr, "Can't open file");
    return;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  run(ss.str());

  if (hadError)
    exit(65);
}

static void runPrompt() {
  while (true) {
    std::print("> ");
    std::string line;
    if (!std::getline(std::cin, line))
      break;
    run(line);
    hadError = false;
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
