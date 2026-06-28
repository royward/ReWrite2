#include "program.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "util.hpp"
#include <stdexcept>
#include <print>
#include <ranges>
#include <string>

Program::Program(std::string_view source) {
    Parser parser;
    parser.tokens=lex(source);
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
        } else if constexpr (std::is_same_v<T, ParamSplat>) {
            throw std::runtime_error("internal error: do_match_single should not be matched with splat");
            return false; // make compiler happy
        } else if constexpr (std::is_same_v<T, Const>) {
            return compare_equal(x,alt.value);
        } else if constexpr (std::is_same_v<T, ParamList>) {
            return false; // TODO
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
            //const ParamSplat& splat = std::get<ParamSplat>(parameter.value);
            // TODO
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

DataElement do_call_internal(uint32_t op, const std::vector<DataElement>& args) {
    switch(args.size()) {
        case 1: {
            const DataElement& arg=args[0];
            std::size_t argtype=arg.value.index();
            switch(op) {
                case OP_UNARY_MINUS: {
                    if(argtype==TYPE_I64) {
                        return DataElement{DataInt{-std::get<DataInt>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type unary minus: {}",argtype));
                    }
                }
                default: throw std::runtime_error(std::format("unknown unary op: {}",op));
            }
        } break;
        case 2: {
            const DataElement& arg0=args[0];
            const DataElement& arg1=args[1];
            std::size_t argtype0=arg0.value.index();
            std::size_t argtype1=arg1.value.index();
            switch(op) {
                case OP_PLUS: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value+std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary plus: {},{}",argtype0,argtype1));
                    }
                }
                case OP_MINUS: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value-std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary minus: {},{}",argtype0,argtype1));
                    }
                }
                case OP_TIMES: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value*std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary times: {},{}",argtype0,argtype1));
                    }
                }
                case OP_DIVIDE: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("division by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value/std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary divide: {},{}",argtype0,argtype1));
                    }
                }
                case OP_MODULUS: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("modulus by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value%std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary modulus: {},{}",argtype0,argtype1));
                    }
                }
                default: throw std::runtime_error(std::format("unknown binary op: {}",op));
            }
        } break;
        default: throw std::runtime_error(std::format("do_call_internal only works with 1 or 2 args, found {}",args.size()));
    }
    return DataElement{DataUnbound{}}; // keep compiler happy
}

void Program::do_call_single(const Expression& expression, const std::vector<DataElement>& bindings, std::vector<DataElement>& sofar) const {
    std::visit([this,&expression,&bindings,&sofar](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, Id>) {
            sofar.push_back(bindings[alt.value]);
        } else if constexpr (std::is_same_v<T, ExprSplat>) {
            throw std::runtime_error("TODO"); // TODO
        } else if constexpr (std::is_same_v<T, Const>) {
            sofar.push_back(alt.value);
        } else if constexpr (std::is_same_v<T, ExprList>) {
            throw std::runtime_error("TODO"); // TODO
        } else if constexpr (std::is_same_v<T, Call>) {
            std::vector<DataElement> args;
            for(const Expression& e:alt.args) {
                do_call_single(e,bindings,args);
            }
            do_call(alt.func_id,sofar,args);
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

void Program::do_call(uint32_t op, std::vector<DataElement>& sofar, std::vector<DataElement> args) const {
    while(true) {
        // first find the right rule set
        const std::vector<Rule>& rules=program[op];
        // find the first match
        for(const Rule& rule: rules) {
            std::vector<DataElement> bindings(rule.names.size(),DataElement{DataUnbound{}});
            if(do_match_vec(rule.left,args,bindings)) { // TODO: guards
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
                    continue;
                }
            }
        }
        throw std::runtime_error(std::format("rule not matched: {}",op)); // TODO better error here
    }
}

std::vector<DataElement> Program::run(const std::string& fn, const std::vector<DataElement>& args) const {
    std::vector<DataElement> sofar;
    do_call(function_map.at(fn),sofar,args);
    return sofar;
}
