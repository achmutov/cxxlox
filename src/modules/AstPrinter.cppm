module;

#include <concepts>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

export module AstPrinter;
import Ast;

namespace lox {

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

template <class T>
concept ExprType = std::same_as<T, Expr>;

export struct AstPrinter : public Visitor<std::string> {
  auto print(Expr &expr) -> std::string {
    return expr.accept(*this);
  }

  auto visitTernaryExpr(TernaryExpr &expr) -> std::string override {
    return parenthesize("ternary", *expr.cond, *expr.left, *expr.right);
  }

  auto visitBinaryExpr(BinaryExpr &expr) -> std::string override {
    return parenthesize(expr.op.lexeme, *expr.left, *expr.right);
  }

  auto visitGroupingExpr(GroupingExpr &expr) -> std::string override {
    return parenthesize("group", *expr.expression);
  }

  auto visitUnaryExpr(UnaryExpr &expr) -> std::string override {
    return parenthesize(expr.op.lexeme, *expr.right);
  }

  template <ExprType... Exprs>
  auto parenthesize(std::string_view lexeme, Exprs &...exprs) -> std::string {
    std::ostringstream ss;
    ss << '(' << lexeme;
    ((ss << ' ' << exprs.accept(*this)), ...);
    ss << ')';
    return ss.str();
  }

  auto visitLiteralExpr(LiteralExpr &expr) -> std::string override {
    return expr.literal.visit(overloads{
        [](std::monostate) -> std::string { return "<empty>"; },
        [](std::string value) { return value; },
        [](bool value) { return std::string(value ? "true" : "false"); },
        [](double value) { return std::to_string(value); }});
  }
};

} // namespace lox
