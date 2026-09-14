#pragma once

#include "tengine/ast.hpp"
#include "tengine/builtins.hpp"
#include "tengine/lexer.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace te {

class Interpreter {
public:
    void execute(const Program& program);
    const std::vector<Diagnostic>& diagnostics() const noexcept;

private:
    using Value = TokenValue;

    Value evaluate(const Expr& expression);
    std::optional<Value> execute(const Statement& statement);
    std::optional<Value> executeBlock(const Program& program, bool createScope);
    Value call(const CallExpr& call);
    Value callBuiltin(BuiltinFunction builtin, const std::vector<Value>& arguments);
    bool condition(const Expr& expression, const std::string& construct);
    void runtimeError(const std::string& message);

    std::vector<std::unordered_map<std::string, Value>> scopes_;
    std::unordered_map<std::string, const FunctionStatement*> functions_;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace te
