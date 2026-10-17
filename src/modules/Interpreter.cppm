module;

#include <cassert>
#include <concepts>
#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

export module Interpreter;
import Ast;
import Token;
import TokenType;
import utils;
import InterpreterObject;

#define INTERPRET_RETURN(LEFT, RIGHT, OP, ...)                                 \
  {                                                                            \
    const auto result = tryInterpretBinary<__VA_ARGS__>(LEFT, RIGHT, OP<>());  \
    if (result.has_value())                                                    \
      return result.value();                                                   \
  }

namespace lox {

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};

} // namespace

template <typename T, typename Variant>
struct IsVariantAlternative;

template <typename T, typename... Alternatives>
struct IsVariantAlternative<T, std::variant<Alternatives...>>
    : std::bool_constant<(std::same_as<T, Alternatives> || ...)> {};

template <typename T>
concept InterpreterValue = IsVariantAlternative<T, InterpreterObject>::value &&
                           requires(const T &object) { object.value; };

template <typename F, typename T, typename R>
concept InterpretBinaryOperation =
    InterpreterValue<T> && InterpreterValue<R> &&
    requires(F &operation, const T &lhs, const T &rhs) {
      {
        std::invoke(operation, lhs.value, rhs.value)
      } -> std::same_as<decltype(R::value)>;
    };

export class InterpreterTypeException : public std::runtime_error {
  Token token;

public:
  InterpreterTypeException(Token token, const std::string &&msg)
      : std::runtime_error{msg}, token{std::move(token)} {}
};

template <typename F, typename Input, typename Output>
concept BinaryOperation = requires(F &f, Input a, Output b) {
  { std::invoke(f, a, b) } -> std::same_as<Output>;
};

export struct Interpreter : public Visitor<InterpreterObject> {
  auto visitLiteralExpr(LiteralExpr &expr) -> InterpreterObject override {
    return expr.literal.visit(overloads{
        [](std::string value) -> InterpreterObject {
          return InterpreterString{std::move(value)};
        },
        [](double value) -> InterpreterObject {
          return InterpreterNumber{value};
        },
        [](bool value) -> InterpreterObject {
          return InterpreterBoolean{value};
        },
        [](std::monostate) -> InterpreterObject { return InterpreterNil{}; }});
  } // namespace lox

  auto visitGroupingExpr(GroupingExpr &expr) -> InterpreterObject override {
    return evaluate(expr);
  }

  auto visitUnaryExpr(UnaryExpr &expr) -> InterpreterObject override {
    auto right = evaluate(*expr.right);
    switch (expr.op.type) {
    case TokenType::MINUS:
      if (auto p = std::get_if<InterpreterNumber>(&right))
        return -*p;
      throw InterpreterTypeException(expr.op, "Operand must be a number");
    case TokenType::BANG:
      return !isTruthy(right);
    default:
      // unreachable
      panic("Invalid unary token: {}", expr.op.toString());
    }
  }

  auto visitBinaryExpr(BinaryExpr &expr) -> InterpreterObject override {
    auto left = evaluate(*expr.left);
    auto right = evaluate(*expr.right);
    switch (expr.op.type) {
    case TokenType::SLASH:
      return interpretBinary<InterpreterNumber>(left, right, std::divides<>(),
                                                expr.op);
    case TokenType::STAR:
      return interpretBinary<InterpreterNumber>(left, right,
                                                std::multiplies<>(), expr.op);
    case TokenType::MINUS:
      return interpretBinary<InterpreterNumber>(left, right, std::minus<>(),
                                                expr.op);
    case TokenType::PLUS:
      INTERPRET_RETURN(left, right, std::plus, InterpreterNumber)
      INTERPRET_RETURN(left, right, std::plus, InterpreterString)
      throw InterpreterTypeException(
          expr.op, "Operands must be two numbers or two strings.");
    case TokenType::GREATER:
      return interpretBinary<InterpreterNumber, InterpreterBoolean>(
          left, right, std::greater<>(), expr.op);
    case TokenType::GREATER_EQUAL:
      return interpretBinary<InterpreterNumber, InterpreterBoolean>(
          left, right, std::greater_equal<>(), expr.op);
    case TokenType::LESS:
      return interpretBinary<InterpreterNumber, InterpreterBoolean>(
          left, right, std::less<>(), expr.op);
    case TokenType::LESS_EQUAL:
      return interpretBinary<InterpreterNumber, InterpreterBoolean>(
          left, right, std::less_equal<>(), expr.op);

    case TokenType::BANG_EQUAL:
      return !isEqual(left, right);
    case TokenType::EQUAL_EQUAL:
      return isEqual(left, right);

      // case TokenType::AND:
      // case TokenType::OR:
      break;
    default:
      panic("Invalid binary OP token type", expr.op.toString());
    }
  }

private:
  auto evaluate(Expr &expr) -> InterpreterObject {
    return expr.accept(*this);
  }

  auto isTruthy(InterpreterObject &object) -> InterpreterBoolean {
    return InterpreterBoolean(object.visit(
        overloads{[](const InterpreterBoolean v) { return v.value; },
                  [](const InterpreterString &) { return true; },
                  [](const InterpreterNumber) { return true; },
                  [](const InterpreterNil) { return false; }}));
  }

  auto isEqual(const InterpreterObject &left, const InterpreterObject &right)
      -> InterpreterBoolean {

    if (std::holds_alternative<InterpreterNil>(left) &&
        std::holds_alternative<InterpreterNil>(right))
      return InterpreterBoolean{true};

    if (std::holds_alternative<InterpreterNil>(left) ||
        std::holds_alternative<InterpreterNil>(right))
      return InterpreterBoolean{false};

    INTERPRET_RETURN(left, right, std::equal_to, InterpreterNumber,
                     InterpreterBoolean)
    INTERPRET_RETURN(left, right, std::equal_to, InterpreterBoolean,
                     InterpreterBoolean)
    INTERPRET_RETURN(left, right, std::equal_to, InterpreterString,
                     InterpreterBoolean)
    INTERPRET_RETURN(left, right, std::equal_to, InterpreterBoolean)

    return InterpreterBoolean{false};
  }

  template <typename T, typename R = T, typename F>
    requires InterpretBinaryOperation<F, T, R>
  auto tryInterpretBinary(const InterpreterObject &left,
                          const InterpreterObject &right, F &&f)
      -> std::optional<R> {
    const auto *left_value = std::get_if<T>(&left);
    const auto *right_value = std::get_if<T>(&right);

    if (!left_value || !right_value)
      return std::nullopt;

    return R{std::invoke(f, left_value->value, right_value->value)};
  }

  template <typename T, typename R = T, typename F>
    requires InterpretBinaryOperation<F, T, R>
  auto interpretBinary(const InterpreterObject &left,
                       const InterpreterObject &right, F &&f,
                       const Token &token) -> R {
    const auto result =
        tryInterpretBinary<T, R>(left, right, std::forward<decltype(f)>(f));

    if (!result.has_value())
      throw InterpreterTypeException(token, "Operands must be numbers");

    return result.value();
  }
};
} // namespace lox
