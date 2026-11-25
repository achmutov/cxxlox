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
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module lox.runtime.interpreter;
import lox.ast;
import lox.token;
import lox.tokentype;
import lox.utils;
import lox.runtime.object;
import lox.runtime.errors;
import lox.runtime.environment;

#define INTERPRET_RETURN(LEFT, RIGHT, OP, ...)                                 \
  {                                                                            \
    const auto result = tryInterpretBinary<__VA_ARGS__>(LEFT, RIGHT, OP<>());  \
    if (result.has_value())                                                    \
      return result.value();                                                   \
  }

namespace lox::runtime {

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
concept InterpreterValue = IsVariantAlternative<T, Object>::value &&
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

export struct Interpreter : public Expr::Visitor<Object>,
                            public Stmt::Visitor<void> {
  constexpr static auto binary_numeric_error = "Operands must be numbers";

  void interpret(const std::span<StmtPtr> stmts) {
    try {
      for (auto &stmt : stmts) {
        execute(*stmt);
      }
    } catch (RuntimeException &ex) {
      std::cout << ex.what2() << "\n";
      hadRuntimeError = true;
    }
  }

  auto visitTernaryExpr(TernaryExpr &expr) -> Object override {
    return isTruthy(evaluate(*expr.cond)).value ? evaluate(*expr.left)
                                                : evaluate(*expr.right);
  }

  auto visitLiteralExpr(LiteralExpr &expr) -> Object override {
    return expr.literal.visit(overloads{
        [](std::string value) -> Object { return String{std::move(value)}; },
        [](double value) -> Object { return Number{value}; },
        [](bool value) -> Object { return Boolean{value}; },
        [](std::monostate) -> Object { return Nil{}; }});
  } // namespace lox

  auto visitGroupingExpr(GroupingExpr &expr) -> Object override {
    return evaluate(*expr.expression);
  }

  auto visitUnaryExpr(UnaryExpr &expr) -> Object override {
    auto right = evaluate(*expr.right);
    switch (expr.op.type) {
    case TokenType::MINUS:
      if (auto p = std::get_if<Number>(&right))
        return -*p;
      throw TypeException(expr.op, "Operand must be a number");
    case TokenType::BANG:
      return !isTruthy(right);
    default:
      // unreachable
      panic("Invalid unary token: {}", expr.op.toString());
    }
  }

  auto visitBinaryExpr(BinaryExpr &expr) -> Object override {
    auto left = evaluate(*expr.left);
    auto right = evaluate(*expr.right);
    switch (expr.op.type) {
    case TokenType::SLASH: {
      const auto *left_value = std::get_if<Number>(&left);
      const auto *right_value = std::get_if<Number>(&right);

      if (!left_value || !right_value)
        throw TypeException(expr.op, binary_numeric_error);

      // check division
      if (std::abs(right_value->value) < std::numeric_limits<double>::epsilon())
        throw ZeroDivisionException{};

      return Number{left_value->value / right_value->value};
    }
    case TokenType::STAR:
      return interpretNumericalBinaryExpr(left, right, std::multiplies<>(),
                                          expr.op);
    case TokenType::MINUS:
      return interpretNumericalBinaryExpr(left, right, std::minus<>(), expr.op);
    case TokenType::PLUS: {
      INTERPRET_RETURN(left, right, std::plus, Number)
      INTERPRET_RETURN(left, right, std::plus, String)

      // EVIL
      const auto result = std::visit(
          overloads{[](Number left, String &right) -> std::optional<Object> {
                      return String{stringify(Object{left}) + right.value};
                    },
                    [](String &left, Number right) -> std::optional<Object> {
                      return String{left.value + stringify(Object{right})};
                    },
                    [](const auto &, const auto &) -> std::optional<Object> {
                      return std::nullopt;
                    }},
          left, right);
      if (result.has_value())
        return result.value();

      throw TypeException(expr.op, "Operands must be two numbers or two "
                                   "strings, or one number and one string.");
    }
    case TokenType::GREATER:
      return interpretNumericalBinaryExpr<Boolean>(left, right,
                                                   std::greater<>(), expr.op);
    case TokenType::GREATER_EQUAL:
      return interpretNumericalBinaryExpr<Boolean>(
          left, right, std::greater_equal<>(), expr.op);
    case TokenType::LESS:
      return interpretNumericalBinaryExpr<Boolean>(left, right, std::less<>(),
                                                   expr.op);
    case TokenType::LESS_EQUAL:
      return interpretNumericalBinaryExpr<Boolean>(
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

  auto visitVarExpr(VarExpr &expr) -> Object override {
    return env.get(expr.name.lexeme);
  }

  auto visitAssignExpr(AssignExpr &expr) -> Object override {
    auto value = evaluate(*expr.value);
    env.assign(expr.name.lexeme, value);
    return value;
  }

  auto visitLogicalExpr(LogicalExpr &expr) -> Object override {
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

  auto visitCallExpr(CallExpr &expr) -> Object override {
    auto callee = evaluate(*expr.callee);
    auto args = expr.args |
                std::ranges::views::transform(
                    [this](const auto &v) { return evaluate(*v); }) |
                std::ranges::to<std::vector<Object>>();

    if (const auto *object = std::get_if<Callable>(&callee)) {
      // check arity
      {
        const auto arity = object->get()->arity();
        if (args.size() != arity)
          throw TypeException(expr.rightParen,
                              std::format("Expected {} arguments but got {}",
                                          arity, args.size()));
      }

      return object->get()->call(std::move(args));
    }

    throw TypeException(expr.rightParen,
                        "Can only call functions and classes.");
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
    const auto value =
        stmt.expression != nullptr ? evaluate(*stmt.expression) : Nil{};

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
    else if (stmt.elseBranch)
      execute(*stmt.elseBranch);
  }

  void visitWhileStmt(WhileStmt &stmt) override {
    while (isTruthy(evaluate(*stmt.condition)).value)
      try {
        execute(*stmt.body);
      } catch (BreakException &) {
        break;
      }
  }

  void visitBreakStmt(BreakStmt &stmt) override {
    (void)stmt;
    throw BreakException();
  }

private:
  Environment env;

  void execute(Stmt &stmt) {
    stmt.accept(*this);
  }
  auto evaluate(Expr &expr) -> Object {
    return expr.accept(*this);
  }

  auto isTruthy(const Object &object) -> Boolean {
    return Boolean(object.visit(overloads{
        [](const Boolean v) { return v.value; },
        [](const String &) { return true; }, [](const Number) { return true; },
        [](const Nil) { return false; },
        [](const Callable &) { return true; }}));
  }

  auto isEqual(const Object &left, const Object &right) -> Boolean {

    if (std::holds_alternative<Nil>(left) && std::holds_alternative<Nil>(right))
      return Boolean{true};

    if (std::holds_alternative<Nil>(left) || std::holds_alternative<Nil>(right))
      return Boolean{false};

    INTERPRET_RETURN(left, right, std::equal_to, Number, Boolean)
    INTERPRET_RETURN(left, right, std::equal_to, Boolean, Boolean)
    INTERPRET_RETURN(left, right, std::equal_to, String, Boolean)
    INTERPRET_RETURN(left, right, std::equal_to, Boolean)

    return Boolean{false};
  }

  template <typename T, typename R = T, typename F>
    requires InterpretBinaryOperation<F, T, R>
  auto tryInterpretBinary(const Object &left, const Object &right, F &&f)
      -> std::optional<R> {
    const auto *left_value = std::get_if<T>(&left);
    const auto *right_value = std::get_if<T>(&right);

    if (!left_value || !right_value)
      return std::nullopt;

    return R{std::invoke(f, left_value->value, right_value->value)};
  }

  template <typename R = Number, typename F>
    requires InterpretBinaryOperation<F, Number, R>
  auto interpretNumericalBinaryExpr(const Object &left, const Object &right,
                                    F &&f, const Token &token) -> R {
    const auto result = tryInterpretBinary<Number, R>(
        left, right, std::forward<decltype(f)>(f));

    if (!result.has_value())
      throw TypeException(token, binary_numeric_error);

    return result.value();
  }
};
} // namespace lox::runtime
