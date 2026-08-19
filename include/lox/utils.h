#pragma once

#include <lox/TokenType.h>

#include <optional>
#include <string_view>

namespace lox {

void report(int line, std::string_view where, std::string_view message);

void error(int line, std::string_view message);

auto tokenType(TokenType type) -> std::optional<std::string_view>;

} // namespace lox
