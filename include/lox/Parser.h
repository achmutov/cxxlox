#pragma once

#include <lox/Ast.h>
#include <lox/Token.h>
#include <lox/TokenType.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>

namespace lox {

template <class T>
concept TokenTypeType = std::same_as<TokenType, T>;

enum class BinaryExprPrecedence : std::uint8_t {
  COMPARISON,
  TERM,
  FACTOR,
  UNARY
};

class ParseException : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class Parser {
  using ExprPtr = std::unique_ptr<Expr>;

  const std::span<const Token> tokens;
  std::size_t current{0};

  // traversal
  auto expression() -> ExprPtr;
  auto equality() -> ExprPtr;
  auto comparison() -> ExprPtr;
  auto term() -> ExprPtr;
  auto factor() -> ExprPtr;
  auto unary() -> ExprPtr;
  auto primary() -> ExprPtr;

  // traversal helpers
  auto prec2expr(BinaryExprPrecedence) -> ExprPtr;
  auto binaryExpr(BinaryExprPrecedence, std::span<const TokenType>) -> ExprPtr;
  auto match(const std::span<const TokenType>) -> bool;
  auto advance() -> Token;

  // error handling
  auto consumeOrError(TokenType, std::string_view) -> Token;
  auto error(const Token &, std::string_view) -> ParseException;
  void reportError(const Token &, std::string_view);
  void synchronize();

  [[nodiscard]] auto check(const TokenType) const -> bool;
  [[nodiscard]] auto peek() const -> Token;
  [[nodiscard]] auto previous() const -> Token;
  [[nodiscard]] auto isAtEnd() const -> bool;

public:
  // TODO: should be non-zero length
  // ideally, use superconstructing super elider
  Parser(std::span<const Token>);
  auto parse() -> ExprPtr;
};

} // namespace lox
