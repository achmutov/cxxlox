module;

#include <cstddef>
#include <vector>

export module lox.runtime:callable;

export namespace lox::runtime {

struct Object;
struct Interpreter;

struct CallableI {
  virtual auto arity() -> std::size_t = 0;
  virtual auto call(Interpreter &interpreter, std::vector<Object> args)
      -> Object = 0;
  virtual ~CallableI() = default;
};

} // namespace lox::runtime
