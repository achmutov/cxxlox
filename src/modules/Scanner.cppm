module;

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module Scanner;
import Token;
import utils;
import TokenType;

namespace lox {

static constexpr auto lookup_keyword(std::string_view keyword)
    -> std::optional<TokenType> {
  using namespace std::string_view_literals;

  // SORTED
  static constexpr std::array keywords{
      std::pair{"and"sv, TokenType::AND},
      std::pair{"class"sv, TokenType::CLASS},
      std::pair{"else"sv, TokenType::ELSE},
      std::pair{"false"sv, TokenType::FALSE},
      std::pair{"for"sv, TokenType::FOR},
      std::pair{"fun"sv, TokenType::FUN},
      std::pair{"if"sv, TokenType::IF},
      std::pair{"nil"sv, TokenType::NIL},
      std::pair{"or"sv, TokenType::OR},
      std::pair{"print"sv, TokenType::PRINT},
      std::pair{"return"sv, TokenType::RETURN},
      std::pair{"super"sv, TokenType::SUPER},
      std::pair{"this"sv, TokenType::THIS},
      std::pair{"true"sv, TokenType::TRUE},
      std::pair{"var"sv, TokenType::VAR},
      std::pair{"while"sv, TokenType::WHILE},
  };

  const auto *const it = std::ranges::lower_bound(
      keywords, keyword, {}, &decltype(keywords)::value_type::first);
  if (it != keywords.end() && it->first == keyword)
    return it->second;
  return std::nullopt;
}

export class Scanner {
  const std::string_view content;
  std::vector<Token> tokens;
  std::size_t start{};
  std::size_t current{};
  std::size_t line{1};

  [[nodiscard]] auto isAtEnd() const -> bool {
    return current >= content.length();
  }

  void scanToken() {
    auto const c = advance();
    switch (c) {
    case '(':
      addToken(TokenType::LEFT_PAREN);
      break;
    case ')':
      addToken(TokenType::RIGHT_PAREN);
      break;
    case '{':
      addToken(TokenType::LEFT_BRACE);
      break;
    case '}':
      addToken(TokenType::RIGHT_BRACE);
      break;
    case ',':
      addToken(TokenType::COMMA);
      break;
    case '.':
      addToken(TokenType::DOT);
      break;
    case '-':
      addToken(TokenType::MINUS);
      break;
    case '+':
      addToken(TokenType::PLUS);
      break;
    case ';':
      addToken(TokenType::SEMICOLON);
      break;
    case '*':
      addToken(TokenType::STAR);
      break;
    case '!':
      addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
      break;
    case '=':
      addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
      break;
    case '<':
      addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
      break;
    case '>':
      addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
      break;
    case '/':
      if (match('/'))
        while (peek() != '\n' && !isAtEnd())
          advance();
      else if (match('*')) {
        blockComment();
      } else
        addToken(TokenType::SLASH);
      break;
    case ' ':
    case '\r':
    case '\t':
      break;
    case '\n':
      line++;
      break;
    case '"':
      string();
      break;
    default:
      if (isDigit(c))
        number();
      else if (isAlpha(c))
        identifier();
      else
        error(line, "Unexpected character.");

      return;
    }
  }

  auto advance() -> char {
    return content.at(current++);
  }

  void addToken(TokenType type) {
    addToken(type, std::nullopt);
  }

  void addToken(TokenType type, const std::optional<Literal> &literal) {
    const std::string_view lexeme = content.substr(start, current - start);
    tokens.emplace_back(type, std::string(lexeme), literal, line);
  }

  auto match(char expected) -> bool {
    if (isAtEnd() || content.at(current) != expected)
      return false;
    current++;
    return true;
  }

  [[nodiscard]] auto peek() const -> char {
    return isAtEnd() ? '\0' : content.at(current);
  }

  [[nodiscard]] auto peekNext() const -> char {
    return current + 1 >= content.length() ? '\0' : content.at(current + 1);
  }

  void string() {
    while (peek() != '"' && !isAtEnd()) {
      if (peek() == '\n')
        line++;
      advance();
    }

    if (isAtEnd()) {
      error(line, "Unterminated string.");
      return;
    }

    // close '"'
    advance();

    const auto lexeme = content.substr(start + 1, current - start - 2);
    const auto literal = std::string(lexeme);
    addToken(TokenType::STRING, {literal});
  }

  static auto isDigit(char c) -> bool {
    return c >= '0' && c <= '9';
  }

  static auto isAlpha(char c) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
  }

  static auto isAlphaNumeric(char c) -> bool {
    return isDigit(c) || isAlpha(c);
  }

  void number() {
    // integer part
    while (isDigit(peek()))
      advance();

    // dot and fractional part
    if (peek() == '.' && isDigit(peekNext())) {
      advance();
      while (isDigit(peek()))
        advance();
    }

    const auto lexeme = content.substr(start, current - start);
    const auto literal = std::stod(std::string(lexeme));
    addToken(TokenType::NUMBER, {literal});
  }

  void identifier() {
    while (isAlphaNumeric(peek()))
      advance();

    const auto identifier = content.substr(start, current - start);
    const auto keyword = lookup_keyword(identifier);
    addToken(keyword.value_or(TokenType::IDENTIFIER));
  }

  void blockComment() {
    auto nested = 1z;
    do {
      if (isAtEnd()) {
        error(line, "Unterminated block comment.");
        return;
      }

      switch (advance()) {
      case '*':
        if (peek() == '/')
          nested--;
        advance();
        break;
      case '/':
        if (peek() == '*')
          nested++;
        advance();
        break;
      }
    } while (nested > 0);
  }

public:
  explicit Scanner(std::string_view content) : content{content} {}

  auto scanTokens() -> std::vector<Token> {
    while (!isAtEnd()) {
      start = current;
      scanToken();
    }

    tokens.emplace_back(TokenType::EOF_, "", std::nullopt, line);
    return tokens;
  }
};
} // namespace lox
