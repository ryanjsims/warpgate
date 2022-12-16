#pragma once
#include <unordered_map>
#include <optional>

namespace warpgate::utils {
    extern std::unordered_map<uint32_t, std::string> bone_hashmap;
    extern std::unordered_map<uint32_t, uint32_t> bone_hierarchy;
    extern std::unordered_map<std::string, std::string> rigify_names;
}