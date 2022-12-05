#pragma once
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "dme.h"
#include "json.hpp"
#include "parameter.h"
#include "tiny_gltf.h"
#include "tsqueue.h"


namespace utils::gltf {
    int add_texture_to_gltf(
        tinygltf::Model &gltf, 
        std::filesystem::path texture_path, 
        std::filesystem::path output_filename, 
        std::optional<std::string> label = {}
    );

    int add_mesh_to_gltf(tinygltf::Model &gltf, const DME &dme, uint32_t index);

    void build_material(
        tinygltf::Model &gltf, 
        tinygltf::Material &material,
        const DME &dme, 
        uint32_t material_index, 
        std::unordered_map<uint32_t, uint32_t> &texture_indices, 
        utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
        std::filesystem::path output_directory
    );

    std::optional<tinygltf::TextureInfo> load_texture_info(
        tinygltf::Model &gltf,
        const DME &dme, 
        uint32_t i, 
        std::unordered_map<uint32_t, uint32_t> &texture_indices, 
        utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
        std::filesystem::path output_filename,
        Parameter::Semantic semantic
    );

    std::optional<std::pair<tinygltf::TextureInfo, tinygltf::TextureInfo>> load_specular_info(
        tinygltf::Model &gltf,
        const DME &dme, 
        uint32_t i, 
        std::unordered_map<uint32_t, uint32_t> &texture_indices, 
        utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
        std::filesystem::path output_filename,
        Parameter::Semantic semantic
    );

    std::vector<uint8_t> expand_vertex_stream(nlohmann::json &layout, std::span<uint8_t> data, uint32_t stream, bool is_rigid, const DME &dme);
}