module;

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

export module lox.parser;
import lox.token;
import lox.ast;
import lox.tokentype;
import lox.utils;

// clang-format off

// program        -> declaration* EOF ;
// declaration    -> varDecl | statement ;
// statement      -> printStmt | ifStmt | whileStmt | forStmt | expressionStmt | replExpressionStmt | block | break ;
// block          -> "{" declaration* "}"
// printStmt      -> "print" expression ";" ;
// ifStmt         -> "if" "(" expression ")" statement ( "else" statement )? ;
// whileStmt      -> "while" "(" expression ")" statement ;
// forStmt        -> "for" "(" ( varDecl | exprStmt | ";" ) expression? : expression? ")" statement ;
// expressionStmt -> expression ";" ;
// replExpressionStmt
//                -> expression EOF;
// expression     -> comma ;
// comma          -> assignment ("," assignment)* ;
// assignment     -> IDENTIFIER "=" assignment
//                 | ternary
// ternary        -> logical_or ("?" ternary ":" ternary)? ;
// logical_or     -> logical_and ("or" logical_and)* ;
// logical_and    -> equality ("and" equality)* ;
// equality       -> comparison ("==" | "!=" comparison)* ;
// comparison     -> term ((">" | "<" | ">=" | "<=") term)* ;
// term           -> factor (("+" | "-") factor)* ;
// factor         -> unary (( "*" | "/" ) unary)* ;
// unary          -> ("!" | "-") unary | call
// call           -> primary ( "(" arguments? ")" )* ;
// arguments      -> assignment ("," assignment )* ","? ;
// primary        -> "true" | "false" | "nil" | NUMBER | STRING | grouping | IDENTIFIER;
// grouping       -> "(" expression ")" ;

// clang-format on

namespace lox {

template <class T>
concept TokenTypeType = std::same_as<TokenType, T>;

enum class BinaryExprPrecedence : std::uint8_t {
  COMPARISON,
  TERM,
  FACTOR,
  UNARY
};

namespace {
template <class... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};
} // namespace

export class ParseException : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

export class Parser {
  const std::span<const Token> tokens;
  std::size_t current{0};
  bool isRepl = false;
  int loopDepth = 0;

  // traversal
  auto declaration() -> StmtPtr {
    try {
      if (match({TokenType::VAR}))
        return varDeclaration();
      return statement();
    } catch (ParseException &ex) {
      synchronize();
      return nullptr;
    }
  }
  auto varDeclaration() -> StmtPtr {
    const Token name =
        consumeOrError(TokenType::IDENTIFIER, "Expect variable name.");
    ExprPtr initializer = nullptr;
    if (match({TokenType::EQUAL}))
      initializer = expression();
    consumeOrError(TokenType::SEMICOLON,
                   "Expect ';' after variable declaration.");
    return std::make_unique<VarStmt>(name, std::move(initializer));
  }
  auto statement() -> StmtPtr {
    if (match({TokenType::PRINT}))
      return printStatement();
    if (match({TokenType::LEFT_BRACE}))
      return block();
    if (match({TokenType::IF}))
      return ifStatement();
    if (match({TokenType::WHILE}))
      return whileStatement();
    if (match({TokenType::FOR}))
      return forStatement();
    if (match({TokenType::BREAK}))
      return breakStatement();
    return expressionStatement();
  }
  auto printStatement() -> StmtPtr {
    auto expr = expression();
    consumeOrError(TokenType::SEMICOLON, "Expect ';' after value.");
    return std::make_unique<PrintStmt>(std::move(expr));
  }
  auto block() -> StmtPtr {
    auto stmts = std::vector<StmtPtr>{};
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
      stmts.push_back(declaration());
    }
    consumeOrError(TokenType::RIGHT_BRACE,
                   "Expect '}' at the end of the block");
    return std::make_unique<BlockStmt>(std::move(stmts));
  }
  auto expressionStatement() -> StmtPtr {
    auto expr = expression();
    if (match({TokenType::SEMICOLON}))
      return std::make_unique<ExpressionStmt>(std::move(expr));
    if (isRepl && check(TokenType::EOF_))
      return std::make_unique<ReplExpressionStmt>(std::move(expr));
    throw error(peek(), "Expect ';' after value.");
  }
  auto ifStatement() -> StmtPtr {
    consumeOrError(TokenType::LEFT_PAREN, "Expect '(' after 'if'");
    auto conditional = expression();
    consumeOrError(TokenType::RIGHT_PAREN, "Expect ')' after condition");
    auto thenBranch = statement();
    auto elseBranch = match({TokenType::ELSE}) ? statement() : nullptr;
    return std::make_unique<IfStmt>(
        std::move(conditional), std::move(thenBranch), std::move(elseBranch));
  }
  auto whileStatement() -> StmtPtr {
    consumeOrError(TokenType::LEFT_PAREN, "Expect '(' after 'while'");
    auto condition = expression();
    consumeOrError(TokenType::RIGHT_PAREN, "Expect ')' after condition");
    auto body = loopBodyStatement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
  }
  auto forStatement() -> StmtPtr {
    // parse for clauses
    consumeOrError(TokenType::LEFT_PAREN, "Expect '(' after 'for'");
    auto initializer =
        !match({TokenType::SEMICOLON})
            ? match({TokenType::VAR}) ? varDeclaration() : expressionStatement()
            : nullptr;
    auto condition = check(TokenType::SEMICOLON) ? nullptr : expression();
    consumeOrError(TokenType::SEMICOLON, "Expect ';' after loop condition");
    auto increment = check(TokenType::RIGHT_PAREN) ? nullptr : expression();
    consumeOrError(TokenType::RIGHT_PAREN, "Expect ')' after for clauses");

    // parse body
    auto body = loopBodyStatement();

    // desugar for clauses
    {
      if (increment) {
        std::vector<StmtPtr> stmts;
        stmts.reserve(2);
        stmts.emplace_back(std::move(body));
        stmts.emplace_back(
            std::make_unique<ExpressionStmt>(std::move(increment)));
        body = std::make_unique<BlockStmt>(std::move(stmts));
      }

      if (!condition)
        condition = std::make_unique<LiteralExpr>(true);
      body = std::make_unique<WhileStmt>(std::move(condition), std::move(body));

      if (initializer) {
        std::vector<StmtPtr> stmts;
        stmts.reserve(2);
        stmts.emplace_back(std::move(initializer));
        stmts.emplace_back(std::move(body));
        body = std::make_unique<BlockStmt>(std::move(stmts));
      }
    }

    return body;
  }
  auto breakStatement() -> StmtPtr {
    if (loopDepth > 0) {
      consumeOrError(TokenType::SEMICOLON, "Expect ';' after break.");
      return std::make_unique<BreakStmt>();
    }
    throw error(previous(), "'break' is not enclosed in a loop");
  }
  auto loopBodyStatement() -> StmtPtr {
    loopDepth += 1;
    const auto _ = Defer([this]() { this->loopDepth -= 1; });
    return statement();
  }
  auto expression() -> ExprPtr {
    return comma();
  }
  auto comma() -> ExprPtr {
    return binaryExpr<&Parser::assignment>({TokenType::COMMA});
  }
  auto assignment() -> ExprPtr {
    auto expr = ternary();
    if (match({TokenType::EQUAL})) {
      const auto equals = previous();
      auto value = ternary();
      if (auto var_ptr = dynamic_cast<VarExpr *>(expr.get())) {
        return std::make_unique<AssignExpr>(var_ptr->name, std::move(value));
      }
      error(equals, "Invalid assignment target.");
    }
    return expr;
  }
  auto ternary() -> ExprPtr {
    auto expr = logicalOr();
    while (match({TokenType::QUESTION_MARK})) {
      auto left = ternary();
      consumeOrError(TokenType::COLON,
                     "Expect ':' after first expression in ternary.");
      auto right = ternary();
      expr = std::make_unique<TernaryExpr>(std::move(expr), std::move(left),
                                           std::move(right));
    }
    return expr;
  }

  auto logicalOr() -> ExprPtr {
    return binaryExpr<&Parser::logicalAnd, LogicalExpr>({TokenType::OR});
  };
  auto logicalAnd() -> ExprPtr {
    return binaryExpr<&Parser::equality, LogicalExpr>({TokenType::AND});
  }

  auto equality() -> ExprPtr {
    return binaryExpr<&Parser::comparison>(
        {TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL});
  }
  auto comparison() -> ExprPtr {
    return binaryExpr<&Parser::term>({TokenType::LESS, TokenType::LESS_EQUAL,
                                      TokenType::GREATER,
                                      TokenType::GREATER_EQUAL});
  }
  auto term() -> ExprPtr {
    return binaryExpr<&Parser::factor>({TokenType::MINUS, TokenType::PLUS});
  }
  auto factor() -> ExprPtr {
    return binaryExpr<&Parser::unary>({TokenType::STAR, TokenType::SLASH});
  }
  auto unary() -> ExprPtr {
    if (match({TokenType::BANG, TokenType::MINUS})) {
      const auto op = previous();
      auto right = unary();
      return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return call();
  }
  auto call() -> ExprPtr {
    auto expr = primary();
    while (match({TokenType::LEFT_PAREN}))
      expr = finishCall(std::move(expr));
    return expr;
  }
  auto finishCall(ExprPtr callee) -> ExprPtr {
    std::vector<ExprPtr> args;

    if (!check(TokenType::RIGHT_PAREN))
      do {
        if (args.size() >= 255)
          throw error(peek(), "Can't have more than 255 arguments");
        args.emplace_back(assignment());
      } while (match({TokenType::COMMA}) && !check(TokenType::RIGHT_PAREN));

    auto rightParen = consumeOrError(TokenType::RIGHT_PAREN,
                                     "Expect ')' after function arguments");

    return std::make_unique<CallExpr>(std::move(callee), std::move(args),
                                      std::move(rightParen));
  }
  auto primary() -> ExprPtr {
    if (match({TokenType::FALSE}))
      return std::make_unique<LiteralExpr>(false);
    if (match({TokenType::TRUE}))
      return std::make_unique<LiteralExpr>(true);
    if (match({TokenType::NIL}))
      return std::make_unique<LiteralExpr>(std::monostate{});

    if (match({TokenType::NUMBER, TokenType::STRING})) {
      const auto token = previous();
      const auto literal =
          optional_expect(token.literal, "Token {} is supposed to have literal",
                          token.toString());

      const auto parse_literal = literal.visit(
          overloads{[](auto const &arg) -> ParseLiteral { return arg; }});

      return std::make_unique<LiteralExpr>(parse_literal);
    }

    if (match({TokenType::IDENTIFIER}))
      return std::make_unique<VarExpr>(previous());

    if (match({TokenType::LEFT_PAREN})) {
      auto expr = expression();
      consumeOrError(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
      return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw error(peek(), "Expect expression.");
  }

  template <auto nextExpr, class T = BinaryExpr>
  auto binaryExpr(const std::span<const TokenType> ts) -> ExprPtr {
    auto expr = std::invoke(nextExpr, this);
    while (match(ts)) {
      const auto op = previous();
      auto right = std::invoke(nextExpr, this);
      expr = std::make_unique<T>(op, std::move(expr), std::move(right));
    }
    return expr;
  }
  auto match(const std::span<const TokenType> ts) -> bool {
    if (std::ranges::any_of(ts, [this](TokenType t) { return check(t); })) {
      advance();
      return true;
    }
    return false;
  }
  auto advance() -> Token {
    if (!isAtEnd())
      current++;

    return previous();
  }

  // error handling
  auto consumeOrError(TokenType tokenType, std::string_view error_message)
      -> Token {
    if (check(tokenType))
      return advance();

    throw error(peek(), error_message);
  }
  auto error(const Token &token, std::string_view error_message)
      -> ParseException {
    reportError(token, error_message);
    return ParseException{std::string(error_message)};
  }
  void reportError(const Token &token, std::string_view message) {
    if (token.type == TokenType::EOF_)
      report(token.line, "at end", message);
    else
      report(token.line, std::format("at '{}'", token.lexeme), message);
  }
  void synchronize() {
    // skip first erroneous token
    advance();

    // while (advance().type != EOF_) ?
    while (!isAtEnd()) {
      if (previous().type == TokenType::SEMICOLON)
        return;

      switch (peek().type) {
      case TokenType::CLASS:
      case TokenType::FUN:
      case TokenType::VAR:
      case TokenType::FOR:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::PRINT:
      case TokenType::RETURN:
        return;
      default:
        advance();
      }
    }
  }

  [[nodiscard]] auto check(const TokenType type) const -> bool {
    return type == peek().type;
  }

  [[nodiscard]] auto isAtEnd() const -> bool {
    return peek().type == TokenType::EOF_;
  }

  [[nodiscard]] auto peek() const -> Token {
    return tokens.at(current);
  }

  [[nodiscard]] auto previous() const -> Token {
    return tokens.at(current - 1);
  }

public:
  // TODO: should be non-zero length
  // ideally, use superconstructing super elider
  explicit Parser(std::span<const Token> tokens, bool isRepl = false)
      : tokens{tokens}, isRepl{isRepl} {}
  auto parse() -> std::vector<StmtPtr> {
    auto stmts = std::vector<StmtPtr>{};
    while (!isAtEnd()) {
      stmts.push_back(declaration());
    }
    return stmts;
  }
};

} // namespace lox
