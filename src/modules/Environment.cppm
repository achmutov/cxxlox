module;
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

export module Environment;
import InterpreterObject;
import InterpreterErrors;
import utils;

namespace lox {

export class Environment {
public:
  void define(std::string name, InterpreterObject value) {
    values.insert({std::move(name), std::move(value)});
  }

  void assign(std::string_view name, InterpreterObject value) {
    if (const auto it = values.find(name); it != values.end()) {
      it->second = std::move(value);
    } else {
      undefinedVariable(name);
    }
  }

  auto get(std::string_view name) -> InterpreterObject {
    if (const auto it = values.find(name); it != values.end()) {
      return it->second;
    }
    undefinedVariable(name);
  }

private:
  std::unordered_map<std::string, InterpreterObject, StringHash,
                     std::equal_to<>>
      values;

  [[noreturn]] void undefinedVariable(std::string_view name) {
    throw InterpreterRuntimeException(
        std::format("Undefined variable '{}'.", name));
  }
};

} // namespace lox
