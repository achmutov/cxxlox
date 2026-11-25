module;

#include <cstddef>
#include <ranges>
#include <utility>
#include <vector>

export module lox.runtime:function;
import :callable;
import :interpreter;
import :environment;

namespace lox::runtime {
struct Function : public CallableI {
  FunctionStmt declaration;

  explicit Function(FunctionStmt stmt) : declaration{std::move(stmt)} {}

  auto arity() -> std::size_t override {
    return declaration.params.size();
  }

  auto call(Interpreter &interpreter, std::vector<Object> args)
      -> Object override {
    auto env = Environment{interpreter.globals};
    for (auto a : std::ranges::views::zip(declaration.params, args))
      env.define(std::get<0>(a).lexeme, std::get<1>(a));

    interpreter.executeBlock(declaration.body, &env);
    return Nil{};
  }
};
} // namespace lox::runtime
