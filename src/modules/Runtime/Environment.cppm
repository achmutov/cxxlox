module;
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

export module lox.runtime:environment;
import :object;
import lox.runtime.errors;
import lox.utils;

namespace lox::runtime {

export class Environment {
public:
  Environment() = default;
  explicit Environment(Environment *enclosing) : enclosing{enclosing} {}

  void define(std::string name, Object value) {
    values.insert({std::move(name), std::move(value)});
  }

  void assign(std::string_view name, Object value) {
    if (const auto it = values.find(name); it != values.end()) {
      it->second = std::move(value);
    } else if (enclosing != nullptr) {
      enclosing->assign(name, value);
    } else {
      undefinedVariable(name);
    }
  }

  auto get(std::string_view name) -> Object {
    if (const auto it = values.find(name); it != values.end())
      return it->second;
    if (enclosing != nullptr)
      return enclosing->get(name);
    undefinedVariable(name);
  }

private:
  std::unordered_map<std::string, Object, StringHash, std::equal_to<>> values;

  Environment *enclosing;

  [[noreturn]] void undefinedVariable(std::string_view name) {
    throw RuntimeException(std::format("Undefined variable '{}'.", name));
  }
};

} // namespace lox::runtime
