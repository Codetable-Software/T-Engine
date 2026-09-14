#pragma once

#include <optional>
#include <string_view>

namespace te {

enum class BuiltinFunction {
    Len,
    Sqrt,
    Abs,
    TypeOf,
};

std::optional<BuiltinFunction> lookupBuiltin(std::string_view name);
std::string_view builtinName(BuiltinFunction builtin);

} // namespace te