module;

#include <cassert>
#include <concepts>
#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <print>
#include <span>
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
import InterpreterErrors;
import Environment;

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

template <typename F, typename Input, typename Output>
concept BinaryOperation = requires(F &f, Input a, Output b) {
  { std::invoke(f, a, b) } -> std::same_as<Output>;
};

export struct Interpreter : public Expr::Visitor<InterpreterObject>,
                            public Stmt::Visitor<void> {
  constexpr static auto binary_numeric_error = "Operands must be numbers";

  void interpret(const std::span<StmtPtr> stmts) {
    try {
      for (auto &stmt : stmts) {
        execute(*stmt);
      }
    } catch (InterpreterRuntimeException &ex) {
      std::cout << ex.what2() << "\n";
      hadRuntimeError = true;
    }
  }

  auto visitTernaryExpr(TernaryExpr &expr) -> InterpreterObject override {
    return isTruthy(evaluate(*expr.cond)).value ? evaluate(*expr.left)
                                                : evaluate(*expr.right);
  }

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
    return evaluate(*expr.expression);
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
    case TokenType::SLASH: {
      const auto *left_value = std::get_if<InterpreterNumber>(&left);
      const auto *right_value = std::get_if<InterpreterNumber>(&right);

      if (!left_value || !right_value)
        throw InterpreterTypeException(expr.op, binary_numeric_error);

      // check division
      if (std::abs(right_value->value) < std::numeric_limits<double>::epsilon())
        throw InterpreterZeroDivisionException{};

      return InterpreterNumber{left_value->value / right_value->value};
    }
    case TokenType::STAR:
      return interpretNumericalBinaryExpr(left, right, std::multiplies<>(),
                                          expr.op);
    case TokenType::MINUS:
      return interpretNumericalBinaryExpr(left, right, std::minus<>(), expr.op);
    case TokenType::PLUS: {
      INTERPRET_RETURN(left, right, std::plus, InterpreterNumber)
      INTERPRET_RETURN(left, right, std::plus, InterpreterString)

      // EVIL
      const auto result = std::visit(
          overloads{
              [](InterpreterNumber left,
                 InterpreterString &right) -> std::optional<InterpreterObject> {
                return InterpreterString{stringify(InterpreterObject{left}) +
                                         right.value};
              },
              [](InterpreterString &left,
                 InterpreterNumber right) -> std::optional<InterpreterObject> {
                return InterpreterString{left.value +
                                         stringify(InterpreterObject{right})};
              },
              [](const auto &, const auto &)
                  -> std::optional<InterpreterObject> { return std::nullopt; }},
          left, right);
      if (result.has_value())
        return result.value();

      throw InterpreterTypeException(expr.op,
                                     "Operands must be two numbers or two "
                                     "strings, or one number and one string.");
    }
    case TokenType::GREATER:
      return interpretNumericalBinaryExpr<InterpreterBoolean>(
          left, right, std::greater<>(), expr.op);
    case TokenType::GREATER_EQUAL:
      return interpretNumericalBinaryExpr<InterpreterBoolean>(
          left, right, std::greater_equal<>(), expr.op);
    case TokenType::LESS:
      return interpretNumericalBinaryExpr<InterpreterBoolean>(
          left, right, std::less<>(), expr.op);
    case TokenType::LESS_EQUAL:
      return interpretNumericalBinaryExpr<InterpreterBoolean>(
          left, right, std::less_equal<>(), expr.op);

    case TokenType::BANG_EQUAL:
      return !isEqual(left, right);
    case TokenType::EQUAL_EQUAL:
      return isEqual(left, right);

    case TokenType::COMMA:
      return right;

      // case TokenType::AND:
      // case TokenType::OR:
      break;
    default:
      panic("Invalid binary OP token type {}", expr.op.toString());
    }
  }

  auto visitVarExpr(VarExpr &expr) -> InterpreterObject override {
    return env.get(expr.name.lexeme);
  }

  auto visitAssignExpr(AssignExpr &expr) -> InterpreterObject override {
    auto value = evaluate(*expr.value);
    env.assign(expr.name.lexeme, value);
    return value;
  }

  auto visitLogicalExpr(LogicalExpr &expr) -> InterpreterObject override {
    auto left_obj = evaluate(*expr.left);
    switch (expr.op.type) {
    case TokenType::OR:
      if (isTruthy(left_obj).value)
        return left_obj;
      break;
    case TokenType::AND:
      if (!isTruthy(left_obj).value)
        return left_obj;
      break;
    default:
      panic("Invalid logical token: {}", expr.op.toString());
    }
    return evaluate(*expr.right);
  }

  void visitExpressionStmt(ExpressionStmt &stmt) override {
    evaluate(*stmt.expression);
  }
  void visitReplExpressionStmt(ReplExpressionStmt &stmt) override {
    std::println("{}", stringify(evaluate(*stmt.expression)));
  }
  void visitPrintStmt(PrintStmt &stmt) override {
    std::println("{}", stringify(evaluate(*stmt.expression)));
  }
  void visitVarStmt(VarStmt &stmt) override {
    const auto name = stmt.name.lexeme;
    const auto value = stmt.expression != nullptr ? evaluate(*stmt.expression)
                                                  : InterpreterNil{};

    env.define(name, value);
  }

  void visitBlockStmt(BlockStmt &stmt) override {
    auto previous = std::move(env);
    const auto _ = Defer([this, &previous]() { env = std::move(previous); });

    auto currentEnv = Environment(&previous);
    env = std::move(currentEnv);
    for (auto &stmt_ : stmt.stmts) {
      execute(*stmt_);
    }
  }

  void visitIfStmt(IfStmt &stmt) override {
    if (isTruthy(evaluate(*stmt.condition)).value)
      execute(*stmt.thenBranch);
    else
      execute(*stmt.elseBranch);
  }

  void visitWhileStmt(WhileStmt &stmt) override {
    while (isTruthy(evaluate(*stmt.condition)).value)
      execute(*stmt.body);
  }

private:
  Environment env;

  void execute(Stmt &stmt) {
    stmt.accept(*this);
  }
  auto evaluate(Expr &expr) -> InterpreterObject {
    return expr.accept(*this);
  }

  auto isTruthy(const InterpreterObject &object) -> InterpreterBoolean {
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

  template <typename R = InterpreterNumber, typename F>
    requires InterpretBinaryOperation<F, InterpreterNumber, R>
  auto interpretNumericalBinaryExpr(const InterpreterObject &left,
                                    const InterpreterObject &right, F &&f,
                                    const Token &token) -> R {
    const auto result = tryInterpretBinary<InterpreterNumber, R>(
        left, right, std::forward<decltype(f)>(f));

    if (!result.has_value())
      throw InterpreterTypeException(token, binary_numeric_error);

    return result.value();
  }
};
} // namespace lox
