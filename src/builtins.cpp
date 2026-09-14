#include "tengine/builtins.hpp"

namespace te {

std::optional<BuiltinFunction> lookupBuiltin(std::string_view name) {
    if (name == "len") return BuiltinFunction::Len;
    if (name == "sqrt") return BuiltinFunction::Sqrt;
    if (name == "abs") return BuiltinFunction::Abs;
    if (name == "typeOf") return BuiltinFunction::TypeOf;
    return std::nullopt;
}

std::string_view builtinName(BuiltinFunction builtin) {
    switch (builtin) {
    case BuiltinFunction::Len: return "len";
    case BuiltinFunction::Sqrt: return "sqrt";
    case BuiltinFunction::Abs: return "abs";
    case BuiltinFunction::TypeOf: return "typeOf";
    }
    return "unknown";
}

} // namespace te