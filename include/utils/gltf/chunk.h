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
        std::string name
    );
}