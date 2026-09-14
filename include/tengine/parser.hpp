#pragma once

#include "tengine/ast.hpp"
#include "tengine/lexer.hpp"

#include <vector>

namespace te {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    Program parse();
    const std::vector<Diagnostic>& diagnostics() const noexcept;

private:
    const Token& peek() const;
    const Token& peekNext() const;
    const Token& previous() const;
    bool isAtEnd() const;
    const Token& advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& consume(TokenKind kind, const std::string& message);

    StatementPtr statement();
    StatementPtr letStatement();
    StatementPtr printStatement();
    StatementPtr assignStatement();
    StatementPtr functionStatement();
    StatementPtr returnStatement();
    StatementPtr ifStatement();
    StatementPtr whileStatement();
    Program block();
    ExprPtr expression();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr addition();
    ExprPtr multiplication();
    ExprPtr unary();
    ExprPtr primary();
    ExprPtr makeBinary(ExprPtr left, TokenKind op);
    void synchronize();
    void error(const Token& token, const std::string& message);

    const std::vector<Token>& tokens_;
    std::vector<Diagnostic> diagnostics_;
    std::size_t current_{0};
};

} // namespace te
