#pragma once

#include <lox/Token.h>
#include <lox/TokenType.h>

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace lox {

class Scanner {
  const std::string_view content;
  std::vector<Token> tokens;
  std::size_t start{};
  std::size_t current{};
  std::size_t line{1};

  [[nodiscard]] auto isAtEnd() const -> bool;
  void scanToken();
  auto advance() -> char;
  void addToken(TokenType);
  void addToken(TokenType, const std::optional<Literal> &);
  auto match(char) -> bool;
  [[nodiscard]] auto peek() const -> char;
  [[nodiscard]] auto peekNext() const -> char;
  void string();
  static auto isDigit(char) -> bool;
  static auto isAlpha(char) -> bool;
  static auto isAlphaNumeric(char) -> bool;
  void number();
  void identifier();
  void blockComment();

public:
  Scanner(std::string_view);
  auto scanTokens() -> std::vector<Token>;
};

} // namespace lox
