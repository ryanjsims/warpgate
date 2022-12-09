#pragma once
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include <cnk0.h>
#include <cnk1.h>
#include "tiny_gltf.h"
#include "utils/tsqueue.h"

namespace warpgate::utils::gltf::chunk {
    int add_chunks_to_gltf(
        tinygltf::Model &gltf,
        const warpgate::chunk::CNK0 &chunk0,
        const warpgate::chunk::CNK1 &chunk1,
        utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>> &image_queue,
        std::filesystem::path output_directory, 
        std::string name,
        int sampler_index,
        bool export_textures
    );
    
    int add_mesh_to_gltf(
        tinygltf::Model &gltf,
        const warpgate::chunk::CNK0 &chunk,
        int material_base_index,
        std::string name,
        bool include_colors = false
    );

    int add_materials_to_gltf(
        tinygltf::Model &gltf,
        const warpgate::chunk::CNK1 &chunk,
        utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>> &image_queue,
        std::filesystem::path output_directory,
        std::string name,
        int sampler_index
    );

    tinygltf::Model build_gltf_from_chunks(
        const warpgate::chunk::CNK0 &chunk0,
        const warpgate::chunk::CNK1 &chunk1,
        std::filesystem::path output_directory, 
        bool export_textures,
        utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>> &image_queue,
        std::string name
    );
}