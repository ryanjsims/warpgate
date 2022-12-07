#pragma once
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include <cnk0.h>
#include <cnk1.h>
#include "tiny_gltf.h"

namespace warpgate::utils::gltf::chunk {
    int add_mesh_to_gltf(
        tinygltf::Model &gltf,
        const CNK0 &chunk,
        int material_base_index,
        std::string name,
        bool include_colors = false
    );

    int add_materials_to_gltf(
        tinygltf::Model &gltf,
        const CNK1 &chunk,
        std::filesystem::path output_directory,
        std::string name,
        int sampler_index
    );

    tinygltf::Model build_gltf_from_chunks(
        const CNK0 &chunk0,
        const CNK1 &chunk1,
        std::filesystem::path output_directory, 
        bool export_textures,
        std::string name
    );
}