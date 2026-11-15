module;

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

export module InterpreterErrors;
import Token;

export namespace lox {

class InterpreterRuntimeException : public std::runtime_error {
public:
  explicit InterpreterRuntimeException(std::string &&msg)
      : std::runtime_error{msg} {}

  constexpr virtual auto name() -> std::string_view {
    return "RuntimeException";
  };

  virtual auto what2() -> std::string {
    return std::format("{}: {}", name(), what());
  }
};

class InterpreterTypeException : public InterpreterRuntimeException {
public:
  Token token;
  InterpreterTypeException(Token token, std::string &&msg)
      : InterpreterRuntimeException{std::move(msg)}, token{std::move(token)} {}

  constexpr auto name() -> std::string_view override {
    return "TypeException";
  }

  auto what2() -> std::string override {
    return std::format("{}: {} [line {}]", name(), what(), token.line);
  }
};

class InterpreterZeroDivisionException : public InterpreterRuntimeException {
public:
  InterpreterZeroDivisionException()
      : InterpreterRuntimeException{"division by zero"} {}

  constexpr auto name() -> std::string_view override {
    return "ZeroDivisionError";
  }
};

} // namespace lox
