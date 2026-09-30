module;

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

export module Parser;
import Token;
import Ast;
import TokenType;
import utils;

namespace lox {

template <class T>
concept TokenTypeType = std::same_as<TokenType, T>;

enum class BinaryExprPrecedence : std::uint8_t {
  COMPARISON,
  TERM,
  FACTOR,
  UNARY
};

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

export class ParseException : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

export class Parser {
  const std::span<const Token> tokens;
  std::size_t current{0};

  // traversal
  auto expression() -> ExprPtr {
    return equality();
  }
  auto equality() -> ExprPtr {
    return binaryExpr(BinaryExprPrecedence::COMPARISON,
                      {TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL});
  }
  auto comparison() -> ExprPtr {
    return binaryExpr(BinaryExprPrecedence::TERM,
                      {TokenType::LESS, TokenType::LESS_EQUAL,
                       TokenType::GREATER_EQUAL, TokenType::GREATER_EQUAL});
  }
  auto term() -> ExprPtr {
    return binaryExpr(BinaryExprPrecedence::FACTOR,
                      {TokenType::MINUS, TokenType::PLUS});
  }
  auto factor() -> ExprPtr {
    return binaryExpr(BinaryExprPrecedence::UNARY,
                      {TokenType::STAR, TokenType::SLASH});
  }
  auto unary() -> ExprPtr {
    if (match({TokenType::BANG, TokenType::MINUS})) {
      const auto op = previous();
      auto right = unary();
      return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return primary();
  }
  auto primary() -> ExprPtr {
    if (match({TokenType::FALSE}))
      return std::make_unique<LiteralExpr>(false);
    if (match({TokenType::TRUE}))
      return std::make_unique<LiteralExpr>(true);
    if (match({TokenType::NIL}))
      return std::make_unique<LiteralExpr>(std::nullopt);

    if (match({TokenType::NUMBER, TokenType::STRING})) {
      auto prev_literal = previous().literal;

      std::optional<ParseLiteral> const n =
          prev_literal.transform([](auto const &v) {
            return v.visit(
                overloads{[](auto const &arg) -> ParseLiteral { return arg; }});
          });

      return std::make_unique<LiteralExpr>(n);
    }

    if (match({TokenType::LEFT_PAREN})) {
      auto expr = expression();
      consumeOrError(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
      return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw error(peek(), "Expect expression.");
  }

  // traversal helpers
  auto prec2expr(BinaryExprPrecedence prec) -> ExprPtr {
    switch (prec) {
    case BinaryExprPrecedence::COMPARISON:
      return comparison();
    case BinaryExprPrecedence::TERM:
      return term();
    case BinaryExprPrecedence::FACTOR:
      return factor();
    case BinaryExprPrecedence::UNARY:
      return unary();
    }
  }
  auto binaryExpr(const BinaryExprPrecedence prec,
                  const std::span<const TokenType> ts) -> ExprPtr {
    auto expr = prec2expr(prec);
    while (match(ts)) {
      const auto op = previous();
      auto right = prec2expr(prec);
      expr =
          std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }
    return expr;
  }
  auto match(const std::span<const TokenType> ts) -> bool {
    if (std::ranges::any_of(ts, [this](TokenType t) { return check(t); })) {
      advance();
      return true;
    }
    return false;
  }
  auto advance() -> Token {
    if (!isAtEnd())
      current++;

    return previous();
  }

  // error handling
  auto consumeOrError(TokenType tokenType, std::string_view error_message)
      -> Token {
    if (check(tokenType))
      return advance();

    throw error(peek(), error_message);
  }
  auto error(const Token &token, std::string_view error_message)
      -> ParseException {
    reportError(token, error_message);
    return ParseException{std::string(error_message)};
  }
  void reportError(const Token &token, std::string_view message) {
    if (token.type == TokenType::EOF_)
      report(token.line, "at end", message);
    else
      report(token.line, std::format("at '{}'", token.lexeme), message);
  }
  void synchronize() {
    // skip first erroneous token
    advance();

    // while (advance().type != EOF_) ?
    while (!isAtEnd()) {
      if (previous().type == TokenType::SEMICOLON)
        return;

      switch (peek().type) {
      case TokenType::CLASS:
      case TokenType::FUN:
      case TokenType::VAR:
      case TokenType::FOR:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::PRINT:
      case TokenType::RETURN:
        return;
      default:
        advance();
      }
    }
  }

  [[nodiscard]] auto check(const TokenType type) const -> bool {
    return !isAtEnd() && type == peek().type;
  }

  [[nodiscard]] auto isAtEnd() const -> bool {
    return peek().type == TokenType::EOF_;
  }

  [[nodiscard]] auto peek() const -> Token {
    return tokens.at(current);
  }

  [[nodiscard]] auto previous() const -> Token {
    return tokens.at(current - 1);
  }

public:
  // TODO: should be non-zero length
  // ideally, use superconstructing super elider
  explicit Parser(std::span<const Token> tokens) : tokens{tokens} {}
  auto parse() -> std::optional<ExprPtr> {
    try {
      return expression();
    } catch (ParseException &ex) {
      return std::nullopt;
    }
  }
};

} // namespace lox
