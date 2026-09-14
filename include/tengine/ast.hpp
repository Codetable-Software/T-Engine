#pragma once

#include "tengine/token.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace te {

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct LiteralExpr {
    TokenValue value;
};

struct VariableExpr {
    std::string name;
};

struct BinaryExpr {
    ExprPtr left;
    TokenKind op;
    ExprPtr right;
};

struct UnaryExpr {
    TokenKind op;
    ExprPtr right;
};

struct CallExpr {
    std::string name;
    std::vector<ExprPtr> arguments;
};

struct Expr {
    std::variant<LiteralExpr, VariableExpr, BinaryExpr, UnaryExpr, CallExpr> value;
};

struct Statement;
using StatementPtr = std::unique_ptr<Statement>;
using Program = std::vector<StatementPtr>;

struct LetStatement {
    std::string name;
    ExprPtr initializer;
};

struct PrintStatement {
    ExprPtr expression;
};

struct ExpressionStatement {
    ExprPtr expression;
};

struct AssignStatement {
    std::string name;
    ExprPtr value;
};

struct ReturnStatement {
    ExprPtr value;
};

struct FunctionStatement {
    std::string name;
    std::vector<std::string> parameters;
    Program body;
};

struct IfStatement {
    ExprPtr condition;
    Program thenBranch;
    Program elseBranch;
};

struct WhileStatement {
    ExprPtr condition;
    Program body;
};

struct Statement {
    std::variant<LetStatement, PrintStatement, ExpressionStatement, AssignStatement,
                 ReturnStatement, FunctionStatement, IfStatement, WhileStatement> value;
};

} // namespace te
