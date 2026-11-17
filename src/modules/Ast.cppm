module;

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

export module Ast;
import Token;
import InterpreterObject;

export namespace lox {

// Utility macros
#define EXPRS(PREDICATE, ...)                                                  \
  PREDICATE(TernaryExpr, __VA_ARGS__)                                          \
  PREDICATE(BinaryExpr, __VA_ARGS__)                                           \
  PREDICATE(UnaryExpr, __VA_ARGS__)                                            \
  PREDICATE(GroupingExpr, __VA_ARGS__)                                         \
  PREDICATE(LiteralExpr, __VA_ARGS__)                                          \
  PREDICATE(VarExpr, __VA_ARGS__)                                              \
  PREDICATE(AssignExpr, __VA_ARGS__)                                           \
  PREDICATE(LogicalExpr, __VA_ARGS__)

#define STMTS(STMT, ...)                                                       \
  STMT(ExpressionStmt, __VA_ARGS__)                                            \
  STMT(ReplExpressionStmt, __VA_ARGS__)                                        \
  STMT(PrintStmt, __VA_ARGS__)                                                 \
  STMT(VarStmt, __VA_ARGS__)                                                   \
  STMT(BlockStmt, __VA_ARGS__)                                                 \
  STMT(IfStmt, __VA_ARGS__)

// Forward declare constructs
#define CONSTRUCT_DECL(CONSTRUCT, ...) struct CONSTRUCT;
EXPRS(CONSTRUCT_DECL)
STMTS(CONSTRUCT_DECL)
#undef CONSTRUCT_DECL

#define VISITOR_EXPR_TYPES(PREDICATE, ...)                                     \
  PREDICATE(std::string, __VA_ARGS__)                                          \
  PREDICATE(InterpreterObject, __VA_ARGS__)
#define VISITOR_STMT_TYPES(STMT, ...) STMT(void, __VA_ARGS__)

#define VISITOR_ABS(EXPR, T)    virtual auto visit##EXPR(EXPR &) -> T = 0;
#define ACCEPT_DECL_ABS(T, ...) virtual auto accept(Visitor<T> &) -> T = 0;

struct Expr {
  template <class T>
  struct Visitor {
    EXPRS(VISITOR_ABS, T)
  };

  VISITOR_EXPR_TYPES(ACCEPT_DECL_ABS)
  virtual ~Expr() = default;
};

struct Stmt {
  template <class T>
  struct Visitor {
    STMTS(VISITOR_ABS, T)
  };
  VISITOR_STMT_TYPES(ACCEPT_DECL_ABS)
  virtual ~Stmt() = default;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

#undef VISITOR_ABS
#undef ACCEPT_DECL_ABS

using ParseLiteral = std::variant<std::monostate, std::string, double, bool>;

#define CONSTRUCT_ACCEPT_T(T, CONSTRUCT)                                       \
  auto accept(Visitor<T> &visitor) -> T override {                             \
    return visitor.visit##CONSTRUCT(*this);                                    \
  }

#define CONSTRUCT_ACCEPT(T) CONSTRUCT_ACCEPT_T(T, CONSTRUCT)

struct TernaryExpr : public Expr {
  ExprPtr cond, left, right;
  TernaryExpr(ExprPtr cond, ExprPtr left, ExprPtr right)
      : cond{std::move(cond)}, left{std::move(left)}, right{std::move(right)} {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, TernaryExpr)
};

struct BinaryExpr : public Expr {
  Token op;
  ExprPtr left, right;
  BinaryExpr(Token op, ExprPtr left, ExprPtr right)
      : op{std::move(op)}, left(std::move(left)), right(std::move(right)) {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, BinaryExpr)
};

struct UnaryExpr : public Expr {
  Token op;
  ExprPtr right;
  UnaryExpr(Token op, ExprPtr right)
      : op{std::move(op)}, right(std::move(right)) {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, UnaryExpr)
};

struct GroupingExpr : public Expr {
  ExprPtr expression;
  explicit GroupingExpr(ExprPtr expression)
      : expression(std::move(expression)) {}

  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, GroupingExpr)
};

struct LiteralExpr : public Expr {
  ParseLiteral literal;
  explicit LiteralExpr(ParseLiteral literal) : literal{std::move(literal)} {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, LiteralExpr)
};

struct VarExpr : public Expr {
  Token name;
  explicit VarExpr(Token name) : name{std::move(name)} {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, VarExpr)
};

struct AssignExpr : public Expr {
  Token name;
  ExprPtr value;
  explicit AssignExpr(Token name, ExprPtr value)
      : name{std::move(name)}, value{std::move(value)} {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, AssignExpr)
};

struct LogicalExpr : public Expr {
  Token op;
  ExprPtr left, right;
  explicit LogicalExpr(Token op, ExprPtr left, ExprPtr right)
      : op{std::move(op)}, left(std::move(left)), right(std::move(right)) {}
  VISITOR_EXPR_TYPES(CONSTRUCT_ACCEPT_T, LogicalExpr)
};

struct ExpressionStmt : public Stmt {
  ExprPtr expression;
  explicit ExpressionStmt(ExprPtr expression)
      : expression{std::move(expression)} {}
  VISITOR_STMT_TYPES(CONSTRUCT_ACCEPT_T, ExpressionStmt)
};

struct ReplExpressionStmt : public Stmt {
  ExprPtr expression;
  explicit ReplExpressionStmt(ExprPtr expression)
      : expression{std::move(expression)} {}
  VISITOR_STMT_TYPES(CONSTRUCT_ACCEPT_T, ReplExpressionStmt)
};

struct PrintStmt : public Stmt {
  ExprPtr expression;
  explicit PrintStmt(ExprPtr expression) : expression{std::move(expression)} {}
  VISITOR_STMT_TYPES(CONSTRUCT_ACCEPT_T, PrintStmt)
};

struct VarStmt : public Stmt {
  Token name;
  ExprPtr expression;
  explicit VarStmt(Token name, ExprPtr expression)
      : name{std::move(name)}, expression{std::move(expression)} {}
  VISITOR_STMT_TYPES(CONSTRUCT_ACCEPT_T, VarStmt)
};

struct BlockStmt : public Stmt {
  std::vector<StmtPtr> stmts;
  explicit BlockStmt(std::vector<StmtPtr> stmts) : stmts{std::move(stmts)} {}
  VISITOR_STMT_TYPES(CONSTRUCT_ACCEPT_T, BlockStmt)
};

struct IfStmt : public Stmt {
  ExprPtr condition;
  StmtPtr thenBranch;
  StmtPtr elseBranch;
  explicit IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
      : condition{std::move(condition)}, thenBranch{std::move(thenBranch)},
        elseBranch{std::move(elseBranch)} {}
  VISITOR_STMT_TYPES(CONSTRUCT_ACCEPT_T, IfStmt)
};

} // namespace lox
