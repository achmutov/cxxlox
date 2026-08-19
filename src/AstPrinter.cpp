#include <lox/AstPrinter.h>

#include <lox/Ast.h>

#include <sstream>
#include <string>
#include <string_view>

namespace lox {

auto AstPrinter::print(Expr &expr) -> std::string {
  return expr.accept(*this);
}

template <ExprType... Exprs>
auto AstPrinter::parenthesize(std::string_view lexeme, Exprs &...exprs)
    -> std::string {
  std::ostringstream ss;
  ss << '(' << lexeme;
  ((ss << ' ' << exprs.accept(*this)), ...);
  ss << ')';
  return ss.str();
}

auto AstPrinter::visitBinaryExpr(BinaryExpr &expr) -> std::string {
  return parenthesize(expr.op.lexeme, *expr.left, *expr.right);
}

auto AstPrinter::visitGroupingExpr(GroupingExpr &expr) -> std::string {
  return parenthesize("group", *expr.expression);
}

auto AstPrinter::visitUnaryExpr(UnaryExpr &expr) -> std::string {
  return parenthesize(expr.op.lexeme, *expr.right);
}

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

auto AstPrinter::visitLiteralExpr(LiteralExpr &expr) -> std::string {
  return expr.literal
      .transform([](ParseLiteral v) {
        return v.visit(overloads{
            [](std::string value) { return value; },
            [](bool value) { return std::string(value ? "true" : "false"); },
            [](double value) { return std::to_string(value); }});
      })
      .value_or("<empty>");
}

} // namespace lox
