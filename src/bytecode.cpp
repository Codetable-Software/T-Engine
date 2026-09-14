#include "tengine/bytecode.hpp"

#include "t_runtime.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace te {

BytecodeProgram BytecodeCompiler::compile(const Program& program) {
    program_ = {};
    diagnostics_.clear();
    functionIndices_.clear();
    names_.clear();
    nameTable_.clear();

    for (const auto& statement : program) {
        if (const auto* function = std::get_if<FunctionStatement>(&statement->value)) {
            if (functionIndices_.contains(function->name)) {
                error("function '" + function->name + "' sudah dideklarasikan");
            } else {
                functionIndices_[function->name] = program_.functions.size();
                program_.functions.push_back({function->name, function->parameters, {}});
            }
        }
    }

    FunctionContext mainContext{program_.main, nullptr, true};
    current_ = &mainContext;
    compileBlock(program);
    emit(OpCode::Halt);
    program_.main = std::move(mainContext.chunk);

    for (const auto& statement : program) {
        if (const auto* function = std::get_if<FunctionStatement>(&statement->value)) {
            compileFunction(*function);
        }
    }
    return program_;
}

const std::vector<Diagnostic>& BytecodeCompiler::diagnostics() const noexcept {
    return diagnostics_;
}

void BytecodeCompiler::compileFunction(const FunctionStatement& function) {
    const auto index = functionIndices_.at(function.name);
    FunctionContext context{program_.functions[index].chunk, &program_.functions[index], false};
    current_ = &context;
    compileBlock(function.body);
    emit(OpCode::Constant, addConstant(std::monostate{}));
    emit(OpCode::Return);
    program_.functions[index].chunk = std::move(context.chunk);
}

void BytecodeCompiler::compileBlock(const Program& program) {
    for (const auto& statement : program) compileStatement(*statement);
}

void BytecodeCompiler::compileStatement(const Statement& statement) {
    std::visit([this](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, LetStatement>) {
            compileExpression(*node.initializer);
            emit(OpCode::DefineName, addName(node.name));
        } else if constexpr (std::is_same_v<T, AssignStatement>) {
            compileExpression(*node.value);
            emit(OpCode::StoreName, addName(node.name));
        } else if constexpr (std::is_same_v<T, PrintStatement>) {
            compileExpression(*node.expression);
            emit(OpCode::Print);
        } else if constexpr (std::is_same_v<T, ExpressionStatement>) {
            compileExpression(*node.expression);
            emit(OpCode::Pop);
        } else if constexpr (std::is_same_v<T, ReturnStatement>) {
            compileExpression(*node.value);
            emit(OpCode::Return);
        } else if constexpr (std::is_same_v<T, FunctionStatement>) {
            // Function bodies are compiled in a separate pass.
        } else if constexpr (std::is_same_v<T, IfStatement>) {
            compileExpression(*node.condition);
            const auto falseJump = emit(OpCode::JumpIfFalse);
            emit(OpCode::Pop);
            compileBlock(node.thenBranch);
            if (!node.elseBranch.empty()) {
                const auto endJump = emit(OpCode::Jump);
                patchJump(falseJump, current_->chunk.code.size());
                emit(OpCode::Pop);
                compileBlock(node.elseBranch);
                patchJump(endJump, current_->chunk.code.size());
            } else {
                patchJump(falseJump, current_->chunk.code.size());
                emit(OpCode::Pop);
            }
        } else if constexpr (std::is_same_v<T, WhileStatement>) {
            const auto loopStart = current_->chunk.code.size();
            compileExpression(*node.condition);
            const auto exitJump = emit(OpCode::JumpIfFalse);
            emit(OpCode::Pop);
            compileBlock(node.body);
            emit(OpCode::Jump, static_cast<int>(loopStart));
            patchJump(exitJump, current_->chunk.code.size());
            emit(OpCode::Pop);
        }
    }, statement.value);
}

void BytecodeCompiler::compileExpression(const Expr& expression) {
    std::visit([this](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            emit(OpCode::Constant, addConstant(node.value));
        } else if constexpr (std::is_same_v<T, VariableExpr>) {
            emit(OpCode::LoadName, addName(node.name));
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            compileExpression(*node.right);
            emit(node.op == TokenKind::Bang ? OpCode::Not : OpCode::Negate);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            if (node.op == TokenKind::AndAnd) {
                compileExpression(*node.left);
                const auto falseJump = emit(OpCode::JumpIfFalse);
                emit(OpCode::Pop);
                compileExpression(*node.right);
                patchJump(falseJump, current_->chunk.code.size());
                return;
            }
            if (node.op == TokenKind::OrOr) {
                compileExpression(*node.left);
                const auto trueJump = emit(OpCode::JumpIfTrue);
                emit(OpCode::Pop);
                compileExpression(*node.right);
                patchJump(trueJump, current_->chunk.code.size());
                return;
            }
            compileExpression(*node.left);
            compileExpression(*node.right);
            switch (node.op) {
            case TokenKind::Plus: emit(OpCode::Add); break;
            case TokenKind::Minus: emit(OpCode::Subtract); break;
            case TokenKind::Star: emit(OpCode::Multiply); break;
            case TokenKind::Slash: emit(OpCode::Divide); break;
            case TokenKind::EqualEqual: emit(OpCode::Equal); break;
            case TokenKind::BangEqual: emit(OpCode::NotEqual); break;
            case TokenKind::Greater: emit(OpCode::Greater); break;
            case TokenKind::GreaterEqual: emit(OpCode::GreaterEqual); break;
            case TokenKind::Less: emit(OpCode::Less); break;
            case TokenKind::LessEqual: emit(OpCode::LessEqual); break;
            default: error("operator tidak didukung compiler bytecode"); break;
            }
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            if (const auto builtin = lookupBuiltin(node.name)) {
                for (const auto& argument : node.arguments) compileExpression(*argument);
                emit(OpCode::CallBuiltin, static_cast<int>(*builtin),
                     static_cast<int>(node.arguments.size()));
                return;
            }
            const auto function = functionIndices_.find(node.name);
            if (function == functionIndices_.end()) {
                error("function '" + node.name + "' belum dideklarasikan");
                emit(OpCode::Constant, addConstant(std::monostate{}));
                return;
            }
            for (const auto& argument : node.arguments) compileExpression(*argument);
            emit(OpCode::Call, static_cast<int>(function->second),
                 static_cast<int>(node.arguments.size()));
        }
    }, expression.value);
}

int BytecodeCompiler::addConstant(TokenValue value) {
    current_->chunk.constants.push_back(std::move(value));
    return static_cast<int>(current_->chunk.constants.size() - 1);
}

int BytecodeCompiler::addName(const std::string& name) {
    // Names are stored in each chunk's constant pool because functions have
    // independent chunks and constant indices.
    return addConstant(name);
}

std::size_t BytecodeCompiler::emit(OpCode op, int operand, int operand2) {
    current_->chunk.code.push_back({op, operand, operand2});
    return current_->chunk.code.size() - 1;
}

void BytecodeCompiler::patchJump(std::size_t instruction, std::size_t target) {
    current_->chunk.code[instruction].operand = static_cast<int>(target);
}

void BytecodeCompiler::error(const std::string& message) {
    diagnostics_.push_back({{}, message});
}

void BytecodeVM::execute(const BytecodeProgram& program) {
    program_ = &program;
    stack_.clear();
    frames_.clear();
    globals_.clear();
    diagnostics_.clear();
    frames_.push_back({0, 0, 0, {}, true});
    runFrame();
}

const std::vector<Diagnostic>& BytecodeVM::diagnostics() const noexcept {
    return diagnostics_;
}

void BytecodeVM::runFrame() {
    while (!frames_.empty() && diagnostics_.empty()) {
        auto& frame = frames_.back();
        const Chunk& chunk = frame.isMain
            ? program_->main
            : program_->functions[frame.functionIndex].chunk;
        if (frame.ip >= chunk.code.size()) {
            frames_.pop_back();
            continue;
        }

        const auto instruction = chunk.code[frame.ip++];
        switch (instruction.op) {
        case OpCode::Constant: push(constant(chunk, instruction.operand)); break;
        case OpCode::LoadName: {
            const auto name = std::get<std::string>(constant(chunk, instruction.operand));
            push(loadName(name));
            break;
        }
        case OpCode::DefineName: {
            const auto name = std::get<std::string>(constant(chunk, instruction.operand));
            defineName(name, pop());
            break;
        }
        case OpCode::StoreName: {
            const auto name = std::get<std::string>(constant(chunk, instruction.operand));
            storeName(name, pop());
            break;
        }
        case OpCode::Add:
        case OpCode::Subtract:
        case OpCode::Multiply:
        case OpCode::Divide:
        case OpCode::Equal:
        case OpCode::NotEqual:
        case OpCode::Greater:
        case OpCode::GreaterEqual:
        case OpCode::Less:
        case OpCode::LessEqual:
        case OpCode::And:
        case OpCode::Or: {
            const auto right = pop();
            const auto left = pop();
            if (instruction.op == OpCode::Equal) push(left == right);
            else if (instruction.op == OpCode::NotEqual) push(left != right);
            else if (instruction.op == OpCode::And || instruction.op == OpCode::Or) {
                if (!std::holds_alternative<bool>(left) || !std::holds_alternative<bool>(right)) {
                    runtimeError("operator boolean hanya menerima boolean");
                    break;
                }
                push(instruction.op == OpCode::And
                    ? std::get<bool>(left) && std::get<bool>(right)
                    : std::get<bool>(left) || std::get<bool>(right));
            } else if (!std::holds_alternative<double>(left) ||
                       !std::holds_alternative<double>(right)) {
                runtimeError("operator bytecode membutuhkan dua angka");
            } else {
                const auto lhs = std::get<double>(left);
                const auto rhs = std::get<double>(right);
                switch (instruction.op) {
                case OpCode::Add: push(lhs + rhs); break;
                case OpCode::Subtract: push(lhs - rhs); break;
                case OpCode::Multiply: push(lhs * rhs); break;
                case OpCode::Divide:
                    if (std::abs(rhs) < 1e-12) runtimeError("pembagian dengan nol");
                    else push(lhs / rhs);
                    break;
                case OpCode::Greater: push(lhs > rhs); break;
                case OpCode::GreaterEqual: push(lhs >= rhs); break;
                case OpCode::Less: push(lhs < rhs); break;
                case OpCode::LessEqual: push(lhs <= rhs); break;
                default: break;
                }
            }
            break;
        }
        case OpCode::Negate: {
            const auto value = pop();
            if (!std::holds_alternative<double>(value)) runtimeError("unary '-' membutuhkan angka");
            else push(-std::get<double>(value));
            break;
        }
        case OpCode::Not: {
            const auto value = pop();
            if (!std::holds_alternative<bool>(value)) runtimeError("unary '!' membutuhkan boolean");
            else push(!std::get<bool>(value));
            break;
        }
        case OpCode::Print: {
            const auto value = pop();
            if (const auto* number = std::get_if<double>(&value)) te_runtime_print_number(*number);
            else if (const auto* text = std::get_if<std::string>(&value)) te_runtime_print_string(text->c_str());
            else if (const auto* boolean = std::get_if<bool>(&value)) te_runtime_print_boolean(*boolean ? 1 : 0);
            break;
        }
        case OpCode::Pop: static_cast<void>(pop()); break;
        case OpCode::Jump: frame.ip = static_cast<std::size_t>(instruction.operand); break;
        case OpCode::JumpIfFalse:
            if (!std::holds_alternative<bool>(stack_.back())) {
                runtimeError("kondisi if/while harus boolean");
            } else if (!std::get<bool>(stack_.back())) {
                frame.ip = static_cast<std::size_t>(instruction.operand);
            }
            break;
        case OpCode::JumpIfTrue:
            if (!std::holds_alternative<bool>(stack_.back())) {
                runtimeError("kondisi &&/|| harus boolean");
            } else if (std::get<bool>(stack_.back())) {
                frame.ip = static_cast<std::size_t>(instruction.operand);
            }
            break;
        case OpCode::Call: {
            const auto functionIndex = static_cast<std::size_t>(instruction.operand);
            const auto argumentCount = static_cast<std::size_t>(instruction.operand2);
            if (functionIndex >= program_->functions.size() ||
                argumentCount != program_->functions[functionIndex].parameters.size()) {
                runtimeError("jumlah argumen function tidak sesuai");
                break;
            }
            std::vector<Value> arguments;
            for (std::size_t i = 0; i < argumentCount; ++i) {
                arguments.push_back(pop());
            }
            Frame next{functionIndex, 0, stack_.size(), {}, false};
            for (std::size_t i = 0; i < argumentCount; ++i) {
                next.locals[program_->functions[functionIndex].parameters[argumentCount - i - 1]]
                    = std::move(arguments[i]);
            }
            frames_.push_back(std::move(next));
            break;
        }
        case OpCode::CallBuiltin: {
            const auto builtin = static_cast<BuiltinFunction>(instruction.operand);
            const auto argumentCount = static_cast<std::size_t>(instruction.operand2);
            std::vector<Value> arguments;
            arguments.reserve(argumentCount);
            for (std::size_t i = 0; i < argumentCount; ++i) arguments.push_back(pop());
            std::reverse(arguments.begin(), arguments.end());
            push(callBuiltin(builtin, arguments));
            break;
        }
        case OpCode::Return: {
            const auto result = pop();
            const auto base = frame.stackBase;
            frames_.pop_back();
            stack_.resize(base);
            if (!frames_.empty()) push(result);
            break;
        }
        case OpCode::Halt:
            frames_.clear();
            break;
        }
    }
}

BytecodeVM::Value BytecodeVM::callBuiltin(
    BuiltinFunction builtin, const std::vector<Value>& arguments) {
    if (arguments.size() != 1) {
        runtimeError("builtin '" + std::string(builtinName(builtin)) + "' menerima 1 argumen");
        return {};
    }
    switch (builtin) {
    case BuiltinFunction::Len:
        if (const auto* text = std::get_if<std::string>(&arguments[0])) {
            return static_cast<double>(text->size());
        }
        runtimeError("builtin 'len' membutuhkan string");
        return {};
    case BuiltinFunction::Sqrt:
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
        if (const auto* number = std::get_if<double>(&arguments[0])) return std::abs(*number);
        runtimeError("builtin 'abs' membutuhkan angka");
        return {};
    case BuiltinFunction::TypeOf:
        if (std::holds_alternative<double>(arguments[0])) return std::string("number");
        if (std::holds_alternative<std::string>(arguments[0])) return std::string("string");
        if (std::holds_alternative<bool>(arguments[0])) return std::string("boolean");
        return std::string("void");
    }
    return {};
}

BytecodeVM::Value BytecodeVM::pop() {
    if (stack_.empty()) {
        runtimeError("stack underflow");
        return {};
    }
    auto value = std::move(stack_.back());
    stack_.pop_back();
    return value;
}

void BytecodeVM::push(Value value) {
    stack_.push_back(std::move(value));
}

BytecodeVM::Value BytecodeVM::loadName(const std::string& name) {
    if (!frames_.back().isMain) {
        const auto local = frames_.back().locals.find(name);
        if (local != frames_.back().locals.end()) return local->second;
    }
    const auto global = globals_.find(name);
    if (global != globals_.end()) return global->second;
    runtimeError("variabel '" + name + "' belum dideklarasikan");
    return {};
}

void BytecodeVM::defineName(const std::string& name, Value value) {
    if (frames_.back().isMain) globals_[name] = std::move(value);
    else frames_.back().locals[name] = std::move(value);
}

void BytecodeVM::storeName(const std::string& name, Value value) {
    if (!frames_.back().isMain) {
        const auto local = frames_.back().locals.find(name);
        if (local != frames_.back().locals.end()) {
            local->second = std::move(value);
            return;
        }
    }
    const auto global = globals_.find(name);
    if (global != globals_.end()) {
        global->second = std::move(value);
        return;
    }
    runtimeError("variabel '" + name + "' belum dideklarasikan");
}

BytecodeVM::Value BytecodeVM::constant(const Chunk& chunk, int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= chunk.constants.size()) {
        runtimeError("constant pool index tidak valid");
        return {};
    }
    return chunk.constants[static_cast<std::size_t>(index)];
}

bool BytecodeVM::condition(Value value, const std::string& construct) {
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    runtimeError("kondisi '" + construct + "' harus boolean");
    return false;
}

void BytecodeVM::runtimeError(const std::string& message) {
    diagnostics_.push_back({{}, message});
}

} // namespace te