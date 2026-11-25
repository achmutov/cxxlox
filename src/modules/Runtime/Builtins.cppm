module;

#include <cstddef>
#include <ctime>
#include <vector>

export module lox.runtime:builtins;

import :callable;
import :object;

namespace lox::runtime {

struct Object;
struct ClockFunction : public CallableI {
  auto arity() -> std::size_t override {
    return 0;
  };
  auto call(Interpreter &interpreter, std::vector<Object> args)
      -> Object override {
    (void)interpreter;
    (void)args;

    return Number{static_cast<double>(time(nullptr))};
  }
};
} // namespace lox::runtime
