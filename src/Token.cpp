#include <lox/Token.h>

#include <lox/TokenType.h>
#include <lox/utils.h>

#include <format>
#include <optional>
#include <string>
#include <utility>

namespace lox {

Token::Token(TokenType type, std::string lexeme, std::optional<Literal> literal,
             int line)
    : lexeme{std::move(lexeme)}, type{type}, literal{std::move(literal)},
      line{line} {}

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

auto Token::toString() const -> std::string {
  const auto token_repr = tokenType(type).value_or("<unknown>");
  const auto literal_repr =
      literal
          .transform([](Literal v) {
            return v.visit(
                overloads{[](std::string value) { return value; },
                          [](double value) { return std::to_string(value); }});
          })
          .value_or("<empty>");
  const auto line_repr = std::to_string(line);

  return std::format("type={} lexeme={} literal={} line={}", token_repr, lexeme,
                     literal_repr, line_repr);
}

} // namespace lox
