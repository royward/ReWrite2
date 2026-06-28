#pragma once

#include <string>
#include <vector>
#include <string>
#include <cstdint>
#include <format>

enum TokenKind {
    True,
    False,
    Identifier,
    UnsignedInteger,
    LParen,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Comma,
    Arrow,
    Semicolon,
    Guard,
    Splat,
    GreedySplat,
    Dot,
    Colon,
    Plus,
    Minus,
    Times,
    Divide,
    Modulus,
    EqualsEquals,
    Eof,
};

struct Token {
    TokenKind kind;
    std::string_view text;
    uint32_t row;
    uint32_t start_column;
    uint32_t end_column;
};

template <>
struct std::formatter<Token> : std::formatter<std::string> {
    auto format(const Token& t, auto& ctx) const {
        return std::format_to(ctx.out(), "{{{}:{}:{},{}-{}}}",
                               t.text, static_cast<int>(t.kind),
                               t.row+1, t.start_column+1, t.end_column+1);
    }
};

std::vector<Token> lex(std::string_view program);
