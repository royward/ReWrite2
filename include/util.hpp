#pragma once

#include <vector>
#include <string>
#include <unordered_map>

std::vector<std::string> indices_to_names(const std::unordered_map<std::string, std::size_t>& name_to_index);
