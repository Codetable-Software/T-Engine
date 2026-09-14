#pragma once

#include "tengine/token.hpp"

#include <string>
#include <vector>

namespace te {

struct Diagnostic {
    SourceLocation location;
    std::string message;
};

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> scan();
    const std::vector<Diagnostic>& diagnostics() const noexcept;

private:
    char peek() const;
    char advance();
    bool match(char expected);
    void addToken(TokenKind kind, TokenValue value = {});
    void scanToken();
    void scanNumber();
    void scanIdentifier();
    void scanString();
    void error(const std::string& message);

    std::string source_;
    std::vector<Token> tokens_;
    std::vector<Diagnostic> diagnostics_;
    std::size_t start_{0};
    std::size_t current_{0};
    SourceLocation location_{};
};

} // namespace te
