module;

#include <memory>
#include <string>
#include <utility>
#include <variant>

export module Ast;
import Token;

export namespace lox {

// Utility macros
#define EXPRS(PREDICATE, ...)                                                  \
  PREDICATE(TernaryExpr, __VA_ARGS__)                                          \
  PREDICATE(BinaryExpr, __VA_ARGS__)                                           \
  PREDICATE(UnaryExpr, __VA_ARGS__)                                            \
  PREDICATE(GroupingExpr, __VA_ARGS__)                                         \
  PREDICATE(LiteralExpr, __VA_ARGS__)
#define VISITOR_TYPES(PREDICATE, ...) PREDICATE(std::string, __VA_ARGS__)

// Forward declare expressions
#define EXPR_DECL(EXPR, ...) struct EXPR;
EXPRS(EXPR_DECL)
#undef EXPR_DECL

#define VISITOR_EXPR_ABS(EXPR, T) virtual auto visit##EXPR(EXPR &) -> T = 0;

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

using ParseLiteral = std::variant<std::monostate, std::string, double, bool>;

#define EXPR_ACCEPT_T(T, EXPR)                                                 \
  auto accept(Visitor<T> &visitor) -> T override {                             \
    return visitor.visit##EXPR(*this);                                         \
  }

#define EXPR_ACCEPT(T) EXPR_ACCEPT_T(T, EXPR)

struct TernaryExpr : public Expr {
  ExprPtr cond, left, right;
  TernaryExpr(ExprPtr cond, ExprPtr left, ExprPtr right)
      : cond{std::move(cond)}, left{std::move(left)}, right{std::move(right)} {}
  VISITOR_TYPES(EXPR_ACCEPT_T, TernaryExpr)
};

struct BinaryExpr : public Expr {
  Token op;
  ExprPtr left, right;
  BinaryExpr(Token op, ExprPtr left, ExprPtr right)
      : op{std::move(op)}, left(std::move(left)), right(std::move(right)) {}
  VISITOR_TYPES(EXPR_ACCEPT_T, BinaryExpr)
};

struct UnaryExpr : public Expr {
  Token op;
  ExprPtr right;
  UnaryExpr(Token op, ExprPtr right)
      : op{std::move(op)}, right(std::move(right)) {}
  VISITOR_TYPES(EXPR_ACCEPT_T, UnaryExpr)
};

struct GroupingExpr : public Expr {
  ExprPtr expression;
  explicit GroupingExpr(ExprPtr expression)
      : expression(std::move(expression)) {}

  VISITOR_TYPES(EXPR_ACCEPT_T, GroupingExpr)
};

struct LiteralExpr : public Expr {
  ParseLiteral literal;
  explicit LiteralExpr(ParseLiteral literal) : literal{std::move(literal)} {}
  VISITOR_TYPES(EXPR_ACCEPT_T, LiteralExpr)
};

} // namespace lox
