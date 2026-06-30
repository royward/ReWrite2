// ============================================================================
// File: program.cpp
// Description: Run a program. Main language logic is in here
// ============================================================================
#include "program.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "util.hpp"
#include <stdexcept>
#include <print>
#include <ranges>
#include <string>

DataElement do_call_internal(TokenKind op, const std::vector<DataElement>& args);

Program::Program(std::string_view source) {
    Parser parser;
    parser.tokens=lex(source);
    // for(auto t:parser.tokens) {
    //     std::println("{}",t.to_string());
    // }
    while(!parser.eof()) {
        parse_rule(parser);
    }
    function_names=indices_to_names(function_map);
}

bool compare_equal(const DataElement& x, const DataElement& y) {
    if(x.value.index()==TYPE_UNBOUND || y.value.index()==TYPE_UNBOUND) {
        throw std::runtime_error("compare_equal called with unbound type");
    }
    if(x.value.index()!=y.value.index()) {
        return false;
    }
    switch(x.value.index()) {
        case TYPE_BOOL: {
            return std::get<DataBool>(x.value).value==std::get<DataBool>(y.value).value;
        }
        case TYPE_I64: {
            return std::get<DataInt>(x.value).value==std::get<DataInt>(y.value).value;
        }
        case TYPE_LIST: {
            const std::vector<DataElement>& x0=std::get<DataList>(x.value).value;
            const std::vector<DataElement>& y0=std::get<DataList>(y.value).value;
            if(x0.size()!=y0.size()) {
                return false;
            }
            for(std::size_t i=0; i<x0.size(); i++) {
                if(!compare_equal(x0[i],y0[i])) {
                    return false;
                }
            }
            return true;
        }
        case TYPE_CHAR: {
            return std::get<DataChar>(x.value).value==std::get<DataChar>(y.value).value;
        }
        default: throw std::runtime_error("compare_equal called with unknown type");
    }
}

bool do_match_vec(const std::vector<Parameter>& parameters, const std::vector<DataElement>& values, std::vector<DataElement>& bindings);


bool do_match_single(const Parameter& parameter, const DataElement& x, std::vector<DataElement>& bindings) {
    return std::visit([&bindings, &x](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, Id>) {
            DataElement& v=bindings[alt.value];
            if(v.value.index()==TYPE_UNBOUND) {
                v=x;
                return true;
            } else {
                return compare_equal(x,v);
            }
        } else if constexpr (std::is_same_v<T, ParamWildcard>) {
            return true; // match with anything
        } else if constexpr (std::is_same_v<T, ParamSplat>) {
            throw std::runtime_error("internal error: do_match_single should not be matched with splat");
            return false; // make compiler happy
        } else if constexpr (std::is_same_v<T, ParamSplatWild>) {
            throw std::runtime_error("internal error: do_match_single should not be matched with splat");
            return false; // make compiler happy
        } else if constexpr (std::is_same_v<T, Const>) {
            return compare_equal(x,alt.value);
        } else if constexpr (std::is_same_v<T, ParamList>) {
            if(!std::holds_alternative<DataList>(x.value)) {
                return false;
            }
            return do_match_vec(alt.items,std::get<DataList>(x.value).value,bindings);
        }
    }, parameter.value);
}

bool do_match_vec(const std::vector<Parameter>& parameters, const std::vector<DataElement>& values, std::vector<DataElement>& bindings) {
    std::size_t plen=parameters.size();
    std::size_t vlen=values.size();
    std::size_t pi=0;
    std::size_t vi=0;
    while(pi<plen) {
        const Parameter& parameter=parameters[pi];
        if (std::holds_alternative<ParamSplat>(parameter.value)) {
            const ParamSplat& splat = std::get<ParamSplat>(parameter.value);
            if(plen>vlen+1) {
                throw std::runtime_error("failed to get value with '..': parameter list too short");
            }
            // we get just enough stuff in the splat that the rest of the parameters will match up exactly
            std::vector<DataElement> sub_vector(values.begin() + vi, values.begin() + vi + (vlen-plen+1));
            vi+=vlen-plen+1;
            DataElement x=DataElement{DataList(std::move(sub_vector))};
            DataElement& v=bindings[splat.value];
            if(v.value.index()==TYPE_UNBOUND) {
                v=x;
            } else {
                if(!compare_equal(x,v)) {
                    return false;
                }
            }
        } else if (std::holds_alternative<ParamSplatWild>(parameter.value)) {
            if(plen>vlen+1) {
                throw std::runtime_error("failed to get value with '..': parameter list too short");
            }
            // we get just enough stuff in the splat that the rest of the parameters will match up exactly
            vi+=vlen-plen+1;
         } else {
            if(vi>=vlen) {
                return false; // ran out of parameters
            }
            if(!do_match_single(parameter,values[vi],bindings)) {
                return false;
            }
            vi++;
        }
        pi++;
    }
    return vi==vlen; // didn't have extra parameters
}

void Program::do_call_single(const Expression& expression, const std::vector<DataElement>& bindings, std::vector<DataElement>& sofar) const {
    std::visit([this,&expression,&bindings,&sofar](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, Id>) {
            sofar.push_back(bindings[alt.value]);
        } else if constexpr (std::is_same_v<T, ExprSplat>) {
            std::vector<DataElement> splattable;
            do_call_single(*alt.inner,bindings,splattable);
            for(auto& e:splattable) {
                if(!holds_alternative<DataList>(e.value)) {
                    throw std::runtime_error("splat only works on lists");
                }
                sofar.insert(sofar.end(),get<DataList>(e.value).value.begin(),get<DataList>(e.value).value.end());
            }
        } else if constexpr (std::is_same_v<T, Const>) {
            sofar.push_back(alt.value);
        } else if constexpr (std::is_same_v<T, ExprList>) {
            std::vector<DataElement> list_internal;
            for(const Expression& e:alt.items) {
                do_call_single(e,bindings,list_internal);
            }
            sofar.push_back(DataElement{DataList{std::move(list_internal)}});
        } else if constexpr (std::is_same_v<T, Call>) {
            std::vector<DataElement> args;
            for(const Expression& e:alt.args) {
                do_call_single(e,bindings,args);
            }
            do_call_function(alt.func_id,sofar,args);
        } else if constexpr (std::is_same_v<T, CallInternal>) {
            std::vector<DataElement> args;
            for(const Expression& e:alt.args) {
                do_call_single(e,bindings,args);
            }
            sofar.push_back(do_call_internal(alt.func_id,args));
        } else if constexpr (std::is_same_v<T, Never>) {
            throw std::runtime_error("evaluated Never expression");
        }
    }, expression.value);
}

void Program::do_call_function(uint32_t op, std::vector<DataElement>& sofar, std::vector<DataElement> args) const {
// NOTE that the start: and goto are necessary here. Tail recursion requires that f(...) -> g(...)
// doesn't create a new stack frame to call g, so don't want to make a recursive call. One usual way
// of doing this would be while(true) { ... continue ... }, but because there are multiple rules to
// be checked, the continue would need to break out of two levels, which is not supported. A flag could
// also be used to avoid the goto, but that's extra machinery, where the goto is clear.
start:
    // first find the right rule set
    const std::vector<Rule>& rules=program[op];
    // find the first match
    for(const Rule& rule: rules) {
        std::vector<DataElement> bindings(rule.names.size(),DataElement{DataUnbound{}});
        if(do_match_vec(rule.left,args,bindings)) {
            bool guard_ok=true;
            if (rule.guard.has_value()) {
                std::vector<DataElement> guard_sofar;
                do_call_single(*rule.guard,bindings,guard_sofar);
                if(guard_sofar.size()!=1 || !std::holds_alternative<DataBool>(guard_sofar[0].value)) {
                    throw std::runtime_error("Guard must return a single value bool");
                }
                guard_ok=get<DataBool>(guard_sofar[0].value).value;
            }
            if(guard_ok) {
                // found matching rule, now evaluate right side, being careful to handle tail recursion
                std::size_t process_with_tail=0;
                if(rule.right.size()>0 && std::holds_alternative<Call>(rule.right[rule.right.size()-1].value)) {
                    process_with_tail=1;
                }
                for (const Expression& expr : rule.right | std::views::take(rule.right.size() - process_with_tail)) {
                    do_call_single(expr,bindings,sofar);
                }
                if(process_with_tail==0) {
                    return;
                } else {
                    // Tail recursion here
                    const Call& call=std::get<Call>(rule.right[rule.right.size()-1].value);
                    op=call.func_id;
                    std::vector<DataElement> new_args;
                    for(const Expression& e:call.args) {
                        do_call_single(e,bindings,new_args);
                    }
                    args = std::move(new_args);
                    goto start; // tail recursion - call the next function without creating a stack frame
                }
            }
        }
    }
    std::string error=std::format("rule not matched: {}(",function_names[op]);
    for(std::size_t i=0;i<args.size();i++) {
        if(i!=0) {
            error+=",";
        }
        error+=args[i].to_string();
    }
    error+=")";
    throw std::runtime_error(error);
}

std::vector<DataElement> Program::run_string(std::string& call) {
    Parser parser;
    parser.tokens=lex(call);
    std::unordered_map<std::string, std::size_t> param_id_map;
    std::vector<Expression> expressions=parse_expression_list(parser, param_id_map, Eof, Comma);
    const std::vector<DataElement> empty_bindings;
    std::vector<DataElement> result;
    for(Expression& e : expressions) {
        do_call_single(e, empty_bindings, result);
    }
    return result;
}

std::vector<DataElement> Program::run(const std::string& fn, const std::vector<DataElement>& args) const {
    std::vector<DataElement> sofar;
    do_call_function(function_map.at(fn),sofar,args);
    return sofar;
}
