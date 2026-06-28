#include "util.hpp"

std::vector<std::string> indices_to_names(const std::unordered_map<std::string, std::size_t>& name_to_index) {
    std::vector<std::string> result(name_to_index.size());
    for (const auto& [name, index] : name_to_index) {
        result[index] = name;
    }
    return result;
}
