#include "utils/textures.h"

#include <spdlog/spdlog.h>

#include "utils/materials_3.h"
#include "stb_image_write.h"

namespace logger = spdlog;
using namespace warpgate;

std::string utils::textures::relabel_texture(std::string texture_name, std::string label) {
    size_t index = texture_name.find_last_of('_') + 1;
    return texture_name.substr(0, index) + label + texture_name.substr(index + 1);
}

bool utils::textures::write_texture(std::span<uint32_t> data, std::filesystem::path texture_path, gli::texture2d::extent_type extent) {
    if(!stbi_write_png(
            texture_path.string().c_str(), 
            extent.x, extent.y, 
            4, 
            data.data(),
            4 * extent.x
        )) {
        logger::error("Failed to write to {}", texture_path.string());
        return false;
    }
    return true;
}

void utils::textures::process_normalmap(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::info("Processing normal map...");
    gli::texture2d texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    if(gli::is_compressed(texture.format())) {
        logger::debug("Compressed texture (format {})", (int)texture.format());
        texture = gli::convert(texture, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    std::span<uint32_t> pixels = std::span<uint32_t>(texture.data<uint32_t>(), texture.size<uint32_t>());
    std::unique_ptr<uint32_t[]> unpacked_normal = std::make_unique<uint32_t[]>(pixels.size());
    std::unique_ptr<uint32_t[]> tint_map = std::make_unique<uint32_t[]>(pixels.size());

    for(int i = 0; i < pixels.size(); i++) {
        uint32_t pixel = pixels[i];
        uint32_t unpack = ((pixel & 0xFF000000) >> 24) | (pixel & 0x0000FF00) | 0xFFFF0000;
        uint32_t tint = ((pixel & 0x00FF0000) < ( 50 << 16) ? 0x000000FF : 0) 
                    | ((pixel & 0x000000FF) < ( 50 <<  0) ? 0x0000FF00 : 0)
                    | ((pixel & 0x00FF0000) > (150 << 16) ? 0x00FF0000 : 0)
                    |                                       0xFF000000;
        unpacked_normal[i] = unpack;
        tint_map[i] = tint;
    }

    std::filesystem::path normal_path(texture_name);
    normal_path.replace_extension(".png");
    normal_path = output_directory / "textures" / normal_path;
    auto extent = texture.extent();
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, normal_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(unpacked_normal.get(), pixels.size()), normal_path, extent)) {
        logger::info("Saved normal map to {}", normal_path.lexically_relative(output_directory).string());
    }

    std::string tint_name = relabel_texture(texture_name, "T");
    std::filesystem::path tint_path = normal_path.parent_path() / tint_name;
    tint_path.replace_extension(".png");
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, tint_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(tint_map.get(), pixels.size()), tint_path, extent)) {
        logger::info("Saved tint map to {}", tint_path.lexically_relative(output_directory).string());
    }
}

void utils::textures::process_specular(std::string texture_name, std::vector<uint8_t> specular_data, std::vector<uint8_t> albedo_data, std::filesystem::path output_directory) {
    logger::info("Processing specular...");
    gli::texture2d specular(gli::load_dds((char*)specular_data.data(), specular_data.size()));
    if(specular.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    if(gli::is_compressed(specular.format())) {
        logger::debug("Compressed texture (format {})", (int)specular.format());
        specular = gli::convert(specular, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    gli::texture2d albedo(gli::load_dds((char*)albedo_data.data(), albedo_data.size()));
    if(albedo.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load albedo from memory");
    }
    if(gli::is_compressed(albedo.format())) {
        logger::debug("Compressed texture (format {})", (int)albedo.format());
        albedo = gli::convert(albedo, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }

    std::span<uint32_t> specular_pixels = std::span<uint32_t>(specular.data<uint32_t>(), specular.size<uint32_t>());
    std::span<uint32_t> albedo_pixels = std::span<uint32_t>(albedo.data<uint32_t>(), albedo.size<uint32_t>());
    std::unique_ptr<uint32_t[]> metallic_roughness = std::make_unique<uint32_t[]>(albedo_pixels.size());
    std::unique_ptr<uint32_t[]> emissive = std::make_unique<uint32_t[]>(albedo_pixels.size());
    auto extent = albedo.extent();
    int stride = (int)gli::component_count(albedo.format()) * extent.x;
    for(int i = 0; i < albedo_pixels.size(); i++) {
        uint32_t pixel = specular_pixels[i];
        uint32_t albedo_pixel = albedo_pixels[i];
        //uint32_t value = ((((pixel & 0x0000FF00) >> 8 | (pixel & 0x000000FF) << 8) / 255) & 0xFF) << 16;
        uint32_t metal_rough = ((pixel & 0xFF000000) >> 16) | ((pixel & 0x000000FF) << 16) | 0xFF000000;
        uint32_t emissive_pixel = (((pixel & 0x00FF0000) > (50 << 16)) && (albedo_pixel & 0xFF000000) != 0) ? albedo_pixel : 0; 
        metallic_roughness[i] = metal_rough;
        emissive[i] = emissive_pixel;
    }

    std::string metallic_roughness_name = relabel_texture(texture_name, "MR");
    std::filesystem::path metallic_roughness_path(metallic_roughness_name);
    metallic_roughness_path.replace_extension(".png");
    metallic_roughness_path = output_directory / "textures" / metallic_roughness_path;
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, metallic_roughness_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(metallic_roughness.get(), albedo_pixels.size()), metallic_roughness_path, extent)){
        logger::info("Saved metallic roughness map to {}", metallic_roughness_path.lexically_relative(output_directory).string());
    }

    std::string emissive_name = relabel_texture(texture_name, "E");
    std::filesystem::path emissive_path = metallic_roughness_path.parent_path() / emissive_name;
    emissive_path.replace_extension(".png");
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, emissive_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(emissive.get(), albedo_pixels.size()), emissive_path, extent)) {
        logger::info("Saved emissive map to {}", emissive_path.lexically_relative(output_directory).string());
    }
}

void utils::textures::process_detailcube(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::info("Saving detail cube {} as png...", texture_name);
    gli::texture_cube texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    std::filesystem::path texture_path(texture_name);
    logger::debug("Cube map info:");
    logger::debug("    Base level: {}", texture.base_level());
    logger::debug("    Max level:  {}", texture.max_level());
    logger::debug("    Base Face:  {}", texture.base_face());
    logger::debug("    Max Face:   {}", texture.max_face());
    logger::debug("    Base Layer: {}", texture.base_layer());
    logger::debug("    Max Layer:  {}", texture.max_layer());
    for(size_t face = 0; face < texture.faces(); face++){
        gli::texture2d face_texture = texture[face];
        if(gli::is_compressed(face_texture.format())) {
            logger::debug("Compressed detailcube texture (format {})", (int)texture.format());
            face_texture = gli::convert(face_texture, gli::format::FORMAT_RGBA8_UNORM_PACK8);
        }
        logger::debug("Cube map {} face info:", utils::materials3::detailcube_faces[face]);
        logger::debug("    Base level: {}", face_texture.base_level());
        logger::debug("    Max level:  {}", face_texture.max_level());
        logger::debug("    Base face:  {}", face_texture.base_face());
        logger::debug("    Max face:   {}", face_texture.max_face());
        logger::debug("    Base layer: {}", face_texture.base_layer());
        logger::debug("    Max layer:  {}", face_texture.max_layer());
        auto extent = face_texture.extent();
        texture_path = output_directory / "textures" / (std::filesystem::path(texture_name).stem().string() + "_" + utils::materials3::detailcube_faces.at(face));
        texture_path.replace_extension(".png");
        logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, texture_path.lexically_relative(output_directory).string());
        if(write_texture(std::span<uint32_t>(face_texture.data<uint32_t>(), face_texture.size<uint32_t>()), texture_path, extent)){
            logger::info("   Saved face {} to {}", utils::materials3::detailcube_faces.at(face), texture_path.lexically_relative(output_directory).string());
        }
    }
}

void utils::textures::save_texture(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::info("Saving {} as png...", texture_name);
    gli::texture2d texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
        return;
    }
    if(gli::is_compressed(texture.format())) {
        logger::debug("Compressed texture (format {})", (int)texture.format());
        texture = gli::convert(texture, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    std::filesystem::path texture_path(texture_name);
    texture_path.replace_extension(".png");
    texture_path = output_directory / "textures" / texture_path;
    auto extent = texture.extent();
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, texture_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(texture.data<uint32_t>(), texture.size<uint32_t>()), texture_path, extent)){
        logger::info("Saved texture to {}", texture_path.lexically_relative(output_directory).string());
    }
}

void utils::textures::process_cnx_sny(std::string texture_name, std::vector<uint8_t> cnx_data, std::vector<uint8_t> sny_data, std::filesystem::path output_directory) {
    logger::info("Processing color_nx/specular_ny maps for {}...", texture_name);
    gli::texture2d color_nx(gli::load_dds((char*)cnx_data.data(), cnx_data.size()));
    if(color_nx.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} color nx map from memory", texture_name);
        return;
    }
    
    gli::texture2d specular_ny(gli::load_dds((char*)sny_data.data(), sny_data.size()));
    if(specular_ny.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} specular ny map from memory", texture_name);
        return;
    }

    if(gli::is_compressed(color_nx.format())) {
        logger::debug("Decompressing color nx map...");
        color_nx = gli::convert(color_nx, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }

    if(gli::is_compressed(specular_ny.format())) {
        logger::debug("Decompressing specular ny map...");
        specular_ny = gli::convert(specular_ny, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }

    if(!(color_nx.extent().x == specular_ny.extent().x && color_nx.extent().y == specular_ny.extent().y)) {
        logger::error(
            "color_nx and specular_ny maps *must* have matching dimentions: ({}, {}) != ({}, {})", 
            color_nx.extent().x,
            color_nx.extent().y,
            specular_ny.extent().x,
            specular_ny.extent().y
        );
        return;
    }

    std::vector<uint32_t> normal_map;
    std::span<uint32_t> cnx_span(color_nx.data<uint32_t>(), color_nx.size<uint32_t>());
    std::span<uint32_t> sny_span(specular_ny.data<uint32_t>(), specular_ny.size<uint32_t>());
    for(uint32_t i = 0; i < cnx_span.size(); i++) {
        normal_map.push_back(0xFFFF0000 | ((sny_span[i] >> 16) & 0x0000FF00) | (cnx_span[i] >> 24));
        sny_span[i] |= 0xFF000000;
        sny_span[i] ^= 0x00FFFF00;
        cnx_span[i] |= 0xFF000000;
    }

    std::filesystem::path texture_path(texture_name + "_C");
    texture_path.replace_extension(".png");
    texture_path = output_directory / "textures" / texture_path;
    auto extent = color_nx.extent();
    if(write_texture(cnx_span, texture_path, extent)){
        logger::info("Saved albedo texture to {}", texture_path.lexically_relative(output_directory).string());
    }

    texture_path = texture_path.parent_path() / (texture_name + "_S");
    texture_path.replace_extension(".png");
    if(write_texture(sny_span, texture_path, extent)){
        logger::info("Saved metallic roughness texture to {}", texture_path.lexically_relative(output_directory).string());
    }

    texture_path = texture_path.parent_path() / (texture_name + "_N");
    texture_path.replace_extension(".png");
    if(write_texture(normal_map, texture_path, extent)){
        logger::info("Saved normal map to {}", texture_path.lexically_relative(output_directory).string());
    }
}