module;
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

export module Interpreter.Environment;
import Interpreter.Object;
import Interpreter.Errors;
import utils;

namespace lox {

export class Environment {
public:
  Environment() = default;
  explicit Environment(Environment *enclosing) : enclosing{enclosing} {}

  void define(std::string name, InterpreterObject value) {
    values.insert({std::move(name), std::move(value)});
  }

  void assign(std::string_view name, InterpreterObject value) {
    if (const auto it = values.find(name); it != values.end()) {
      it->second = std::move(value);
    } else if (enclosing != nullptr) {
      enclosing->assign(name, value);
    } else {
      undefinedVariable(name);
    }
  }

  auto get(std::string_view name) -> InterpreterObject {
    if (const auto it = values.find(name); it != values.end())
      return it->second;
    if (enclosing != nullptr)
      return enclosing->get(name);
    undefinedVariable(name);
  }

private:
  std::unordered_map<std::string, InterpreterObject, StringHash,
                     std::equal_to<>>
      values;

  Environment *enclosing;

  [[noreturn]] void undefinedVariable(std::string_view name) {
    throw interpreter::RuntimeException(
        std::format("Undefined variable '{}'.", name));
  }
};

} // namespace lox
