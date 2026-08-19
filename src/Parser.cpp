#include <lox/Parser.h>

#include <lox/Ast.h>
#include <lox/Token.h>
#include <lox/TokenType.h>
#include <lox/utils.h>

#include <algorithm>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace lox {

Parser::Parser(std::span<const Token> tokens) : tokens{tokens} {}

auto Parser::parse() -> ExprPtr {
  return expression();
}

auto Parser::expression() -> ExprPtr {
  return equality();
}

auto Parser::equality() -> ExprPtr {
  return binaryExpr(BinaryExprPrecedence::COMPARISON,
                    {TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL});
}

auto Parser::comparison() -> ExprPtr {
  return binaryExpr(BinaryExprPrecedence::TERM,
                    {TokenType::LESS, TokenType::LESS_EQUAL,
                     TokenType::GREATER_EQUAL, TokenType::GREATER_EQUAL});
}

auto Parser::term() -> ExprPtr {
  return binaryExpr(BinaryExprPrecedence::FACTOR,
                    {TokenType::MINUS, TokenType::PLUS});
}

auto Parser::factor() -> ExprPtr {
  return binaryExpr(BinaryExprPrecedence::UNARY,
                    {TokenType::STAR, TokenType::SLASH});
}

auto Parser::unary() -> ExprPtr {
  if (match({TokenType::BANG, TokenType::MINUS})) {
    const auto op = previous();
    auto right = unary();
    return std::make_unique<UnaryExpr>(op, std::move(right));
  }
  return primary();
}

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

auto Parser::primary() -> ExprPtr {
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

auto Parser::consumeOrError(TokenType tokenType, std::string_view error_message)
    -> Token {
  if (check(tokenType))
    return advance();

  throw error(peek(), error_message);
}

auto Parser::error(const Token &token, std::string_view error_message)
    -> ParseException {
  reportError(token, error_message);
  return ParseException{std::string(error_message)};
}

void Parser::reportError(const Token &token, std::string_view message) {
  if (token.type == TokenType::EOF_)
    report(token.line, "at end", message);
  else
    report(token.line, std::format("at '{}'", token.lexeme), message);
}

auto Parser::prec2expr(BinaryExprPrecedence prec) -> ExprPtr {
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

auto Parser::binaryExpr(const BinaryExprPrecedence prec,
                        const std::span<const TokenType> ts) -> ExprPtr {
  auto expr = prec2expr(prec);
  while (match(ts)) {
    const auto op = previous();
    auto right = prec2expr(prec);
    expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
  }
  return expr;
}

auto Parser::match(const std::span<const TokenType> ts) -> bool {
  if (std::ranges::any_of(ts, [this](TokenType t) { return check(t); })) {
    advance();
    return true;
  }
  return false;
}

auto Parser::check(const TokenType type) const -> bool {
  return !isAtEnd() && type == peek().type;
}

auto Parser::isAtEnd() const -> bool {
  return peek().type == TokenType::EOF_;
}

auto Parser::advance() -> Token {
  if (!isAtEnd())
    current++;

  return previous();
}

auto Parser::peek() const -> Token {
  return tokens.at(current);
}

auto Parser::previous() const -> Token {
  return tokens.at(current - 1);
}

void Parser::synchronize() {
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

} // namespace lox
