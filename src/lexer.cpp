#include <stdexcept>
#include <format>
#include "lexer.hpp"

std::vector<Token> lex(std::string_view program) {
    std::vector<Token> result;
    std::size_t p=0;
    std::size_t len=program.length();
    std::size_t offset_start_of_row=0;
    uint32_t row=0;
    while(p<len) {
        TokenKind token_kind;
        std::size_t start_p=p;
        char c=program[p++];
        if(std::isalpha(static_cast<unsigned char>(c))) {
            while(p<len && (program[p]=='_' || std::isalpha(static_cast<unsigned char>(program[p])))) {
                p++;
            }
            token_kind=Identifier;
        } else if(std::isdigit(static_cast<unsigned char>(c))) {
           while(p<len && std::isdigit(static_cast<unsigned char>(program[p]))) {
                p++;
            }
            token_kind=UnsignedInteger;
        } else if(c=='\n') {
            row++;
            offset_start_of_row=p;
            continue; // NOTE: no token produced for newline
        } else if(std::isblank(static_cast<unsigned char>(c))) {
            continue; // NOTE: no token produced for whitespaces
        } else {
            switch(c) {
                case '(':token_kind=LParen; break;
                case ')':token_kind=RParen; break;
                case ';':token_kind=Semicolon; break;
                case '*':token_kind=Times; break;
                case '+':token_kind=Plus; break;
                case '/':token_kind=Divide; break;
                case '%':token_kind=Modulus; break;
                case '-': {
                    if(p<len && program[p]=='>') {
                        p++;
                        token_kind=Arrow;
                    } else {
                        token_kind=Minus;
                    }
                    break;
                }
                default:throw std::runtime_error(std::format("Unexpected character: {}", c));
            }
        }
        result.push_back(Token{
            .kind = token_kind,
            .text = program.substr(start_p, p - start_p),
            .row = row,
            .start_column = static_cast<uint32_t>(start_p - offset_start_of_row),
            .end_column = static_cast<uint32_t>(p - offset_start_of_row),
        });
    }
    // put a sentinel Eof at the end
    result.push_back(Token{
        .kind = Eof,
        .text = "",
        .row = row,
        .start_column = static_cast<uint32_t>(p - offset_start_of_row),
        .end_column = static_cast<uint32_t>(p - offset_start_of_row),
    });
    return result;
}
