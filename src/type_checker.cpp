#include "tengine/type_checker.hpp"

#include "tengine/builtins.hpp"

#include <type_traits>

namespace te {

std::string valueTypeName(ValueType type) {
    switch (type) {
    case ValueType::Number: return "number";
    case ValueType::String: return "string";
    case ValueType::Boolean: return "boolean";
    case ValueType::Unknown: return "unknown";
    case ValueType::Error: return "error";
    }
    return "unknown";
}

void TypeChecker::check(const Program& program) {
    scopes_.clear();
    scopes_.emplace_back();
    functions_.clear();
    for (const auto& statement : program) {
        if (const auto* function = std::get_if<FunctionStatement>(&statement->value)) {
            if (functions_.contains(function->name)) {
                error("function '" + function->name + "' sudah dideklarasikan");
            } else {
                functions_[function->name] = function->parameters.size();
            }
        }
    }
    for (const auto& statement : program) checkStatement(*statement);
}

const std::vector<Diagnostic>& TypeChecker::diagnostics() const noexcept {
    return diagnostics_;
}

void TypeChecker::checkStatement(const Statement& statement) {
    std::visit([this](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, LetStatement>) {
            if (scopes_.back().contains(node.name)) {
                error("variabel '" + node.name + "' sudah dideklarasikan");
                return;
            }
            const auto type = checkExpression(*node.initializer);
            if (type != ValueType::Error) scopes_.back()[node.name] = type;
        } else if constexpr (std::is_same_v<T, AssignStatement>) {
            const auto currentType = lookup(node.name);
            if (currentType == ValueType::Error) {
                error("variabel '" + node.name + "' belum dideklarasikan");
                return;
            }
            const auto actual = checkExpression(*node.value);
            if (actual != ValueType::Error && currentType != ValueType::Unknown &&
                actual != currentType) {
                error("assignment ke '" + node.name + "' mengharapkan " +
                      valueTypeName(currentType) + ", bukan " + valueTypeName(actual));
            }
        } else if constexpr (std::is_same_v<T, FunctionStatement>) {
            checkFunction(node);
        } else if constexpr (std::is_same_v<T, ReturnStatement>) {
            if (!insideFunction_) {
                error("'return' hanya boleh digunakan di dalam function");
            } else {
                static_cast<void>(checkExpression(*node.value));
            }
        } else if constexpr (std::is_same_v<T, IfStatement>) {
            const auto condition = checkExpression(*node.condition);
            if (condition != ValueType::Boolean && condition != ValueType::Unknown &&
                condition != ValueType::Error) {
                error("kondisi 'if' harus boolean");
            }
            pushScope();
            for (const auto& child : node.thenBranch) checkStatement(*child);
            popScope();
            if (!node.elseBranch.empty()) {
                pushScope();
                for (const auto& child : node.elseBranch) checkStatement(*child);
                popScope();
            }
        } else if constexpr (std::is_same_v<T, WhileStatement>) {
            const auto condition = checkExpression(*node.condition);
            if (condition != ValueType::Boolean && condition != ValueType::Unknown &&
                condition != ValueType::Error) {
                error("kondisi 'while' harus boolean");
            }
            pushScope();
            for (const auto& child : node.body) checkStatement(*child);
            popScope();
        } else {
            static_cast<void>(checkExpression(*node.expression));
        }
    }, statement.value);
}

void TypeChecker::checkFunction(const FunctionStatement& function) {
    const bool wasInsideFunction = insideFunction_;
    insideFunction_ = true;
    pushScope();
    for (const auto& parameter : function.parameters) {
        if (scopes_.back().contains(parameter)) {
            error("parameter '" + parameter + "' duplikat");
        } else {
            scopes_.back()[parameter] = ValueType::Unknown;
        }
    }
    for (const auto& statement : function.body) checkStatement(*statement);
    popScope();
    insideFunction_ = wasInsideFunction;
}

ValueType TypeChecker::checkExpression(const Expr& expression) {
    return std::visit([this](const auto& node) -> ValueType {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            if (std::holds_alternative<double>(node.value)) return ValueType::Number;
            if (std::holds_alternative<std::string>(node.value)) return ValueType::String;
            if (std::holds_alternative<bool>(node.value)) return ValueType::Boolean;
            return ValueType::Error;
        } else if constexpr (std::is_same_v<T, VariableExpr>) {
            const auto type = lookup(node.name);
            if (type == ValueType::Error) {
                error("variabel '" + node.name + "' belum dideklarasikan");
                return ValueType::Error;
            }
            return type;
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            const auto type = checkExpression(*node.right);
            if (type == ValueType::Error) return type;
            if (type == ValueType::Unknown) return ValueType::Unknown;
            const auto expected = node.op == TokenKind::Bang
                ? ValueType::Boolean : ValueType::Number;
            if (type != expected) {
                error("operator unary mengharapkan " + valueTypeName(expected) +
                      ", bukan " + valueTypeName(type));
                return ValueType::Error;
            }
            return expected;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            if (const auto builtin = lookupBuiltin(node.name)) {
                const auto expected = *builtin == BuiltinFunction::TypeOf
                    ? ValueType::String
                    : (*builtin == BuiltinFunction::Len ||
                       *builtin == BuiltinFunction::Sqrt ||
                       *builtin == BuiltinFunction::Abs
                           ? ValueType::Number : ValueType::Error);
                const std::size_t expectedArity = 1;
                if (node.arguments.size() != expectedArity) {
                    error("builtin '" + std::string(builtinName(*builtin)) +
                          "' mengharapkan 1 argumen");
                    return ValueType::Error;
                }
                const auto argumentType = checkExpression(*node.arguments[0]);
                if (*builtin == BuiltinFunction::Len &&
                    argumentType != ValueType::String && argumentType != ValueType::Unknown) {
                    error("builtin 'len' membutuhkan string");
                    return ValueType::Error;
                }
                if ((*builtin == BuiltinFunction::Sqrt || *builtin == BuiltinFunction::Abs) &&
                    argumentType != ValueType::Number && argumentType != ValueType::Unknown) {
                    error("builtin '" + std::string(builtinName(*builtin)) +
                          "' membutuhkan angka");
                    return ValueType::Error;
                }
                return expected;
            }
            const auto function = functions_.find(node.name);
            if (function == functions_.end()) {
                error("function '" + node.name + "' belum dideklarasikan");
                return ValueType::Error;
            }
            if (function->second != node.arguments.size()) {
                error("function '" + node.name + "' mengharapkan " +
                      std::to_string(function->second) + " argumen, bukan " +
                      std::to_string(node.arguments.size()));
                return ValueType::Error;
            }
            for (const auto& argument : node.arguments) {
                static_cast<void>(checkExpression(*argument));
            }
            return ValueType::Unknown;
        } else {
            const auto left = checkExpression(*node.left);
            const auto right = checkExpression(*node.right);
            if (left == ValueType::Error || right == ValueType::Error) return ValueType::Error;
            if (node.op == TokenKind::AndAnd || node.op == TokenKind::OrOr) {
                if (left == ValueType::Unknown || right == ValueType::Unknown) {
                    return ValueType::Unknown;
                }
                if (left != ValueType::Boolean || right != ValueType::Boolean) {
                    error("operator boolean membutuhkan dua boolean");
                    return ValueType::Error;
                }
                return ValueType::Boolean;
            }
            if (left == ValueType::Unknown || right == ValueType::Unknown) {
                return ValueType::Unknown;
            }
            if (node.op == TokenKind::EqualEqual || node.op == TokenKind::BangEqual) {
                if (left != right) {
                    error("perbandingan membutuhkan dua nilai dengan tipe sama");
                    return ValueType::Error;
                }
                return ValueType::Boolean;
            }
            if (node.op == TokenKind::Greater || node.op == TokenKind::GreaterEqual ||
                node.op == TokenKind::Less || node.op == TokenKind::LessEqual ||
                node.op == TokenKind::Plus || node.op == TokenKind::Minus ||
                node.op == TokenKind::Star || node.op == TokenKind::Slash) {
                if (left != ValueType::Number || right != ValueType::Number) {
                    error("operator ini membutuhkan dua angka");
                    return ValueType::Error;
                }
                return (node.op == TokenKind::Plus || node.op == TokenKind::Minus ||
                        node.op == TokenKind::Star || node.op == TokenKind::Slash)
                    ? ValueType::Number : ValueType::Boolean;
            }
            return ValueType::Error;
        }
    }, expression.value);
}

ValueType TypeChecker::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        const auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return ValueType::Error;
}

void TypeChecker::pushScope() {
    scopes_.emplace_back();
}

void TypeChecker::popScope() {
    scopes_.pop_back();
}

void TypeChecker::error(const std::string& message) {
    diagnostics_.push_back({{}, message});
}

} // namespace te