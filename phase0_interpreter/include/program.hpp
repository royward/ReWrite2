// ============================================================================
// File: program.hpp
// Description: storage types for the program itself
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

#include <cstdint>
#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include "data_element.hpp"
#include "token_kind.hpp"

struct Parser;
struct Parameter;
struct Expression;
struct FourTokenKind {
    TokenKind a,b,c,d;
    inline bool check_token(TokenKind t) {return t==a || t==b || t==c || t==d;};
};

struct Id { uint32_t value; };
struct Const { DataElement value; };

struct ParamSplat { uint32_t value; };
struct ParamSplatWild {};
struct ParamList { std::vector<Parameter> items; };
struct ParamWildcard {};

using ParameterVariant = std::variant<Id, ParamSplat, ParamSplatWild, Const, ParamList, ParamWildcard>;

struct Parameter {
    ParameterVariant value;
};

struct ExprSplat { std::unique_ptr<Expression> inner; };
struct ExprList { std::vector<Expression> items; };
struct Call { uint32_t func_id; std::vector<Expression> args; };
struct CallInternal { TokenKind func_id; std::vector<Expression> args; };
struct CallLibrary { TokenKind func_id; std::vector<Expression> args; };
struct Never {};

using ExpressionVariant = std::variant <Id, ExprSplat, Const, ExprList, Call, CallInternal, CallLibrary, Never>;

struct Expression {
    ExpressionVariant value;
};

struct RuleMatch {
    std::vector<Parameter> match;
    std::vector<Expression> expr;
    uint32_t match_count;
    bool update;
};

struct Rule {
    RuleMatch main;
    std::vector<RuleMatch> pre_arrow;
    std::vector<RuleMatch> post_arrow;
    std::vector<std::string> names; // debugging only, and getting size for bind vector
};

class Program {
public:
    Program(std::string_view source);
    //std::vector<DataElement> run(const std::string& fn, const std::vector<DataElement>& args) const;
    std::vector<DataElement> run_string(std::string& call);
private:
    void do_call_single(const Expression& expression, const std::vector<DataElement>& bindings, std::vector<DataElement>& sofar) const;
    void do_call_multi(const std::vector<Expression>& expressions, const std::vector<DataElement>& bindings, std::vector<DataElement>& sofar) const;
    void do_call_function(uint32_t op, std::vector<DataElement>& sofar, std::vector<DataElement> args) const;
    // Implementations for parsing in parser.cpp
    void parse_rule(Parser& parser);
    void parse_const(Parser& parser);
    Parameter parse_param(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map);
    std::vector<Parameter> parse_param_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, FourTokenKind end, TokenKind sep);
    Expression parse_expression(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, uint8_t pri);
    std::vector<Expression> parse_expression_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, FourTokenKind end, TokenKind sep);
    // data
    std::vector<std::vector<Rule>> program;
    std::vector<std::string> function_names; // debugging only
    std::unordered_map<std::string, std::size_t> function_map; // only for lookups for call
    std::unordered_map<std::string, std::vector<DataElement>> constants; // only used at the parsing stage
};

bool compare_equal(const DataElement& x, const DataElement& y);

