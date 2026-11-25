module;

#include <memory>

export module lox.runtime;

export import :interpreter;
export import :environment;
export import :callable;
export import :object;
export import :builtins;
export import :function;

lox::runtime::Interpreter::Interpreter() {
  globals.define("clock", std::make_shared<ClockFunction>());
  env = &globals;
}

void lox::runtime::Interpreter::visit(FunctionStmt &stmt) {
  env->define(stmt.name.lexeme, std::make_shared<Function>(stmt));
}
