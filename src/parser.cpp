#include "tengine/parser.hpp"

#include <utility>

namespace te {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

Program Parser::parse() {
    Program program;
    while (!isAtEnd()) {
        try {
            program.push_back(statement());
        } catch (...) {
            synchronize();
        }
    }
    return program;
}

const std::vector<Diagnostic>& Parser::diagnostics() const noexcept {
    return diagnostics_;
}

const Token& Parser::peek() const { return tokens_[current_]; }
const Token& Parser::peekNext() const {
    return current_ + 1 < tokens_.size() ? tokens_[current_ + 1] : tokens_.back();
}
const Token& Parser::previous() const { return tokens_[current_ - 1]; }
bool Parser::isAtEnd() const { return check(TokenKind::EndOfFile); }
const Token& Parser::advance() {
    if (!isAtEnd()) ++current_;
    return previous();
}
bool Parser::check(TokenKind kind) const { return peek().kind == kind; }
bool Parser::match(TokenKind kind) {
    if (!check(kind)) return false;
    advance();
    return true;
}

const Token& Parser::consume(TokenKind kind, const std::string& message) {
    if (check(kind)) return advance();
    error(peek(), message);
    throw 1;
}

StatementPtr Parser::statement() {
    if (match(TokenKind::Let)) return letStatement();
    if (match(TokenKind::Print)) return printStatement();
    if (match(TokenKind::Fn)) return functionStatement();
    if (match(TokenKind::Return)) return returnStatement();
    if (match(TokenKind::If)) return ifStatement();
    if (match(TokenKind::While)) return whileStatement();
    if (check(TokenKind::Identifier) && peekNext().kind == TokenKind::Equal) {
        return assignStatement();
    }
    auto expressionNode = expression();
    consume(TokenKind::Semicolon, "harapkan ';' setelah ekspresi");
    return std::make_unique<Statement>(ExpressionStatement{std::move(expressionNode)});
}

StatementPtr Parser::functionStatement() {
    const auto& name = consume(TokenKind::Identifier, "harapkan nama function setelah 'fn'");
    consume(TokenKind::LeftParen, "harapkan '(' setelah nama function");

    std::vector<std::string> parameters;
    if (!check(TokenKind::RightParen)) {
        do {
            const auto& parameter = consume(TokenKind::Identifier, "harapkan nama parameter");
            parameters.push_back(std::get<std::string>(parameter.value));
        } while (match(TokenKind::Comma));
    }
    consume(TokenKind::RightParen, "harapkan ')' setelah parameter");
    consume(TokenKind::LeftBrace, "harapkan '{' sebelum body function");
    auto body = block();
    return std::make_unique<Statement>(FunctionStatement{
        std::get<std::string>(name.value), std::move(parameters), std::move(body)});
}

StatementPtr Parser::returnStatement() {
    auto value = expression();
    consume(TokenKind::Semicolon, "harapkan ';' setelah return");
    return std::make_unique<Statement>(ReturnStatement{std::move(value)});
}

StatementPtr Parser::ifStatement() {
    consume(TokenKind::LeftParen, "harapkan '(' setelah 'if'");
    auto condition = expression();
    consume(TokenKind::RightParen, "harapkan ')' setelah kondisi if");
    consume(TokenKind::LeftBrace, "harapkan '{' setelah kondisi if");
    auto thenBranch = block();

    Program elseBranch;
    if (match(TokenKind::Else)) {
        consume(TokenKind::LeftBrace, "harapkan '{' setelah 'else'");
        elseBranch = block();
    }
    return std::make_unique<Statement>(IfStatement{
        std::move(condition), std::move(thenBranch), std::move(elseBranch)});
}

StatementPtr Parser::whileStatement() {
    consume(TokenKind::LeftParen, "harapkan '(' setelah 'while'");
    auto condition = expression();
    consume(TokenKind::RightParen, "harapkan ')' setelah kondisi while");
    consume(TokenKind::LeftBrace, "harapkan '{' setelah kondisi while");
    auto body = block();
    return std::make_unique<Statement>(WhileStatement{
        std::move(condition), std::move(body)});
}

Program Parser::block() {
    Program statements;
    while (!check(TokenKind::RightBrace) && !isAtEnd()) {
        try {
            statements.push_back(statement());
        } catch (...) {
            synchronize();
        }
    }
    consume(TokenKind::RightBrace, "harapkan '}' setelah body function");
    return statements;
}

StatementPtr Parser::assignStatement() {
    const auto& name = consume(TokenKind::Identifier, "harapkan nama variabel");
    consume(TokenKind::Equal, "harapkan '=' setelah nama variabel");
    auto value = expression();
    consume(TokenKind::Semicolon, "harapkan ';' setelah assignment");
    return std::make_unique<Statement>(AssignStatement{
        std::get<std::string>(name.value), std::move(value)});
}

StatementPtr Parser::letStatement() {
    const auto& name = consume(TokenKind::Identifier, "harapkan nama variabel setelah 'let'");
    consume(TokenKind::Equal, "harapkan '=' setelah nama variabel");
    auto initializer = expression();
    consume(TokenKind::Semicolon, "harapkan ';' setelah deklarasi");
    return std::make_unique<Statement>(LetStatement{
        std::get<std::string>(name.value), std::move(initializer)});
}

StatementPtr Parser::printStatement() {
    auto expressionNode = expression();
    consume(TokenKind::Semicolon, "harapkan ';' setelah print");
    return std::make_unique<Statement>(PrintStatement{std::move(expressionNode)});
}

ExprPtr Parser::expression() { return logicalOr(); }

ExprPtr Parser::logicalOr() {
    auto left = logicalAnd();
    while (match(TokenKind::OrOr)) {
        left = std::make_unique<Expr>(BinaryExpr{
            std::move(left), previous().kind, logicalAnd()});
    }
    return left;
}

ExprPtr Parser::logicalAnd() {
    auto left = equality();
    while (match(TokenKind::AndAnd)) {
        left = std::make_unique<Expr>(BinaryExpr{
            std::move(left), previous().kind, equality()});
    }
    return left;
}

ExprPtr Parser::equality() {
    auto left = comparison();
    while (check(TokenKind::EqualEqual) || check(TokenKind::BangEqual)) {
        const auto op = advance().kind;
        left = std::make_unique<Expr>(BinaryExpr{
            std::move(left), op, comparison()});
    }
    return left;
}

ExprPtr Parser::comparison() {
    auto left = addition();
    while (check(TokenKind::Greater) || check(TokenKind::GreaterEqual) ||
           check(TokenKind::Less) || check(TokenKind::LessEqual)) {
        const auto op = advance().kind;
        left = std::make_unique<Expr>(BinaryExpr{
            std::move(left), op, addition()});
    }
    return left;
}

ExprPtr Parser::addition() {
    auto left = multiplication();
    while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
        const auto op = advance().kind;
        left = makeBinary(std::move(left), op);
    }
    return left;
}

ExprPtr Parser::multiplication() {
    auto left = unary();
    while (check(TokenKind::Star) || check(TokenKind::Slash)) {
        const auto op = advance().kind;
        left = std::make_unique<Expr>(BinaryExpr{
            std::move(left), op, unary()});
    }
    return left;
}

ExprPtr Parser::unary() {
    if (match(TokenKind::Bang) || match(TokenKind::Minus)) {
        return std::make_unique<Expr>(UnaryExpr{previous().kind, unary()});
    }
    return primary();
}

ExprPtr Parser::primary() {
    if (match(TokenKind::Number) || match(TokenKind::String) ||
        match(TokenKind::True) || match(TokenKind::False)) {
        return std::make_unique<Expr>(LiteralExpr{previous().value});
    }
    if (match(TokenKind::Identifier)) {
        const auto name = std::get<std::string>(previous().value);
        if (!match(TokenKind::LeftParen)) {
            return std::make_unique<Expr>(VariableExpr{name});
        }
        std::vector<ExprPtr> arguments;
        if (!check(TokenKind::RightParen)) {
            do {
                arguments.push_back(expression());
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightParen, "harapkan ')' setelah argumen function");
        return std::make_unique<Expr>(CallExpr{std::move(name), std::move(arguments)});
    }
    if (match(TokenKind::LeftParen)) {
        auto node = expression();
        consume(TokenKind::RightParen, "harapkan ')' setelah ekspresi");
        return node;
    }
    error(peek(), "harapkan ekspresi");
    throw 1;
}

ExprPtr Parser::makeBinary(ExprPtr left, TokenKind op) {
    auto right = multiplication();
    return std::make_unique<Expr>(BinaryExpr{std::move(left), op, std::move(right)});
}

void Parser::synchronize() {
    if (!isAtEnd()) advance();
    while (!isAtEnd()) {
        if (previous().kind == TokenKind::Semicolon) return;
        if (check(TokenKind::Let) || check(TokenKind::Print)) return;
        advance();
    }
}

void Parser::error(const Token& token, const std::string& message) {
    diagnostics_.push_back({token.location, message});
}

} // namespace te
