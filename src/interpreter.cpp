#include "tengine/interpreter.hpp"

#include "t_runtime.h"

#include <cmath>
#include <optional>
#include <string_view>
#include <type_traits>

namespace te {

void Interpreter::execute(const Program& program) {
    scopes_.clear();
    scopes_.emplace_back();
    functions_.clear();
    for (const auto& statement : program) {
        if (const auto* function = std::get_if<FunctionStatement>(&statement->value)) {
            functions_[function->name] = function;
        }
    }
    for (const auto& statement : program) {
        if (execute(*statement).has_value()) {
            runtimeError("'return' tidak boleh berada di luar function");
            break;
        }
    }
}

const std::vector<Diagnostic>& Interpreter::diagnostics() const noexcept {
    return diagnostics_;
}

std::optional<Interpreter::Value> Interpreter::execute(const Statement& statement) {
    std::optional<Value> result;
    std::visit([this, &result](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, LetStatement>) {
            scopes_.back()[node.name] = evaluate(*node.initializer);
        } else if constexpr (std::is_same_v<T, PrintStatement>) {
            const auto value = evaluate(*node.expression);
            if (const auto* number = std::get_if<double>(&value)) te_runtime_print_number(*number);
            else if (const auto* text = std::get_if<std::string>(&value)) te_runtime_print_string(text->c_str());
            else if (const auto* boolean = std::get_if<bool>(&value)) te_runtime_print_boolean(*boolean ? 1 : 0);
        } else if constexpr (std::is_same_v<T, ExpressionStatement>) {
            static_cast<void>(evaluate(*node.expression));
        } else if constexpr (std::is_same_v<T, AssignStatement>) {
            for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
                if (it->contains(node.name)) {
                    (*it)[node.name] = evaluate(*node.value);
                    return;
                }
            }
            runtimeError("variabel '" + node.name + "' belum dideklarasikan");
        } else if constexpr (std::is_same_v<T, ReturnStatement>) {
            result = evaluate(*node.value);
        } else if constexpr (std::is_same_v<T, IfStatement>) {
            if (condition(*node.condition, "if")) {
                result = executeBlock(node.thenBranch, true);
            } else if (!node.elseBranch.empty()) {
                result = executeBlock(node.elseBranch, true);
            }
        } else if constexpr (std::is_same_v<T, WhileStatement>) {
            while (condition(*node.condition, "while")) {
                result = executeBlock(node.body, true);
                if (result.has_value()) break;
            }
        }
    }, statement.value);
    return result;
}

std::optional<Interpreter::Value> Interpreter::executeBlock(
    const Program& program, bool createScope) {
    if (createScope) scopes_.emplace_back();
    for (const auto& statement : program) {
        if (auto result = execute(*statement)) {
            if (createScope) scopes_.pop_back();
            return result;
        }
    }
    if (createScope) scopes_.pop_back();
    return std::nullopt;
}

Interpreter::Value Interpreter::evaluate(const Expr& expression) {
    return std::visit([this](const auto& node) -> Value {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            return node.value;
        } else if constexpr (std::is_same_v<T, VariableExpr>) {
            for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
                const auto found = it->find(node.name);
                if (found != it->end()) return found->second;
            }
            runtimeError("variabel '" + node.name + "' belum dideklarasikan");
            return {};
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return call(node);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            const auto left = evaluate(*node.left);
            if (node.op == TokenKind::AndAnd || node.op == TokenKind::OrOr) {
                if (!std::holds_alternative<bool>(left)) {
                    runtimeError("operator boolean hanya menerima boolean");
                    return {};
                }
                const bool leftValue = std::get<bool>(left);
                if (node.op == TokenKind::AndAnd && !leftValue) return false;
                if (node.op == TokenKind::OrOr && leftValue) return true;
                const auto right = evaluate(*node.right);
                if (!std::holds_alternative<bool>(right)) {
                    runtimeError("operator boolean hanya menerima boolean");
                    return {};
                }
                return std::get<bool>(right);
            }
            const auto right = evaluate(*node.right);
            switch (node.op) {
            case TokenKind::EqualEqual: return left == right;
            case TokenKind::BangEqual: return left != right;
            default: break;
            }
            if ((node.op == TokenKind::Plus || node.op == TokenKind::Minus ||
                 node.op == TokenKind::Star || node.op == TokenKind::Slash ||
                 node.op == TokenKind::Greater || node.op == TokenKind::GreaterEqual ||
                 node.op == TokenKind::Less || node.op == TokenKind::LessEqual) &&
                (!std::holds_alternative<double>(left) || !std::holds_alternative<double>(right))) {
                runtimeError("operator ini hanya menerima angka");
                return {};
            }
            const double lhs = std::get<double>(left);
            const double rhs = std::get<double>(right);
            switch (node.op) {
            case TokenKind::Plus: return lhs + rhs;
            case TokenKind::Minus: return lhs - rhs;
            case TokenKind::Star: return lhs * rhs;
            case TokenKind::Slash:
                if (std::abs(rhs) < 1e-12) {
                    runtimeError("pembagian dengan nol");
                    return {};
                }
                return lhs / rhs;
            case TokenKind::Greater: return lhs > rhs;
            case TokenKind::GreaterEqual: return lhs >= rhs;
            case TokenKind::Less: return lhs < rhs;
            case TokenKind::LessEqual: return lhs <= rhs;
            default: return {};
            }
        } else {
            const auto value = evaluate(*node.right);
            if (node.op == TokenKind::Bang) {
                if (!std::holds_alternative<bool>(value)) {
                    runtimeError("operator '!' hanya menerima boolean");
                    return {};
                }
                return !std::get<bool>(value);
            }
            if (!std::holds_alternative<double>(value)) {
                runtimeError("operator unary '-' hanya menerima angka");
                return {};
            }
            return -std::get<double>(value);
        }
    }, expression.value);
}

Interpreter::Value Interpreter::call(const CallExpr& callExpression) {
    if (const auto builtin = lookupBuiltin(callExpression.name)) {
        std::vector<Value> arguments;
        arguments.reserve(callExpression.arguments.size());
        for (const auto& argument : callExpression.arguments) {
            arguments.push_back(evaluate(*argument));
        }
        return callBuiltin(*builtin, arguments);
    }

    const auto function = functions_.find(callExpression.name);
    if (function == functions_.end()) {
        runtimeError("function '" + callExpression.name + "' belum dideklarasikan");
        return {};
    }
    const auto* declaration = function->second;
    if (declaration->parameters.size() != callExpression.arguments.size()) {
        runtimeError("function '" + callExpression.name + "' menerima " +
                     std::to_string(declaration->parameters.size()) + " argumen");
        return {};
    }

    std::vector<Value> arguments;
    arguments.reserve(callExpression.arguments.size());
    for (const auto& argument : callExpression.arguments) {
        arguments.push_back(evaluate(*argument));
    }

    scopes_.emplace_back();
    for (std::size_t index = 0; index < declaration->parameters.size(); ++index) {
        scopes_.back()[declaration->parameters[index]] = arguments[index];
    }
    const auto result = executeBlock(declaration->body, false);
    scopes_.pop_back();
    return result.value_or(Value{});
}

Interpreter::Value Interpreter::callBuiltin(
    BuiltinFunction builtin, const std::vector<Value>& arguments) {
    const auto invalidArity = [&](std::size_t expected) {
        if (arguments.size() != expected) {
            runtimeError("builtin '" + std::string(builtinName(builtin)) +
                         "' menerima " + std::to_string(expected) + " argumen");
            return true;
        }
        return false;
    };

    switch (builtin) {
    case BuiltinFunction::Len:
        if (invalidArity(1)) return {};
        if (const auto* text = std::get_if<std::string>(&arguments[0])) {
            return static_cast<double>(text->size());
        }
        runtimeError("builtin 'len' membutuhkan string");
        return {};
    case BuiltinFunction::Sqrt:
        if (invalidArity(1)) return {};
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            if (*number < 0) {
                runtimeError("builtin 'sqrt' tidak menerima angka negatif");
                return {};
            }
            return std::sqrt(*number);
        }
        runtimeError("builtin 'sqrt' membutuhkan angka");
        return {};
    case BuiltinFunction::Abs:
        if (invalidArity(1)) return {};
        if (const auto* number = std::get_if<double>(&arguments[0])) return std::abs(*number);
        runtimeError("builtin 'abs' membutuhkan angka");
        return {};
    case BuiltinFunction::TypeOf:
        if (invalidArity(1)) return {};
        if (std::holds_alternative<double>(arguments[0])) return std::string("number");
        if (std::holds_alternative<std::string>(arguments[0])) return std::string("string");
        if (std::holds_alternative<bool>(arguments[0])) return std::string("boolean");
        return std::string("void");
    }
    return {};
}

bool Interpreter::condition(const Expr& expression, const std::string& construct) {
    const auto value = evaluate(expression);
    if (const auto* boolean = std::get_if<bool>(&value)) return *boolean;
    runtimeError("kondisi '" + construct + "' harus boolean");
    return false;
}

void Interpreter::runtimeError(const std::string& message) {
    diagnostics_.push_back({{}, message});
}

} // namespace te
