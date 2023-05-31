#pragma once
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include <dmat.h>
#include <synthium/manager.h>
#include "parameter.h"
#include "tiny_gltf.h"
#include "utils/tsqueue.h"
#include "version.h"

namespace warpgate::utils::gltf::dmat {
    int add_material_to_gltf(
        tinygltf::Model &gltf, 
        const DMAT &dmat, 
        uint32_t material_index,
        int sampler_index,
        bool export_textures,
        std::unordered_map<uint32_t, uint32_t> &texture_indices,
        std::unordered_map<uint32_t, std::vector<uint32_t>> &material_indices,
        tsqueue<std::pair<std::string, Semantic>> &image_queue,
        std::filesystem::path output_directory,
        std::string dme_name
    );

    void process_images(
        synthium::Manager& manager, 
        utils::tsqueue<std::pair<std::string, Semantic>>& queue, 
        std::shared_ptr<std::filesystem::path> output_directory
    );

    void build_material(
        tinygltf::Model &gltf, 
        tinygltf::Material &material,
        const DMAT &dmat, 
        uint32_t material_index, 
        std::unordered_map<uint32_t, uint32_t> &texture_indices,
        tsqueue<std::pair<std::string, Semantic>> &image_queue,
        std::filesystem::path output_directory,
        int sampler
    );

    std::optional<tinygltf::TextureInfo> load_texture_info(
        tinygltf::Model &gltf,
        const DMAT &dmat, 
        uint32_t i, 
        std::unordered_map<uint32_t, uint32_t> &texture_indices, 
        tsqueue<std::pair<std::string, Semantic>> &image_queue,
        std::filesystem::path output_filename,
        Semantic semantic,
        int sampler
    );

    std::optional<std::pair<tinygltf::TextureInfo, tinygltf::TextureInfo>> load_specular_info(
        tinygltf::Model &gltf,
        const DMAT &dmat, 
        uint32_t i, 
        std::unordered_map<uint32_t, uint32_t> &texture_indices, 
        tsqueue<std::pair<std::string, Semantic>> &image_queue,
        std::filesystem::path output_filename,
        Semantic semantic,
        int sampler
    );
}