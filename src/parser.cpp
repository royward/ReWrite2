// ============================================================================
// File: parser.cpp
// Description: Tools for parsing a list of tokens into program rules
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

#include "token.hpp"
#include "program.hpp"
#include "parser.hpp"
#include "util.hpp"
#include <print>
#include <ranges>
#include <charconv>

const std::unordered_map<std::string, TokenKind> library_map = {
    {"count_trailing_zeros", CountTrailingZeros},
    {"pop_count", PopCount},
    {"char_to_int", CharToInt},
    {"int_to_char", IntToChar},
    {"print", Print},
    {"println", PrintLn},
    {"println_debug", PrintLnDebug},
    {"load_text_file", LoadTextFile},
    {"save_text_file", SaveTextFile},
    {"save_binary_file", SaveBinaryFile},
};

void parse_error(const Token& token, std::vector<std::string_view> expected) {
    std::string message;
    if (expected.size() == 1) {
        message = std::format("found {}, expected {}", token.text, expected[0]);
    } else {
        message = std::format("found {}, expected one of {}", token.text, expected[0]);
        for (const auto& elem : expected | std::views::drop(1)) {
            message += std::format(", {}", elem);
        }
    }
    throw std::runtime_error(message);
}

std::string token_kind_to_string(TokenKind& t) {
    switch(t) {
        case LParen : return "(";
        case RParen : return ")";
        case LBracket : return "[";
        case RBracket : return "]";
        case LBrace : return "{";
        case RBrace : return "}";
        case Semicolon : return ";";
        case Comma : return ",";
        default: return "unknown";
    }
}

int64_t token_to_int(Token& token) {
    int64_t value;
    auto result = std::from_chars(token.text.data(), token.text.data() + token.text.size(), value);
    if (result.ec != std::errc{}) {
        parse_error(token,{"integer"});
    } else if (result.ptr != token.text.data() + token.text.size()) {
        // parsed successfully but didn't consume the whole string
        parse_error(token,{"integer"});
    }
    return value;
}

//std::vector<Parameter> parse_param_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, TokenKind end, TokenKind sep);

std::vector<Parameter> string_to_paramlist(std::string_view s) {
    std::vector<Parameter> chars;
    chars.reserve(s.size());
    for(char c : s) {
        chars.push_back(Parameter{Const{DataElement{DataChar{c}}}});
    }
    return chars;
}

std::vector<Expression> string_to_exprlist(std::string_view s) {
    std::vector<Expression> chars;
    chars.reserve(s.size());
    for(char c : s) {
        chars.push_back(Expression{Const{DataElement{DataChar{c}}}});
    }
    return chars;
}

void Program::parse_const(Parser& parser) {
    parser.advance();
    Token identifier=parser.current();
    if(identifier.kind!=Identifier) {
        parse_error(identifier,{"identifier"});
    }
    std::string name=identifier.text;
    parser.advance();
    Token eq=parser.current();
    if(eq.kind!=Equal) {
        parse_error(eq,{"="});
    }
    parser.advance();
    std::unordered_map<std::string, std::size_t> empty_param_id_map;
    std::vector<Expression> expr=parse_expression_list(parser, empty_param_id_map, Semicolon, Comma);
    if(constants.find(name)!=constants.end()) {
        throw std::runtime_error(std::format("const already declared {}",name));
    }
    const std::vector<DataElement> empty_bindings;
    std::vector<DataElement> values;
    for(const Expression& e : expr) {
        do_call_single(e, empty_bindings, values);
    }
    constants[name] = std::move(values);
}

Parameter Program::parse_param(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map) {
    Token t=parser.current();
    parser.advance();
    switch(t.kind) {
        case UnsignedInteger : return Parameter{Const{DataElement{DataInt{token_to_int(t)}}}};
        case True : return Parameter{Const{DataElement{DataBool{true}}}};
        case False : return Parameter{Const{DataElement{DataBool{false}}}};
        case Minus : {
            Token t2=parser.current();
            parser.advance();
            if(t2.kind==UnsignedInteger) {
                return Parameter{Const{DataElement{DataInt{-token_to_int(t)}}}};
            } else {
                parse_error(t2,{"integer"});
            } break;
        }
        case Identifier : {
            auto find_constant=constants.find(t.text);
            if(find_constant != constants.end()) {
                std::vector<DataElement>& const_val=find_constant->second;
                if(const_val.size()!=1) {
                    throw std::runtime_error(std::format("const expression is expected to be of size 1, found {}",const_val.size()));
                }
                return Parameter{Const{const_val[0]}};
            } else {
                std::string s=static_cast<std::string>(t.text);
                param_id_map.try_emplace(s,param_id_map.size());
                uint32_t id=static_cast<uint32_t>(param_id_map[s]);
                return Parameter{Id{id}};
            }
        }
        case Wildcard : {
            return Parameter{ParamWildcard{}};
        }
        case Splat: {
            Token t2=parser.current();
            parser.advance();
            if(t2.kind==Identifier) {
                std::string s=static_cast<std::string>(t2.text);
                param_id_map.try_emplace(s,param_id_map.size());
                uint32_t id=static_cast<uint32_t>(param_id_map[s]);
                return Parameter{ParamSplat{id}};
            } else if(t2.kind==Wildcard) {
                return Parameter{ParamSplatWild{}};
            } else {
                parse_error(t2,{"identifier","_"});
            } break;
        }
        case LBrace: {
            std::vector<Parameter> list_internal=parse_param_list(parser, param_id_map, RBrace, Comma);
            return Parameter{ParamList{std::move(list_internal)}};
        }
        case String: {
            return Parameter{ParamList{string_to_paramlist(t.text)}};
        }
        case Chars: {
            std::vector<Parameter> chars=string_to_paramlist(t.text);
            if(chars.size()!=1) {
                throw std::runtime_error(std::format("char expression is expected to be of size 1, found {}",chars.size()));
            }
            return std::move(chars[0]);
        }
        default : parse_error(t,{"parameter"});
    }
    // never gets to here, but g++ doesn't know that
    return Parameter{Const{DataElement{DataUnbound{}}}};
}

std::vector<Parameter> Program::parse_param_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, TokenKind end, TokenKind sep) {
    std::size_t splat_count=0;
    std::vector<Parameter> param_list;
    if(parser.current().kind==end) {
        parser.advance();
        return param_list;
    } else {
        TokenKind separator_or_end=sep;
        while(separator_or_end!=end) {
            Token t=parser.current();
            if(t.kind==end) {
                parser.advance();
                return param_list; // deal with trailing comma case
            } else if(t.kind==Chars) {
                std::vector<Parameter> chars=string_to_paramlist(t.text);
                param_list.insert(param_list.end(),std::make_move_iterator(chars.begin()),std::make_move_iterator(chars.end()));
                parser.advance();
            } else if(t.kind==Identifier && constants.find(t.text)!=constants.end()) {
                std::vector<DataElement>& const_val=constants[t.text];
                for(auto& v : const_val) {
                    param_list.push_back(Parameter{Const{v}});
                }
                parser.advance();
            } else {
                Parameter p2=parse_param(parser,param_id_map);
                if(std::holds_alternative<ParamSplat>(p2.value) || std::holds_alternative<ParamSplatWild>(p2.value)) {
                    splat_count++;
                    if(splat_count>1) {
                        throw std::runtime_error("only one splat per list");
                    }
                }
                param_list.push_back(std::move(p2));
            }
            separator_or_end=parser.current().kind;
            if(separator_or_end!=sep && separator_or_end!=end) {
                parse_error(parser.current(),{token_kind_to_string(sep),token_kind_to_string(end)});
            }
            parser.advance();
    }
    }
    return param_list;
}

std::tuple<uint8_t, TokenKind> prefix_binding_power(TokenKind op) {
    switch(op) {
        case Minus: return {110, Minus};      // unary minus, binds tighter than any infix op
        case Not:   return {110, Not};        // unary logical/bitwise not
        default:    return {0, Eof};
    }
}

std::tuple<uint8_t, uint8_t, TokenKind> infix_binding_power(TokenKind op) {
    switch(op) {
        // logical
        case OrOr:           return {10, 11, OrOr};
        case AndAnd:          return {30, 31, AndAnd};

        // comparison - now binds looser than bitwise, unlike C
        case EqualEqual:   return {50, 51, EqualEqual};
        case NotEqual:     return {50, 51, NotEqual};
        case Less:         return {50, 51, Less};
        case Greater:      return {50, 51, Greater};
        case LessEqual:    return {50, 51, LessEqual};
        case GreaterEqual: return {50, 51, GreaterEqual};

        // bitwise - binds tighter than comparison
        case Or:           return {70, 71, Or};
        case Xor:          return {90, 91, Xor};
        case And:          return {110, 111, And};

        // shift
        case ShiftLeft:    return {130, 131, ShiftLeft};
        case ShiftRight:   return {130, 131, ShiftRight};

        // arithmetic
        case Plus:         return {150, 151, Plus};
        case Minus:        return {150, 151, Minus};
        case Times:        return {170, 171, Times};
        case Divide:       return {170, 171, Divide};
        case Modulus:      return {170, 171, Modulus};

        default: return {0, 0, Eof};
    }
}

//std::vector<Expression> parse_expression_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, Program& program, TokenKind end, TokenKind sep);

Expression Program::parse_expression(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, uint8_t pri) {
    Token t=parser.current();
    parser.advance();
    Expression expr;
    auto [prefix_pri, prefix_id] = prefix_binding_power(t.kind);
    if(t.kind==Minus && parser.current().kind==UnsignedInteger) {
        Token t2=parser.current();
        parser.advance();
        expr=Expression{Const{DataElement{DataInt{-token_to_int(t2)}}}};
    } else if(prefix_id!=Eof) {
        Expression expr2=parse_expression(parser,param_id_map,prefix_pri);
        std::vector<Expression> expr_list;
        expr_list.push_back(std::move(expr2));
        expr=Expression{CallInternal{prefix_id,std::move(expr_list)}};
    } else {
        switch(t.kind) {
            case UnsignedInteger : expr=Expression{Const{DataElement{DataInt{token_to_int(t)}}}}; break;
            case True : return Expression{Const{DataElement{DataBool{true}}}};
            case False : return Expression{Const{DataElement{DataBool{false}}}};
            case Identifier : {
                auto find_constant=constants.find(t.text);
                if(find_constant != constants.end()) {
                    std::vector<DataElement>& const_val=find_constant->second;
                    if(const_val.size()!=1) {
                        throw std::runtime_error(std::format("const expression is expected to be of size 1, found {}",const_val.size()));
                    }
                    expr=Expression{Const{const_val[0]}};
                } else if(parser.current().kind==LParen) {
                    parser.advance();
                    auto built_in=library_map.find(t.text);
                    std::vector<Expression> expr_list=parse_expression_list(parser, param_id_map, RParen, Comma);
                    if(built_in!=library_map.end()) {
                        expr=Expression{CallLibrary{built_in->second,std::move(expr_list)}};
                    } else {
                        std::string s=static_cast<std::string>(t.text);
                        function_map.try_emplace(s,function_map.size()); // allow insert, because might be forward call
                        uint32_t id=static_cast<uint32_t>(function_map[s]);
                        expr=Expression{Call{id,std::move(expr_list)}};
                    }
                } else {
                    std::string s=static_cast<std::string>(t.text);
                    auto search=param_id_map.find(s);
                    if(search==param_id_map.end()) {
                        throw std::runtime_error(std::format("parameter not bound on right hand size: {}",s));
                    }
                    uint32_t id=static_cast<uint32_t>(search->second);
                    expr=Expression{Id{id}};
                }
            } break;
            case Splat: {
                Expression splat_expr=parse_expression(parser,param_id_map,200);
                expr=Expression{ExprSplat{std::make_unique<Expression>(std::move(splat_expr))}};
                // Token t2=parser.current();
                // parser.advance();
                // if(t2.kind==Identifier) {
                //     std::string s=static_cast<std::string>(t2.text);
                //     auto search=param_id_map.find(s);
                //     if(search==param_id_map.end()) {
                //         throw std::runtime_error(std::format("parameter not bound on left hand size: {}",s));
                //     }
                //     uint32_t id=static_cast<uint32_t>(search->second);
                //     expr=Expression{ExprSplat{std::make_unique<Expression>(Expression{Id{id}})}};
                // } else {
                //     parse_error(t2,{"identifier"});
                // };
            } break;
            case LBrace: {
                std::vector<Expression> list_internal=parse_expression_list(parser, param_id_map, RBrace, Comma);
                expr=Expression{ExprList{std::move(list_internal)}};
            } break;
            case String: {
                return Expression{ExprList{string_to_exprlist(t.text)}};
            }
            case Chars: {
                std::vector<Expression> chars=string_to_exprlist(t.text);
                if(chars.size()!=1) {
                    throw std::runtime_error(std::format("char expression is expected to be of size 1, found {}",chars.size()));
                }
                return std::move(chars[0]);
            }
            case LParen: {
                expr=parse_expression(parser, param_id_map,0);
                if(parser.current().kind!=RParen) {
                    parse_error(parser.current(),{")"});
                }
                parser.advance();
            } break;
            case Hash:
            case HashHash: {
                return Expression{Never{}};
            }
            default : parse_error(t,{"expression"});
        }
    }
    while(true) {
        Token t=parser.current();
        auto [prefix_pri_left, prefix_pri_right, prefix_id] = infix_binding_power(t.kind);
        if(prefix_id==Eof || prefix_pri_left<pri) {
            break;
        }
        parser.advance();
        Expression expr2=parse_expression(parser, param_id_map, prefix_pri_right);
        std::vector<Expression> expr_list;
        expr_list.push_back(std::move(expr));
        expr_list.push_back(std::move(expr2));
        expr=Expression{CallInternal{prefix_id,std::move(expr_list)}};
    }
    return expr;
}

std::vector<Expression> Program::parse_expression_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, TokenKind end, TokenKind sep) {
    std::vector<Expression> expr_list;
    if(parser.current().kind==end) {
        parser.advance();
        return expr_list;
    } else {
        TokenKind separator_or_end=sep;
        while(separator_or_end!=end) {
            Token t=parser.current();
            if(t.kind==end) {
                parser.advance();
                return expr_list; // deal with trailing comma case
            } else if(t.kind==Identifier && constants.find(t.text)!=constants.end()) {
                std::vector<DataElement>& const_val=constants[t.text];
                for(auto& v : const_val) {
                    expr_list.push_back(Expression{Const{v}});
                }
                parser.advance();
            } else if(t.kind==Chars) {
                std::vector<Expression> chars=string_to_exprlist(t.text);
                expr_list.insert(expr_list.end(),std::make_move_iterator(chars.begin()),std::make_move_iterator(chars.end()));
                parser.advance();
            } else {
                expr_list.push_back(parse_expression(parser,param_id_map,0));
            }
            separator_or_end=parser.current().kind;
            if(separator_or_end!=sep && separator_or_end!=end) {
                parse_error(parser.current(),{token_kind_to_string(sep),token_kind_to_string(end)});
            }
            parser.advance();
        }
    }
    return expr_list;
}

void Program::parse_rule(Parser& parser) {
    if(parser.current().kind==Identifier) {
        std::string function=static_cast<std::string>(parser.current().text);
        parser.advance();
        if(parser.current().kind==LParen) {
            Rule rule;
            parser.advance();
            std::unordered_map<std::string, std::size_t> param_id_map;
            rule.left=parse_param_list(parser,param_id_map,RParen,Comma);
            if(parser.current().kind==ColonColon) {
                parser.advance();
                rule.guard=parse_expression(parser,param_id_map,0);
            } else {
                rule.guard=std::nullopt;
            }
            if(parser.current().kind!=Arrow) {
                parse_error(parser.current(),{"->","::"});
            }
            parser.advance();
            rule.right=parse_expression_list(parser,param_id_map,Semicolon,Comma);
            rule.names=indices_to_names(param_id_map);
            function_map.try_emplace(function,function_map.size());
            size_t function_id=function_map[function];
            if(function_id>=program.size()) {
                program.resize(function_id+1);
            }
            program[function_id].push_back(std::move(rule));
        } else {
            parse_error(parser.current(),{"("});
        }
    } else {
        parse_error(parser.current(),{"identifier"});
    }
}

