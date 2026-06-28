#pragma once

#include <cstdint>
#include <vector>

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
};
