#pragma once

#include <lox/Ast.h>

#include <concepts>
#include <string>
#include <string_view>

namespace lox {

template <class T>
concept ExprType = std::same_as<T, Expr>;

struct AstPrinter : public Visitor<std::string> {
  EXPRS(VISITOR_EXPR, std::string)
  auto print(Expr &) -> std::string;

  template <ExprType... Exprs>
  auto parenthesize(std::string_view, Exprs &...) -> std::string;
};

} // namespace lox
