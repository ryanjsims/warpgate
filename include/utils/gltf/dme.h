#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include <dme.h>
#include "json.hpp"
#include "parameter.h"
#include "tiny_gltf.h"
#include "utils/tsqueue.h"
#include "version.h"

namespace warpgate::utils::gltf::dme {
    int add_mesh_to_gltf(tinygltf::Model &gltf, const DME &dme, uint32_t index, uint32_t material_index);
    void add_skeleton_to_gltf(tinygltf::Model &gltf, const DME &dme, std::vector<int> mesh_nodes);
    tinygltf::Model build_gltf_from_dme(
        const DME &dme, 
        tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue, 
        std::filesystem::path output_directory, 
        bool export_textures, 
        bool include_skeleton
    );
    std::vector<uint8_t> expand_vertex_stream(
        nlohmann::json &layout, 
        std::span<uint8_t> data, 
        uint32_t stream, 
        bool is_rigid, 
        const DME &dme
    );
}