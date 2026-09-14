#include "tengine/lexer.hpp"

#include <cctype>
#include <cstdlib>
#include <utility>
#include <unordered_map>

namespace te {

namespace {
const std::unordered_map<std::string, TokenKind> keywords{
    {"let", TokenKind::Let},
    {"print", TokenKind::Print},
    {"fn", TokenKind::Fn},
    {"return", TokenKind::Return},
    {"if", TokenKind::If},
    {"else", TokenKind::Else},
    {"while", TokenKind::While},
    {"true", TokenKind::True},
    {"false", TokenKind::False},
};
}

std::string tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::EndOfFile: return "end of file";
    case TokenKind::Identifier: return "identifier";
    case TokenKind::Number: return "number";
    case TokenKind::String: return "string";
    case TokenKind::Let: return "let";
    case TokenKind::Print: return "print";
    case TokenKind::Fn: return "fn";
    case TokenKind::Return: return "return";
    case TokenKind::If: return "if";
    case TokenKind::Else: return "else";
    case TokenKind::While: return "while";
    case TokenKind::True: return "true";
    case TokenKind::False: return "false";
    case TokenKind::Plus: return "+";
    case TokenKind::Minus: return "-";
    case TokenKind::Star: return "*";
    case TokenKind::Slash: return "/";
    case TokenKind::Bang: return "!";
    case TokenKind::EqualEqual: return "==";
    case TokenKind::BangEqual: return "!=";
    case TokenKind::Greater: return ">";
    case TokenKind::GreaterEqual: return ">=";
    case TokenKind::Less: return "<";
    case TokenKind::LessEqual: return "<=";
    case TokenKind::AndAnd: return "&&";
    case TokenKind::OrOr: return "||";
    case TokenKind::Equal: return "=";
    case TokenKind::LeftParen: return "(";
    case TokenKind::RightParen: return ")";
    case TokenKind::LeftBrace: return "{";
    case TokenKind::RightBrace: return "}";
    case TokenKind::Comma: return ",";
    case TokenKind::Semicolon: return ";";
    }
    return "unknown";
}

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::scan() {
    while (current_ < source_.size()) {
        start_ = current_;
        scanToken();
    }
    tokens_.push_back({TokenKind::EndOfFile, {}, location_});
    return tokens_;
}

const std::vector<Diagnostic>& Lexer::diagnostics() const noexcept {
    return diagnostics_;
}

char Lexer::peek() const {
    return current_ < source_.size() ? source_[current_] : '\0';
}

char Lexer::advance() {
    const char character = peek();
    if (character == '\n') {
        ++location_.line;
        location_.column = 1;
    } else {
        ++location_.column;
    }
    ++location_.offset;
    ++current_;
    return character;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

void Lexer::addToken(TokenKind kind, TokenValue value) {
    tokens_.push_back({kind, std::move(value), {location_.line, location_.column, start_}});
}

void Lexer::scanToken() {
    const char character = advance();
    switch (character) {
    case '(': addToken(TokenKind::LeftParen); break;
    case ')': addToken(TokenKind::RightParen); break;
    case '+': addToken(TokenKind::Plus); break;
    case '-': addToken(TokenKind::Minus); break;
    case '*': addToken(TokenKind::Star); break;
    case '/': addToken(TokenKind::Slash); break;
    case '=': addToken(match('=') ? TokenKind::EqualEqual : TokenKind::Equal); break;
    case '!': addToken(match('=') ? TokenKind::BangEqual : TokenKind::Bang); break;
    case '>': addToken(match('=') ? TokenKind::GreaterEqual : TokenKind::Greater); break;
    case '<': addToken(match('=') ? TokenKind::LessEqual : TokenKind::Less); break;
    case '&': if (match('&')) addToken(TokenKind::AndAnd); else error("harapkan '&' kedua"); break;
    case '|': if (match('|')) addToken(TokenKind::OrOr); else error("harapkan '|' kedua"); break;
    case '{': addToken(TokenKind::LeftBrace); break;
    case '}': addToken(TokenKind::RightBrace); break;
    case ',': addToken(TokenKind::Comma); break;
    case ';': addToken(TokenKind::Semicolon); break;
    case ' ': case '\r': case '\t': case '\n': break;
    case '#':
        while (peek() != '\n' && peek() != '\0') advance();
        break;
    case '"': scanString(); break;
    default:
        if (std::isdigit(static_cast<unsigned char>(character))) scanNumber();
        else if (std::isalpha(static_cast<unsigned char>(character)) || character == '_') scanIdentifier();
        else error("karakter tidak dikenal");
        break;
    }
}

void Lexer::scanNumber() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    const bool hasFraction = peek() == '.' &&
        current_ + 1 < source_.size() &&
        std::isdigit(static_cast<unsigned char>(source_[current_ + 1]));
    if (hasFraction) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    addToken(TokenKind::Number, std::strtod(source_.substr(start_, current_ - start_).c_str(), nullptr));
}

void Lexer::scanIdentifier() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
    const auto text = source_.substr(start_, current_ - start_);
    const auto it = keywords.find(text);
    if (it == keywords.end()) addToken(TokenKind::Identifier, text);
    else if (it->second == TokenKind::True) addToken(TokenKind::True, true);
    else if (it->second == TokenKind::False) addToken(TokenKind::False, false);
    else addToken(it->second);
}

void Lexer::scanString() {
    std::string value;
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\n') {
            error("string tidak boleh melewati baris");
            return;
        }
        value += advance();
    }
    if (peek() == '\0') {
        error("string belum ditutup");
        return;
    }
    advance();
    addToken(TokenKind::String, std::move(value));
}

void Lexer::error(const std::string& message) {
    diagnostics_.push_back({location_, message});
}

} // namespace te
