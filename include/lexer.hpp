#pragma once

#include "token_kind.hpp"
#include <string>
#include <vector>
#include <string>
#include <cstdint>
#include <format>

struct Token {
    TokenKind kind;
    std::string_view text;
    uint32_t row;
    uint32_t start_column;
    uint32_t end_column;
    std::string to_string() const;
};

std::vector<Token> lex(std::string_view program);
