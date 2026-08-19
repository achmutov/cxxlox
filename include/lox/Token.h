#pragma once

#include <lox/TokenType.h>

#include <optional>
#include <string>
#include <variant>

namespace lox {

using Literal = std::variant<std::string, double>;

class Token {
public:
  Token(TokenType, std::string, std::optional<Literal>, int);

  const std::string lexeme;
  const TokenType type;
  const std::optional<Literal> literal;
  const int line;
  [[nodiscard]] auto toString() const -> std::string;
};

auto literal_to_string(const Literal &) -> std::string;

} // namespace lox
