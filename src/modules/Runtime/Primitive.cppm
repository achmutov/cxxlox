module;
#include <string>
export module lox.runtime.primitive;

export namespace lox::runtime {
struct Nil {};

struct String {
  std::string value;
};

struct Number {
  double value;

public:
  auto operator-() {
    value = -value;
    return *this;
  }
};

struct Boolean {
  bool value;

public:
  auto operator!() {
    value = !value;
    return *this;
  }
};
} // namespace lox::runtime
