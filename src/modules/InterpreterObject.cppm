module;

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

export module Interpreter.Object;

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

struct InterpreterCallableI;
using InterpreterCallable = std::shared_ptr<InterpreterCallableI>;

using InterpreterObject =
    std::variant<InterpreterNil, InterpreterString, InterpreterNumber,
                 InterpreterBoolean, InterpreterCallable>;

struct InterpreterCallableI {
  virtual auto arity() -> std::size_t = 0;
  virtual auto call(std::vector<InterpreterObject> args)
      -> InterpreterObject = 0;
};

constexpr auto interpreterTypeName(const InterpreterObject &object)
    -> std::string_view {
  using namespace std::string_view_literals;
  return object.visit(lox::detail::overloads{
      [](const InterpreterString &) { return "string"sv; },
      [](const InterpreterNil) { return "nil"sv; },
      [](const InterpreterBoolean) { return "boolean"sv; },
      [](const InterpreterNumber &) { return "number"sv; },
      [](const InterpreterCallable &) { return "callable"sv; }});
}

auto stringify(InterpreterObject obj) -> std::string {
  return obj.visit(lox::detail::overloads{
      [](InterpreterString value) { return std::move(value.value); },
      [](const InterpreterNil) { return std::string{"nil"}; },
      [](const InterpreterBoolean value) {
        return std::string{value.value ? "true" : "false"};
      },
      [](const InterpreterNumber value) { return std::to_string(value.value); },
      [](const InterpreterCallable &) { return std::string("callable"); }});
}

} // namespace lox
