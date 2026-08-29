module;

#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <variant>

export module Token;
import utils;
import TokenType;

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

export namespace lox {

using Literal = std::variant<std::string, double>;

class Token {
public:
  const std::string lexeme;
  const TokenType type;
  const std::optional<Literal> literal;
  const std::size_t line;

  Token(TokenType type, std::string lexeme, std::optional<Literal> literal,
        std::size_t line)
      : lexeme{std::move(lexeme)}, type{type}, literal{std::move(literal)},
        line{line} {}

  [[nodiscard]] auto toString() const -> std::string {
    const auto token_repr = tokenType(type).value_or("<unknown>");
    const auto literal_repr =
        literal
            .transform([](Literal v) {
              return v.visit(overloads{
                  [](std::string value) { return value; },
                  [](double value) { return std::to_string(value); }});
            })
            .value_or("<empty>");
    const auto line_repr = std::to_string(line);

    return std::format("type={} lexeme={} literal={} line={}", token_repr,
                       lexeme, literal_repr, line_repr);
  }
};

} // namespace lox
