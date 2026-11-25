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

namespace interpreter {
struct Nil {};
struct String {
  std::string value;
};
struct Number {
  double value;

public:
  auto operator-() {
    value = -value;
    return *this;
  }
};
struct Boolean {
  bool value;

public:
  auto operator!() {
    value = !value;
    return *this;
  }
};
struct CallableI;
using Callable = std::shared_ptr<CallableI>;

using Object =
    std::variant<interpreter::Nil, interpreter::String, interpreter::Number,
                 interpreter::Boolean, interpreter::Callable>;
struct CallableI {
  virtual auto arity() -> std::size_t = 0;
  virtual auto call(std::vector<Object> args) -> Object = 0;
};
constexpr auto interpreterTypeName(const Object &object) -> std::string_view {
  using namespace std::string_view_literals;
  return object.visit(
      lox::detail::overloads{[](const String &) { return "string"sv; },
                             [](const Nil) { return "nil"sv; },
                             [](const Boolean) { return "boolean"sv; },
                             [](const Number &) { return "number"sv; },
                             [](const Callable &) { return "callable"sv; }});
}

auto stringify(Object obj) -> std::string {
  return obj.visit(lox::detail::overloads{
      [](String value) { return std::move(value.value); },
      [](const Nil) { return std::string{"nil"}; },
      [](const Boolean value) {
        return std::string{value.value ? "true" : "false"};
      },
      [](const Number value) { return std::to_string(value.value); },
      [](const Callable &) { return std::string("callable"); }});
}
} // namespace interpreter
} // namespace lox
