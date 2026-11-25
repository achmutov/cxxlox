module;

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

export module lox.runtime:errors;
import lox.token;
import :object;

export namespace lox::runtime {
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
  ZeroDivisionException() : RuntimeException{"division by zero"} {}

  constexpr auto name() -> std::string_view override {
    return "ZeroDivisionError";
  }
};

class BreakException : public RuntimeException {
public:
  BreakException() : RuntimeException{"unenclosed 'break' keyword"} {}
  constexpr auto name() -> std::string_view override {
    return "BreakException";
  }
};

class ReturnException : public RuntimeException {
public:
  Object value;
  explicit ReturnException(Object value)
      : RuntimeException{"unenclosed 'return' keyword"},
        value(std::move(value)) {}
  constexpr auto name() -> std::string_view override {
    return "ReturnException";
  }
};

} // namespace lox::runtime
