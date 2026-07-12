// ============================================================================
// File: database_elements.cpp
// Description: DataElement methods - currently only to_string
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

#include "data_element.hpp"
#include <stdexcept>

std::string display_single_char(char ch, char term) {
    if(ch==term) {
        return {'\\',term};
    }
    switch(ch) {
        case '\t':return {'\\','t'};
        case '\r':return {'\\','r'};
        case '\n':return {'\\','n'};
        default: return {ch};
    }
}

namespace detail {
    inline std::string to_string_visit(const DataUnbound&) {
        return "<unbound>";
        //throw std::runtime_error("Unbond variables cannot be displayed");
    }
    inline std::string to_string_visit(const DataBool& b) {
        return b.value ? "true" : "false";
    }
    inline std::string to_string_visit(const DataInt& i) {
        return std::to_string(i.value);
    }
    inline std::string to_string_visit(const DataChar& c) {
        std::string s="\'";
        s+=display_single_char(c.value,'\'');
        s+="\'";
        return s;
    }
    inline std::string to_string_visit(const DataList& l) {
        // first check if the whole list is a string
        if(l.value.size()==0) {
            return "{}";
        }
        bool is_string=true;
        for(const DataElement& e:l.value) {
            if(!std::holds_alternative<DataChar>(e.value)) {
                is_string=false;
                break;
            }
        }
        if(is_string) {
            // whole list is a string, display as such
            std::string out = "\"";
            for (const DataElement& e:l.value) {
                out += display_single_char(std::get<DataChar>(e.value).value,'\"');
            }
            out += "\"";
            return out;
        } else {
            std::string out = "{";
            for (std::size_t i = 0; i < l.value.size(); i++) {
                if (i != 0) out += ",";
                out += l.value[i].to_string();
            }
            out += "}";
            return out;
        }
    }
}

std::string DataElement::to_string() const {
    return std::visit([](const auto& alt) { return detail::to_string_visit(alt); }, value);
}
