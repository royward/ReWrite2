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

inline constexpr uint32_t OP_NO_MATCH = 0;
inline constexpr uint32_t OP_UNARY_MINUS = 1;
inline constexpr uint32_t OP_PLUS = 2;
inline constexpr uint32_t OP_MINUS = 3;
inline constexpr uint32_t OP_TIMES = 4;
inline constexpr uint32_t OP_DIVIDE = 5;
inline constexpr uint32_t OP_EQUAL = 6;
inline constexpr uint32_t OP_MODULUS = 7;

struct Parser;
struct Parameter;

struct Id { uint32_t value; };
struct Const { DataElement value; };

struct ParamSplat { std::unique_ptr<Parameter> inner; };
struct ParamList { std::vector<Parameter> items; };

using ParameterVariant = std::variant<Id, ParamSplat, Const, ParamList>;

struct Parameter {
    ParameterVariant value;
};

struct Expression; // forward declaration

struct ExprSplat { std::unique_ptr<Expression> inner; };
struct ExprList { std::vector<Expression> items; };
struct Call { uint32_t func_id; std::vector<Expression> args; };
struct CallInternal { uint32_t func_id; std::vector<Expression> args; };
struct Never {};

using ExpressionVariant = std::variant <Id, ExprSplat, Const, ExprList, Call, CallInternal, Never>;

struct Expression {
    ExpressionVariant value;
};

struct Rule {
    std::vector<Parameter> left;
    std::vector<Expression> right;
    std::optional<Expression> guard;
    std::vector<std::string> names; // debugging only, and getting size for bind vector
};

class Program {
public:
    Program(std::string_view source);
    std::vector<DataElement> run(const std::string& fn, const std::vector<DataElement>& args) const;
private:
    void do_call_single(const Expression& expression, const std::vector<DataElement>& bindings, std::vector<DataElement>& sofar) const;
    void do_call(uint32_t op, std::vector<DataElement>& sofar, std::vector<DataElement> args) const;
    // Implementations for parsing in parser.cpp
    void parse_rule(Parser& parser);
    Expression parse_expression(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, uint8_t pri);
    std::vector<Expression> parse_expression_list(Parser& parser, std::unordered_map<std::string, std::size_t> &param_id_map, TokenKind end, TokenKind sep);
    // data
    std::vector<std::vector<Rule>> program;
    std::vector<std::string> function_names; // debugging only
    std::unordered_map<std::string, std::size_t> function_map; // only for lookups for call
};
