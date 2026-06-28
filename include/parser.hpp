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
