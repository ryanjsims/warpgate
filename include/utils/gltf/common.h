#pragma once
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "tiny_gltf.h"
#include "version.h"


namespace warpgate::utils::gltf {
    int add_texture_to_gltf(
        tinygltf::Model &gltf, 
        std::filesystem::path texture_path, 
        std::filesystem::path output_filename, 
        std::optional<std::string> label = {}
    );

    void update_bone_transforms(tinygltf::Model &gltf, int skeleton_root);
}