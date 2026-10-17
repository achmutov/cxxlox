module;

#include <string>
#include <string_view>
#include <variant>

export module InterpreterObject;

namespace lox::detail {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace lox::detail

export namespace lox {

template <typename T>
concept ValuedInterpreterObject = requires(const T &x) { x.value; };

struct InterpreterNil {};
struct InterpreterString {
  std::string value;
};
struct InterpreterNumber {
  double value;

public:
  auto operator-() {
    value = -value;
    return *this;
  }
};
struct InterpreterBoolean {
  bool value;

public:
  auto operator!() {
    value = !value;
    return *this;
  }
};

using InterpreterObject = std::variant<InterpreterNil, InterpreterString,
                                       InterpreterNumber, InterpreterBoolean>;

constexpr auto interpreterTypeName(const InterpreterObject &object)
    -> std::string_view {
  using namespace std::string_view_literals;
  return object.visit(lox::detail::overloads{
      [](const InterpreterString &) { return "string"sv; },
      [](const InterpreterNil) { return "nil"sv; },
      [](const InterpreterBoolean) { return "boolean"sv; },
      [](const InterpreterNumber &) { return "number"sv; }});
}
} // namespace lox
