#include "utils/textures.h"

#include <spdlog/spdlog.h>

#include "utils/materials_3.h"
#include "stb_image_write.h"

namespace logger = spdlog;
using namespace warpgate;

std::string utils::textures::relabel_texture(std::string texture_name, std::string label) {
    size_t index = texture_name.find_last_of('_');
    if(index == std::string::npos) {
        return texture_name;
    }
    return texture_name.substr(0, index + 1) + label + texture_name.substr(index + 2);
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
    logger::debug("Processing normal map...");
    gli::texture2d texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    if(gli::is_compressed(texture.format())) {
        logger::trace("Compressed texture (format {})", (int)texture.format());
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
    logger::trace("Writing image of size ({}, {}) to {}", extent.x, extent.y, normal_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(unpacked_normal.get(), pixels.size()), normal_path, extent)) {
        logger::debug("Saved normal map to {}", normal_path.lexically_relative(output_directory).string());
    }

    std::string tint_name = relabel_texture(texture_name, "T");
    std::filesystem::path tint_path = normal_path.parent_path() / tint_name;
    tint_path.replace_extension(".png");
    logger::trace("Writing image of size ({}, {}) to {}", extent.x, extent.y, tint_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(tint_map.get(), pixels.size()), tint_path, extent)) {
        logger::debug("Saved tint map to {}", tint_path.lexically_relative(output_directory).string());
    }
}

void utils::textures::process_specular(std::string texture_name, std::vector<uint8_t> specular_data, std::vector<uint8_t> albedo_data, std::filesystem::path output_directory) {
    logger::debug("Processing specular...");
    gli::texture2d specular(gli::load_dds((char*)specular_data.data(), specular_data.size()));
    if(specular.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    if(gli::is_compressed(specular.format())) {
        logger::trace("Compressed texture (format {})", (int)specular.format());
        specular = gli::convert(specular, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    gli::texture2d albedo(gli::load_dds((char*)albedo_data.data(), albedo_data.size()));
    if(albedo.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load albedo from memory");
    }
    if(gli::is_compressed(albedo.format())) {
        logger::trace("Compressed texture (format {})", (int)albedo.format());
        albedo = gli::convert(albedo, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }

    // std::span<uint32_t> specular_pixels = std::span<uint32_t>(specular.data<uint32_t>(), specular.size<uint32_t>());
    // std::span<uint32_t> albedo_pixels = std::span<uint32_t>(albedo.data<uint32_t>(), albedo.size<uint32_t>());
    // std::unique_ptr<uint32_t[]> metallic_roughness = std::make_unique<uint32_t[]>(specular_pixels.size());
    // std::unique_ptr<uint32_t[]> emissive = std::make_unique<uint32_t[]>(specular_pixels.size());
    auto extent = specular.extent();
    // float ratio_s2a = (float)albedo_pixels.size() / specular_pixels.size();
    // for(int i = 0; i < specular_pixels.size(); i++) {
    //     uint32_t pixel = specular_pixels[i];
    //     uint32_t albedo_pixel = albedo_pixels[(int)(i * ratio_s2a)];
    //     //uint32_t value = ((((pixel & 0x0000FF00) >> 8 | (pixel & 0x000000FF) << 8) / 255) & 0xFF) << 16;
    //     uint32_t metal_rough = ((pixel & 0xFF000000) >> 16) | ((pixel & 0x000000FF) << 16) | 0xFF000000;
    //     uint32_t emissive_pixel = (((pixel & 0x00FF0000) > (50 << 16)) && (albedo_pixel & 0xFF000000) != 0) ? albedo_pixel : 0; 
    //     metallic_roughness[i] = metal_rough;
    //     emissive[i] = emissive_pixel;
    // }

    gli::texture2d metallic_roughness(gli::format::FORMAT_RGBA8_UNORM_PACK8, extent);
    gli::texture2d emissive(gli::format::FORMAT_RGBA8_UNORM_PACK8, extent);
    gli::sampler2d<float> color_sampler(albedo, gli::wrap::WRAP_REPEAT);
    gli::sampler2d<float> spec_sampler(specular, gli::wrap::WRAP_REPEAT);
    gli::sampler2d<float> mr_sampler(metallic_roughness, gli::wrap::WRAP_REPEAT);
    gli::sampler2d<float> e_sampler(emissive, gli::wrap::WRAP_REPEAT);
    // std::unique_ptr<uint32_t[]> metallic_roughness = std::make_unique<uint32_t[]>(extent.x * extent.y);
    // std::unique_ptr<uint32_t[]> emissive = std::make_unique<uint32_t[]>(extent.x * extent.y);

    auto albedo_extent = albedo.extent();
    float x_ratio = ((float)albedo_extent.x) / extent.x;
    float y_ratio = ((float)albedo_extent.y) / extent.y;
    for(int32_t x = 0; x < extent.x; x++) {
        for(int32_t y = 0; y < extent.y; y++) {
            gli::texture2d::extent_type texel_loc(x, y);
            glm::vec<4, float> spec_pixel = spec_sampler.texel_fetch(texel_loc, 0);
            glm::vec<4, float> albedo_pixel = color_sampler.texel_fetch({(int)(x * x_ratio), (int)(y * y_ratio)}, 0);

            if(spec_pixel.b > 0.2f && albedo_pixel.a != 0) {
                albedo_pixel.a = spec_pixel.b;
                e_sampler.texel_write(texel_loc, 0, albedo_pixel);
            }
            spec_pixel.g = spec_pixel.a;
            spec_pixel.b = spec_pixel.r;
            spec_pixel.r = 0;
            spec_pixel.a = 1.0f;
            mr_sampler.texel_write(texel_loc, 0, spec_pixel);
        }
    }

    std::string metallic_roughness_name = relabel_texture(texture_name, "MR");
    std::filesystem::path metallic_roughness_path(metallic_roughness_name);
    metallic_roughness_path.replace_extension(".png");
    metallic_roughness_path = output_directory / "textures" / metallic_roughness_path;
    logger::trace("Writing image of size ({}, {}) to {}", extent.x, extent.y, metallic_roughness_path.lexically_relative(output_directory).string());
    if(write_texture({metallic_roughness.data<uint32_t>(), (size_t)(extent.x * extent.y)}, metallic_roughness_path, extent)){
        logger::debug("Saved metallic roughness map to {}", metallic_roughness_path.lexically_relative(output_directory).string());
    }

    std::string emissive_name = relabel_texture(texture_name, "E");
    std::filesystem::path emissive_path = metallic_roughness_path.parent_path() / emissive_name;
    emissive_path.replace_extension(".png");
    logger::trace("Writing image of size ({}, {}) to {}", extent.x, extent.y, emissive_path.lexically_relative(output_directory).string());
    if(write_texture({emissive.data<uint32_t>(), (size_t)(extent.x * extent.y)}, emissive_path, extent)) {
        logger::debug("Saved emissive map to {}", emissive_path.lexically_relative(output_directory).string());
    }
}

void utils::textures::process_detailcube(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::debug("Saving detail cube {} as png...", texture_name);
    gli::texture_cube texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    std::filesystem::path texture_path(texture_name);
    logger::trace("Cube map info:");
    logger::trace("    Base level: {}", texture.base_level());
    logger::trace("    Max level:  {}", texture.max_level());
    logger::trace("    Base Face:  {}", texture.base_face());
    logger::trace("    Max Face:   {}", texture.max_face());
    logger::trace("    Base Layer: {}", texture.base_layer());
    logger::trace("    Max Layer:  {}", texture.max_layer());
    for(size_t face = 0; face < texture.faces(); face++){
        gli::texture2d face_texture = texture[face];
        if(gli::is_compressed(face_texture.format())) {
            logger::trace("Compressed detailcube texture (format {})", (int)texture.format());
            face_texture = gli::convert(face_texture, gli::format::FORMAT_RGBA8_UNORM_PACK8);
        }
        logger::trace("Cube map {} face info:", utils::materials3::detailcube_faces[face]);
        logger::trace("    Base level: {}", face_texture.base_level());
        logger::trace("    Max level:  {}", face_texture.max_level());
        logger::trace("    Base face:  {}", face_texture.base_face());
        logger::trace("    Max face:   {}", face_texture.max_face());
        logger::trace("    Base layer: {}", face_texture.base_layer());
        logger::trace("    Max layer:  {}", face_texture.max_layer());
        auto extent = face_texture.extent();
        texture_path = output_directory / "textures" / (std::filesystem::path(texture_name).stem().string() + "_" + utils::materials3::detailcube_faces.at(face));
        texture_path.replace_extension(".png");
        logger::trace("Writing image of size ({}, {}) to {}", extent.x, extent.y, texture_path.lexically_relative(output_directory).string());
        if(write_texture(std::span<uint32_t>(face_texture.data<uint32_t>(), face_texture.size<uint32_t>()), texture_path, extent)){
            logger::debug("   Saved face {} to {}", utils::materials3::detailcube_faces.at(face), texture_path.lexically_relative(output_directory).string());
        }
    }
}

std::optional<gli::texture2d> utils::textures::load_texture(std::string texture_name, std::vector<uint8_t>& texture_data) {
    gli::texture2d texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
        return {};
    }
    if(gli::is_compressed(texture.format())) {
        logger::trace("Compressed texture (format {})", (int)texture.format());
        texture = gli::convert(texture, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    return texture;
}

void utils::textures::save_texture(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::debug("Saving {} as png...", texture_name);
    std::optional<gli::texture2d> texture = load_texture(texture_name, texture_data);
    if(!texture.has_value()) {
        return;
    }
    std::filesystem::path texture_path(texture_name);
    texture_path.replace_extension(".png");
    texture_path = output_directory / "textures" / texture_path;
    auto extent = texture->extent();
    logger::trace("Writing image of size ({}, {}) to {}", extent.x, extent.y, texture_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(texture->data<uint32_t>(), texture->size<uint32_t>()), texture_path, extent)){
        logger::debug("Saved texture to {}", texture_path.lexically_relative(output_directory).string());
    }
}

void utils::textures::process_cnx_sny(std::string texture_name, std::span<uint8_t> cnx_data, std::span<uint8_t> sny_data, std::filesystem::path output_directory) {
    logger::debug("Processing color_nx/specular_ny maps for {}...", texture_name);
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
        logger::trace("Decompressing color nx map...");
        color_nx = gli::convert(color_nx, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }

    if(gli::is_compressed(specular_ny.format())) {
        logger::trace("Decompressing specular ny map...");
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
        logger::debug("Saved albedo texture to {}", texture_path.lexically_relative(output_directory).string());
    }

    texture_path = output_directory / "textures" / (texture_name + "_S");
    texture_path.replace_extension(".png");
    if(write_texture(sny_span, texture_path, extent)){
        logger::debug("Saved metallic roughness texture to {}", texture_path.lexically_relative(output_directory).string());
    }

    texture_path = output_directory / "textures" / (texture_name + "_N");
    texture_path.replace_extension(".png");
    if(write_texture(normal_map, texture_path, extent)){
        logger::debug("Saved normal map to {}", texture_path.lexically_relative(output_directory).string());
    }
}