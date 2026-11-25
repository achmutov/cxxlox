module;

#include <memory>
#include <string>
#include <variant>
#include <vector>

export module lox.ast;
import lox.token;

export namespace lox {

// Utility macros
#define EXPRS(EXPR, ...)                                                       \
  EXPR(TernaryExpr, __VA_ARGS__)                                               \
  EXPR(BinaryExpr, __VA_ARGS__)                                                \
  EXPR(UnaryExpr, __VA_ARGS__)                                                 \
  EXPR(GroupingExpr, __VA_ARGS__)                                              \
  EXPR(LiteralExpr, __VA_ARGS__)                                               \
  EXPR(VarExpr, __VA_ARGS__)                                                   \
  EXPR(AssignExpr, __VA_ARGS__)                                                \
  EXPR(LogicalExpr, __VA_ARGS__)                                               \
  EXPR(CallExpr, __VA_ARGS__)

#define STMTS(STMT, ...)                                                       \
  STMT(ExpressionStmt, __VA_ARGS__)                                            \
  STMT(ReplExpressionStmt, __VA_ARGS__)                                        \
  STMT(PrintStmt, __VA_ARGS__)                                                 \
  STMT(VarStmt, __VA_ARGS__)                                                   \
  STMT(BlockStmt, __VA_ARGS__)                                                 \
  STMT(IfStmt, __VA_ARGS__)                                                    \
  STMT(WhileStmt, __VA_ARGS__)                                                 \
  STMT(BreakStmt, __VA_ARGS__)                                                 \
  STMT(FunctionStmt, __VA_ARGS__)

// Forward declare constructs
#define CONSTRUCT_DECL(CONSTRUCT, ...) struct CONSTRUCT;
EXPRS(CONSTRUCT_DECL)
STMTS(CONSTRUCT_DECL)
#undef CONSTRUCT_DECL

#define VISITOR_ABS(EXPR, T) virtual auto visit(EXPR &) -> T = 0;

template <class T>
struct ExprVisitor {
  EXPRS(VISITOR_ABS, T)
};
template <class T>
struct StmtVisitor {
  STMTS(VISITOR_ABS, T)
};

#undef VISITOR_ABS

using ParseLiteral = std::variant<std::monostate, std::string, double, bool>;

template <typename Dummy, typename... Ts>
using MakeVariant = std::variant<Ts...>;

#define ENUMERATE(T, ...) , T
using ExprNode = MakeVariant<void EXPRS(ENUMERATE)>;
using StmtNode = MakeVariant<void STMTS(ENUMERATE)>;
#undef ENUMERATE

struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct TernaryExpr {
  ExprPtr cond, left, right;
};

struct BinaryExpr {
  Token op;
  ExprPtr left, right;
};

struct UnaryExpr {
  Token op;
  ExprPtr right;
};

struct GroupingExpr {
  ExprPtr expression;
};

struct LiteralExpr {
  ParseLiteral literal;
};

struct VarExpr {
  Token name;
};

struct AssignExpr {
  Token name;
  ExprPtr value;
};

struct LogicalExpr {
  Token op;
  ExprPtr left, right;
};

struct CallExpr {
  ExprPtr callee;
  std::vector<ExprPtr> args;
  Token rightParen;
};

struct ExpressionStmt {
  ExprPtr expression;
};

struct ReplExpressionStmt {
  ExprPtr expression;
};

struct PrintStmt {
  ExprPtr expression;
};

struct VarStmt {
  Token name;
  ExprPtr expression;
};

struct BlockStmt {
  std::vector<std::shared_ptr<Stmt>> stmts;
};

struct IfStmt {
  ExprPtr condition;
  StmtPtr thenBranch;
  StmtPtr elseBranch;
};

struct WhileStmt {
  ExprPtr condition;
  StmtPtr body;
};

struct BreakStmt {};

struct FunctionStmt {
  Token name;
  std::vector<Token> params;
  std::vector<std::shared_ptr<Stmt>> body;
};

struct Expr {
  ExprNode node;
};

struct Stmt {
  StmtNode node;
};
} // namespace lox
