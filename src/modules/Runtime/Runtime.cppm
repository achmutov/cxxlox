module;

#include <memory>

export module lox.runtime;

export import :interpreter;
export import :environment;
export import :callable;
export import :object;
export import :builtins;

lox::runtime::Interpreter::Interpreter() {
  globals.define("clock", std::make_shared<ClockFunction>());
  env = &globals;
}
