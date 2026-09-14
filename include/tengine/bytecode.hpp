#pragma once

#include "tengine/ast.hpp"
#include "tengine/builtins.hpp"
#include "tengine/lexer.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace te {

enum class OpCode {
    Constant,
    LoadName,
    DefineName,
    StoreName,
    Add,
    Subtract,
    Multiply,
    Divide,
    Negate,
    Not,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    And,
    Or,
    Print,
    Pop,
    Jump,
    JumpIfFalse,
    JumpIfTrue,
    Call,
    CallBuiltin,
    Return,
    Halt,
};

struct Instruction {
    OpCode op;
    int operand{0};
    int operand2{0};
};

struct Chunk {
    std::vector<Instruction> code;
    std::vector<TokenValue> constants;
};

struct CompiledFunction {
    std::string name;
    std::vector<std::string> parameters;
    Chunk chunk;
};

struct BytecodeProgram {
    Chunk main;
    std::vector<CompiledFunction> functions;
};

class BytecodeCompiler {
public:
    BytecodeProgram compile(const Program& program);
    const std::vector<Diagnostic>& diagnostics() const noexcept;

private:
    struct FunctionContext {
        Chunk chunk;
        const CompiledFunction* metadata{nullptr};
        bool isMain{false};
    };

    void compileStatement(const Statement& statement);
    void compileBlock(const Program& program);
    void compileExpression(const Expr& expression);
    void compileFunction(const FunctionStatement& function);
    int addConstant(TokenValue value);
    int addName(const std::string& name);
    std::size_t emit(OpCode op, int operand = 0, int operand2 = 0);
    void patchJump(std::size_t instruction, std::size_t target);
    void error(const std::string& message);

    BytecodeProgram program_;
    FunctionContext* current_{nullptr};
    std::unordered_map<std::string, std::size_t> functionIndices_;
    std::unordered_map<std::string, int> names_;
    std::vector<std::string> nameTable_;
    std::vector<Diagnostic> diagnostics_;
};

class BytecodeVM {
public:
    void execute(const BytecodeProgram& program);
    const std::vector<Diagnostic>& diagnostics() const noexcept;

private:
    using Value = TokenValue;

    struct Frame {
        std::size_t functionIndex{0};
        std::size_t ip{0};
        std::size_t stackBase{0};
        std::unordered_map<std::string, Value> locals;
        bool isMain{false};
    };

    void runFrame();
    Value pop();
    void push(Value value);
    Value loadName(const std::string& name);
    void defineName(const std::string& name, Value value);
    void storeName(const std::string& name, Value value);
    Value constant(const Chunk& chunk, int index);
    Value callBuiltin(BuiltinFunction builtin, const std::vector<Value>& arguments);
    bool condition(Value value, const std::string& construct);
    void runtimeError(const std::string& message);

    const BytecodeProgram* program_{nullptr};
    std::vector<Value> stack_;
    std::vector<Frame> frames_;
    std::unordered_map<std::string, Value> globals_;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace te