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

namespace detail {
    inline std::string to_string_visit(const DataUnbound&) {
        throw std::runtime_error("Unbond variables cannot be displayed");
    }
    inline std::string to_string_visit(const DataBool& b) {
        return b.value ? "true" : "false";
    }
    inline std::string to_string_visit(const DataInt& i) {
        return std::to_string(i.value);
    }
    inline std::string to_string_visit(const DataChar& c) {
        return std::string(1, c.value);
    }
    inline std::string to_string_visit(const DataList& l) {
        std::string out = "{";
        for (std::size_t i = 0; i < l.value.size(); i++) {
            if (i != 0) out += ",";
            out += l.value[i].to_string();
        }
        out += "}";
        return out;
    }
}

std::string DataElement::to_string() const {
    return std::visit([](const auto& alt) { return detail::to_string_visit(alt); }, value);
}
