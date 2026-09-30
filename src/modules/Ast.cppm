module;

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

export module Ast;
import Token;

export namespace lox {

// Utility macros
#define EXPRS(PREDICATE, ...)                                                  \
  PREDICATE(Binary, __VA_ARGS__)                                               \
  PREDICATE(Unary, __VA_ARGS__)                                                \
  PREDICATE(Grouping, __VA_ARGS__)                                             \
  PREDICATE(Literal, __VA_ARGS__)
#define VISITOR_TYPES(PREDICATE, ...) PREDICATE(std::string, __VA_ARGS__)

// Forward declare expressions
#define EXPR_DECL(EXPR, ...) struct EXPR##Expr;
EXPRS(EXPR_DECL)
#undef EXPR_DECL

#define VISITOR_EXPR_ABS(EXPR, T)                                              \
  virtual auto visit##EXPR##Expr(EXPR##Expr &) -> T = 0;

template <class T>
struct Visitor {
  EXPRS(VISITOR_EXPR_ABS, T)
};

#undef VISITOR_EXPR_ABS

#define ACCEPT_DECL_ABS(T, ...) virtual auto accept(Visitor<T> &) -> T = 0;

struct Expr {
  VISITOR_TYPES(ACCEPT_DECL_ABS)
  virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

#undef ACCEPT_DECL_ABS

using ParseLiteral = std::variant<std::string, double, bool>;

#define EXPR_ACCEPT_T(T, EXPR)                                                 \
  auto accept(Visitor<T> &visitor) -> T override {                             \
    return visitor.visit##EXPR##Expr(*this);                                   \
  }

#define EXPR_ACCEPT(T) EXPR_ACCEPT_T(T, EXPR)

struct BinaryExpr : public Expr {
  Token op;
  ExprPtr left, right;
  BinaryExpr(Token op, ExprPtr left, ExprPtr right)
      : op{std::move(op)}, left(std::move(left)), right(std::move(right)) {}
  VISITOR_TYPES(EXPR_ACCEPT_T, Binary)
};

struct UnaryExpr : public Expr {
  Token op;
  ExprPtr right;
  UnaryExpr(Token op, ExprPtr right)
      : op{std::move(op)}, right(std::move(right)) {}
  VISITOR_TYPES(EXPR_ACCEPT_T, Unary)
};

struct GroupingExpr : public Expr {
  ExprPtr expression;
  explicit GroupingExpr(ExprPtr expression)
      : expression(std::move(expression)) {}

  VISITOR_TYPES(EXPR_ACCEPT_T, Grouping)
};

struct LiteralExpr : public Expr {
  std::optional<ParseLiteral> literal;
  explicit LiteralExpr(std::optional<ParseLiteral> literal)
      : literal{std::move(literal)} {}
  VISITOR_TYPES(EXPR_ACCEPT_T, Literal)
};

} // namespace lox
