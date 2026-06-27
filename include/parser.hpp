#include <cstdint>
#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>

inline constexpr uint32_t OP_NO_MATCH = 0;
inline constexpr uint32_t OP_UNARY_MINUS = 1;
inline constexpr uint32_t OP_PLUS = 2;
inline constexpr uint32_t OP_MINUS = 3;
inline constexpr uint32_t OP_TIMES = 4;
inline constexpr uint32_t OP_DIVIDE = 5;
inline constexpr uint32_t OP_EQUAL = 6;

inline constexpr uint32_t TYPE_BOOL = 0;
inline constexpr uint32_t TYPE_I64 = 1;
inline constexpr uint32_t TYPE_LIST = 2;

struct Parameter;  // forward declaration

struct ParamId { uint32_t value; };
struct ParamSplat { std::unique_ptr<Parameter> inner; };
struct ParamConst { uint16_t width; int64_t value; };
struct ParamList { std::vector<Parameter> items; };

using ParameterVariant = std::variant<ParamId, ParamSplat, ParamConst, ParamList>;

struct Parameter {
    ParameterVariant value;
};

struct Expression; // forward declaration

struct ExprId { uint32_t value; };
struct ExprSplat { std::unique_ptr<Expression> inner; };
struct ExprConst { uint16_t width; int64_t value; };
struct ExprList { std::vector<Expression> items; };
struct Call { uint32_t func_id; std::vector<Expression> args; };
struct CallInternal { uint32_t func_id; std::vector<Expression> args; };
struct Never {};

using ExpressionVariant = std::variant <
    ExprId, ExprSplat, ExprConst, ExprList, Call, CallInternal, Never
>;

struct Expression {
    ExpressionVariant value;
};

struct Rule {
    std::vector<Parameter> left;
    std::vector<Expression> right;
    std::optional<Expression> guard;
    std::vector<std::string> names; // debugging only
};

struct Program {
    std::vector<std::vector<Rule>> program;
    std::vector<std::string> function_names; // debugging only
    std::unordered_map<std::string, std::size_t> function_map; // only for lookups for call
};




