#pragma once

#include "tengine/ast.hpp"
#include "tengine/lexer.hpp"

#include <string>
#include <unordered_map>

namespace te {

enum class ValueType {
    Number,
    String,
    Boolean,
    Unknown,
    Error,
};

class TypeChecker {
public:
    void check(const Program& program);
    const std::vector<Diagnostic>& diagnostics() const noexcept;

private:
    ValueType checkExpression(const Expr& expression);
    void checkStatement(const Statement& statement);
    void checkFunction(const FunctionStatement& function);
    ValueType lookup(const std::string& name) const;
    void pushScope();
    void popScope();
    void error(const std::string& message);

    std::vector<std::unordered_map<std::string, ValueType>> scopes_;
    std::unordered_map<std::string, std::size_t> functions_;
    std::vector<Diagnostic> diagnostics_;
    bool insideFunction_{false};
};

std::string valueTypeName(ValueType type);

} // namespace te