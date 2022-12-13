#pragma once
#include <fstream>
#include <optional>
#include <vector>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

#include "tiny_gltf.h"
#include "json.hpp"

namespace warpgate::utils::materials3 {
    extern std::vector<std::string> detailcube_faces;

    extern std::unordered_map<std::string, std::string> usages;

    extern std::unordered_map<std::string, int> sizes;

    extern std::unordered_map<std::string, int> component_types;

    extern std::unordered_map<std::string, int> types;

    extern nlohmann::json materials;
    void init_materials();

    std::optional<nlohmann::json> get_input_layout(uint32_t material_definition);
}