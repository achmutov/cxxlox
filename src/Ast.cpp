#include <lox/Ast.h>

#include <lox/Token.h>

#include <memory>
#include <optional>
#include <utility>

namespace lox {

#define EXPR_ACCEPT_T(EXPR, T)                                                 \
  auto EXPR##Expr::accept(Visitor<T> &visitor) -> T {                          \
    return visitor.visit##EXPR##Expr(*this);                                   \
  }

#define EXPR_ACCEPT(T) EXPRS(EXPR_ACCEPT_T, T)

VISITOR_TYPES(EXPR_ACCEPT)

BinaryExpr::BinaryExpr(Token op, std::unique_ptr<Expr> left,
                       std::unique_ptr<Expr> right)
    : op{std::move(op)}, left(std::move(left)), right(std::move(right)) {}

UnaryExpr::UnaryExpr(Token op, std::unique_ptr<Expr> right)
    : op{std::move(op)}, right(std::move(right)) {}

GroupingExpr::GroupingExpr(std::unique_ptr<Expr> expression)
    : expression(std::move(expression)) {}

LiteralExpr::LiteralExpr(std::optional<ParseLiteral> literal)
    : literal{std::move(literal)} {}

} // namespace lox
