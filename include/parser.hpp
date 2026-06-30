// ============================================================================
// File: parser.hpp
// Description: a very simple parser container for processing a list of tokens
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

#include <vector>

struct Parser {
    std::vector<Token> tokens;
    std::size_t pos = 0;

    const Token& current() const { return tokens[pos]; }
    const Token& peek() const {
        // Eof is always the last token; once current() is Eof, peeking
        // again just returns Eof again rather than reading out of bounds.
        return pos + 1 < tokens.size() ? tokens[pos + 1] : tokens.back();
    }
    void advance() { pos++; }
    bool eof() { return tokens[pos].kind==Eof; }
};
