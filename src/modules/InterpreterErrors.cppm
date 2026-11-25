module;

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

export module lox.interpreter.errors;
import lox.token;

export namespace lox::interpreter {
class RuntimeException : public std::runtime_error {
public:
  explicit RuntimeException(std::string &&msg) : std::runtime_error{msg} {}

  constexpr virtual auto name() -> std::string_view {
    return "RuntimeException";
  };

  virtual auto what2() -> std::string {
    return std::format("{}: {}", name(), what());
  }
};

class TypeException : public RuntimeException {
public:
  Token token;
  TypeException(Token token, std::string &&msg)
      : RuntimeException{std::move(msg)}, token{std::move(token)} {}

  constexpr auto name() -> std::string_view override {
    return "TypeException";
  }

  auto what2() -> std::string override {
    return std::format("{}: {} [line {}]", name(), what(), token.line);
  }
};

class ZeroDivisionException : public RuntimeException {
public:
  ZeroDivisionException() : interpreter::RuntimeException{"division by zero"} {}

  constexpr auto name() -> std::string_view override {
    return "ZeroDivisionError";
  }
};

class BreakException : public interpreter::RuntimeException {
public:
  BreakException()
      : interpreter::RuntimeException{"unenclosed 'break' keyword"} {}
  constexpr auto name() -> std::string_view override {
    return "BreakException";
  }
};

} // namespace lox::interpreter
