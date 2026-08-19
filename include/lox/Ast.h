#pragma once

#include <lox/Token.h>

#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace lox {

template <class T>
struct Visitor;

#define EXPRS(PREDICATE, ...)                                                  \
  PREDICATE(Binary, __VA_ARGS__)                                               \
  PREDICATE(Unary, __VA_ARGS__)                                                \
  PREDICATE(Grouping, __VA_ARGS__)                                             \
  PREDICATE(Literal, __VA_ARGS__)
#define VISITOR_TYPES(PREDICATE) PREDICATE(std::string)

#define ACCEPT_DECL(T)        virtual auto accept(Visitor<T> &) -> T override;
#define ACCEPT_DECL_ABS(T)    virtual auto accept(Visitor<T> &) -> T = 0;
#define VISITOR_EXPR(EXPR, T) virtual auto visit##EXPR##Expr(EXPR##Expr &) -> T;
#define VISITOR_EXPR_ABS(EXPR, T)                                              \
  virtual auto visit##EXPR##Expr(EXPR##Expr &) -> T = 0;

struct Expr {
  VISITOR_TYPES(ACCEPT_DECL_ABS)
  virtual ~Expr() = default;
};

using ParseLiteral = std::variant<std::string, double, bool>;

struct BinaryExpr : public Expr {
  Token op;
  std::unique_ptr<Expr> left, right;
  BinaryExpr(Token, std::unique_ptr<Expr>, std::unique_ptr<Expr>);
  VISITOR_TYPES(ACCEPT_DECL)
};
struct UnaryExpr : public Expr {
  Token op;
  std::unique_ptr<Expr> right;
  UnaryExpr(Token, std::unique_ptr<Expr>);
  VISITOR_TYPES(ACCEPT_DECL)
};
struct GroupingExpr : public Expr {
  std::unique_ptr<Expr> expression;
  GroupingExpr(std::unique_ptr<Expr>);
  VISITOR_TYPES(ACCEPT_DECL)
};
struct LiteralExpr : public Expr {
  std::optional<ParseLiteral> literal;
  LiteralExpr(std::optional<ParseLiteral>);
  VISITOR_TYPES(ACCEPT_DECL)
};

template <class T>
struct Visitor {
  EXPRS(VISITOR_EXPR_ABS, T)
};

#undef ACCEPT_DECL
#undef ACCEPT_DECL_ABS
#undef VISITOR_EXPR_ABS

} // namespace lox
