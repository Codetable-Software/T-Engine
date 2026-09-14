#pragma once

#include <cstddef>
#include <string>
#include <variant>

namespace te {

struct SourceLocation {
    std::size_t line{1};
    std::size_t column{1};
    std::size_t offset{0};
};

enum class TokenKind {
    EndOfFile,
    Identifier,
    Number,
    String,

    Let,
    Print,
    Fn,
    Return,
    If,
    Else,
    While,
    True,
    False,

    Plus,
    Minus,
    Star,
    Slash,
    Bang,
    EqualEqual,
    BangEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    AndAnd,
    OrOr,
    Equal,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
};

using TokenValue = std::variant<std::monostate, double, std::string, bool>;

struct Token {
    TokenKind kind;
    TokenValue value;
    SourceLocation location;
};

std::string tokenKindName(TokenKind kind);

} // namespace te
