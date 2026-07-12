// ============================================================================
// File: database_elements.hpp
// Description: DataElement definition - all data is worked with in this format
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
#include <string>
#include <variant>

inline constexpr uint32_t TYPE_UNBOUND = 0;
inline constexpr uint32_t TYPE_BOOL = 1;
inline constexpr uint32_t TYPE_I64 = 2;
inline constexpr uint32_t TYPE_LIST = 3;
inline constexpr uint32_t TYPE_CHAR = 4;

struct DataElement;

struct DataUnbound {};
struct DataBool { bool value; };
struct DataInt { int64_t value; };
struct DataList { std::vector<DataElement> value; };
struct DataChar { char value; };

using DataVariant = std::variant <DataUnbound, DataBool, DataInt, DataList, DataChar>;

struct DataElement {
    DataVariant value;
    uint32_t last_match=0;
    std::string to_string() const;
};
