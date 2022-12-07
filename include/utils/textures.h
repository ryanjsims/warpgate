#pragma once
#include <filesystem>
#include <span>

#include "gli/gli.hpp"

namespace warpgate::utils::textures {
    std::string relabel_texture(std::string texture_name, std::string label);

    bool write_texture(std::span<uint32_t> data, std::filesystem::path texture_path, gli::texture2d::extent_type extent);

    void process_normalmap(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory);

    void process_specular(std::string texture_name, std::vector<uint8_t> specular_data, std::vector<uint8_t> albedo_data, std::filesystem::path output_directory);

    void process_detailcube(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory);

    void save_texture(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory);

    void process_cnx_sny(std::string texture_name, std::vector<uint8_t> cnx_data, std::vector<uint8_t> sny_data, std::filesystem::path output_directory);
}