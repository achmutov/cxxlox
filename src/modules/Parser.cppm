module;

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

export module Parser;
import Token;
import Ast;
import TokenType;
import utils;

// expression -> comma
// comma      -> ternary ("," ternary)*
// ternary    -> equality "?" ternary ":" ternary
// equality   -> comparison ("==" | "!=" comparison)*
// comparison -> term ((">" | "<" | ">=" | "<=") term)*
// term       -> factor (("+" | "-") factor)*
// factor     -> unary (( "*" | "/" ) unary)*;
// unary      -> ("!" | "-") unary
//             | primary;
// primary    -> "true" | "false" | "nil" | NUMBER | STRING | grouping;
// grouping   -> "(" expression ")";

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
    return comma();
  }
  auto comma() -> ExprPtr {
    return binaryExpr<&Parser::ternary>({TokenType::COMMA});
  }
  auto ternary() -> ExprPtr {
    auto expr = equality();
    while (match({TokenType::QUESTION_MARK})) {
      auto left = ternary();
      consumeOrError(TokenType::COLON,
                     "Expect ':' after first expression in ternary.");
      auto right = ternary();
      expr = std::make_unique<TernaryExpr>(std::move(expr), std::move(left),
                                           std::move(right));
    }
    return expr;
  }

  auto equality() -> ExprPtr {
    return binaryExpr<&Parser::comparison>(
        {TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL});
  }
  auto comparison() -> ExprPtr {
    return binaryExpr<&Parser::term>({TokenType::LESS, TokenType::LESS_EQUAL,
                                      TokenType::GREATER_EQUAL,
                                      TokenType::GREATER_EQUAL});
  }
  auto term() -> ExprPtr {
    return binaryExpr<&Parser::factor>({TokenType::MINUS, TokenType::PLUS});
  }
  auto factor() -> ExprPtr {
    return binaryExpr<&Parser::unary>({TokenType::STAR, TokenType::SLASH});
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
      return std::make_unique<LiteralExpr>(std::monostate{});

    if (match({TokenType::NUMBER, TokenType::STRING})) {
      const auto token = previous();
      const auto literal =
          optional_expect(token.literal, "Token {} is supposed to have literal",
                          token.toString());

      const auto parse_literal = literal.visit(
          overloads{[](auto const &arg) -> ParseLiteral { return arg; }});

      return std::make_unique<LiteralExpr>(parse_literal);
    }

    if (match({TokenType::LEFT_PAREN})) {
      auto expr = expression();
      consumeOrError(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
      return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw error(peek(), "Expect expression.");
  }

  template <auto nextExpr>
  auto binaryExpr(const std::span<const TokenType> ts) -> ExprPtr {
    auto expr = std::invoke(nextExpr, this);
    while (match(ts)) {
      const auto op = previous();
      auto right = std::invoke(nextExpr, this);
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
