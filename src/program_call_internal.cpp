// ============================================================================
// File: program_call_internal.cpp
// Description: The internal "library functions"
// In a separate file because it is separate from the main execution logic and might get long
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
#include "program.hpp"

// https://en.cppreference.com/cpp/utility/functional for all the cool types stuff

template<typename InType, typename OutType, typename Op>
DataElement binary_op(const char* op_name, const DataElement& a, const DataElement& b, Op op) {
    if (!std::holds_alternative<InType>(a.value) || !std::holds_alternative<InType>(b.value)) {
        throw std::runtime_error(std::format("wrong types binary {}: {},{}", op_name, a.value.index(), b.value.index()));
    }
    return DataElement{OutType{op(std::get<InType>(a.value).value, std::get<InType>(b.value).value)}};
}

DataElement do_call_internal(TokenKind op, const std::vector<DataElement>& args) {
    switch(args.size()) {
        case 1: {
            const DataElement& arg=args[0];
            std::size_t argtype=arg.value.index();
            switch(op) {
                case Minus: {
                    if(argtype==TYPE_I64) {
                        return DataElement{DataInt{-std::get<DataInt>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type unary minus: {}",argtype));
                    }
                }
                case CountTrailingZeros: {
                    if(argtype==TYPE_I64) {
                        int64_t x=std::get<DataInt>(arg.value).value;
                        return DataElement{DataInt{std::countr_zero(static_cast<uint64_t>(x))}};
                    } else {
                        throw std::runtime_error(std::format("wrong type count trailing zeros: {}",argtype));
                    }
                }
                case Not: {
                    if(argtype==TYPE_BOOL) {
                        return DataElement{DataBool{!std::get<DataBool>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type not: {}",argtype));
                    }
                }
                default: throw std::runtime_error(std::format("unknown unary op: {}",(int)op));
            }
        } break;
        case 2: {
            const DataElement& arg0=args[0];
            const DataElement& arg1=args[1];
            std::size_t argtype0=arg0.value.index();
            std::size_t argtype1=arg1.value.index();
            switch(op) {
                case EqualEqual: { // works for all types
                    return DataElement{DataBool{compare_equal(arg0,arg1)}};
                }
                case NotEqual: { // works for all types
                    return DataElement{DataBool{!compare_equal(arg0,arg1)}};
                }
                case Plus: return binary_op<DataInt, DataInt>("plus", arg0, arg1, std::plus<>{});
                case Minus: return binary_op<DataInt, DataInt>("minus", arg0, arg1, std::minus<>{});
                case Times: return binary_op<DataInt, DataInt>("times", arg0, arg1, std::multiplies<>{});
                case Greater: return binary_op<DataInt, DataBool>("greater", arg0, arg1, std::greater<>{});
                case Less: return binary_op<DataInt, DataBool>("less", arg0, arg1, std::less<>{});
                case GreaterEqual: return binary_op<DataInt, DataBool>("greater_equal", arg0, arg1, std::greater_equal<>{});
                case LessEqual: return binary_op<DataInt, DataBool>("less_equal", arg0, arg1, std::less_equal<>{});
                case AndAnd: return binary_op<DataBool, DataBool>("logical_and", arg0, arg1, std::logical_and<>{});
                case OrOr: return binary_op<DataBool, DataBool>("logical_or", arg0, arg1, std::logical_or<>{});
                case And: return binary_op<DataInt, DataInt>("bit_and", arg0, arg1, std::bit_and<>{});
                case Or: return binary_op<DataInt, DataInt>("bit_or", arg0, arg1, std::bit_or<>{});
                case Xor: return binary_op<DataInt, DataInt>("bit_xor", arg0, arg1, std::bit_xor<>{});
                 case ShiftLeft: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value < 0) {
                            throw std::runtime_error("left shift by negative");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value<<std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types left shift: {},{}",argtype0,argtype1));
                    }
                }
                 case ShiftRight: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value < 0) {
                            throw std::runtime_error("right shift by negative");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value>>std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types right shift: {},{}",argtype0,argtype1));
                    }
                }
                 case Divide: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("division by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value/std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary divide: {},{}",argtype0,argtype1));
                    }
                }
                case Modulus: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("modulus by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value%std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary modulus: {},{}",argtype0,argtype1));
                    }
                }
                default: throw std::runtime_error(std::format("unknown binary op: {}",(int)op));
            }
        } break;
        default: throw std::runtime_error(std::format("do_call_internal only works with 1 or 2 args, found {}",args.size()));
    }
    return DataElement{DataUnbound{}}; // keep compiler happy
}
