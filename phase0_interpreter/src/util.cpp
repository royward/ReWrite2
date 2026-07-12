// ============================================================================
// File: util.cpp
// Description: miscellaneous utility functions
// ============================================================================
#include "util.hpp"

// convert a lookup map to an array. Assumes that values are 0..len-1 without repetition
std::vector<std::string> indices_to_names(const std::unordered_map<std::string, std::size_t>& name_to_index) {
    std::vector<std::string> result(name_to_index.size());
    for (const auto& [name, index] : name_to_index) {
        result[index] = name;
    }
    return result;
}
