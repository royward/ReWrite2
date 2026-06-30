// ============================================================================
// File: token.hpp
// Description: The Token type output by the lexer and used for parsing
// ============================================================================
// Copyright 2026 Roy Ward
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
