module;

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

export module lox.runtime:object;
export import lox.runtime.primitive;
import lox.utils;

export namespace lox {

template <typename T>
concept ValuedInterpreterObject = requires(const T &x) { x.value; };

namespace runtime {

struct CallableI;
using Callable = std::shared_ptr<CallableI>;

struct Object {
  using Variant = std::variant<Nil, String, Number, Boolean, Callable>;
  Variant variant;

  template <typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, Object> &&
             std::constructible_from<Variant, T &&>)
  Object(T &&value) : variant{std::forward<T>(value)} {} // NOLINT
};

constexpr auto interpreterTypeName(const Object &object) -> std::string_view {
  using namespace std::string_view_literals;
  return object.variant.visit(
      overloads{[](const String &) { return "string"sv; },
                [](const Nil) { return "nil"sv; },
                [](const Boolean) { return "boolean"sv; },
                [](const Number &) { return "number"sv; },
                [](const Callable &) { return "callable"sv; }});
}

auto stringify(Object obj) -> std::string {
  return obj.variant.visit(
      overloads{[](String value) { return std::move(value.value); },
                [](const Nil) { return std::string{"nil"}; },
                [](const Boolean value) {
                  return std::string{value.value ? "true" : "false"};
                },
                [](const Number value) { return std::to_string(value.value); },
                [](const Callable &) { return std::string("callable"); }});
}
} // namespace runtime
} // namespace lox
