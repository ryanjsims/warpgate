#include <fstream>
#include <filesystem>
#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <synthium/synthium.h>
#include <gli/load_dds.hpp>
#include <gli/convert.hpp>
#include <gli/sampler2d.hpp>
#include <glm/matrix.hpp>

#define _USE_MATH_DEFINES
#include <math.h>
//#include <glm/gtc/type_ptr.hpp>
#include "stb_image_write.h"

#include "argparse/argparse.hpp"
#include "dme_loader.h"
#include "utils.h"
#include "tsqueue.h"
#include "tiny_gltf.h"
#include "json.hpp"
#include "half.hpp"
#include "version.h"

namespace gltf2 = tinygltf;
namespace logger = spdlog;
using namespace nlohmann;
using half_float::half;

std::vector<std::string> detailcube_faces = {"+x", "-x", "+y", "-y", "+z", "-z"};

std::unordered_map<std::string, std::string> usages = {
    {"Position", "POSITION"},
    {"Normal", "NORMAL"},
    {"Binormal", "BINORMAL"},
    {"Tangent", "TANGENT"},
    {"BlendWeight", "WEIGHTS_0"},
    {"BlendIndices", "JOINTS_0"},
    {"Texcoord", "TEXCOORD_"},
    {"Color", "COLOR_"},
};

std::unordered_map<std::string, int> sizes = {
    {"Float3", 12},
    {"D3dcolor", 4},
    {"Float2", 8},
    {"Float4", 16},
    {"ubyte4n", 4},
    {"Float16_2", 4},
    {"float16_2", 4},
    {"Short2", 4},
    {"Float1", 4},
    {"Short4", 8}
};

std::unordered_map<std::string, int> component_types = {
    {"Float3", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"D3dcolor", TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE},
    {"Float2", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"Float4", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"ubyte4n", TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE},
    {"Float16_2", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"float16_2", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"Short2", TINYGLTF_COMPONENT_TYPE_SHORT},
    {"Float1", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"Short4", TINYGLTF_COMPONENT_TYPE_SHORT}
};

std::unordered_map<std::string, int> types = {
    {"Float3", TINYGLTF_TYPE_VEC3},
    {"D3dcolor", TINYGLTF_TYPE_VEC4},
    {"Float2", TINYGLTF_TYPE_VEC2},
    {"Float4", TINYGLTF_TYPE_VEC4},
    {"ubyte4n", TINYGLTF_TYPE_VEC4},
    {"Float16_2", TINYGLTF_TYPE_VEC2},
    {"float16_2", TINYGLTF_TYPE_VEC2},
    {"Short2", TINYGLTF_TYPE_VEC2},
    {"Float1", TINYGLTF_TYPE_SCALAR},
    {"Short4", TINYGLTF_TYPE_VEC4}
};

json materials;
void init_materials() {
    std::ifstream materials_file(MATERIALS_JSON_LOCATION);
    if(materials_file.fail()) {
        logger::error("Could not open materials.json: looking at location {}", MATERIALS_JSON_LOCATION);
        std::exit(1);
    }
    materials = json::parse(materials_file);
}

json get_input_layout(uint32_t material_definition) {
    std::string input_layout_name = materials
        .at("materialDefinitions")
        .at(std::to_string(material_definition))
        .at("drawStyles")
        .at(0)
        .at("inputLayout")
        .get<std::string>();
    
    return materials.at("inputLayouts").at(input_layout_name);
}

bool write_texture(std::span<uint32_t> data, std::filesystem::path texture_path, gli::texture2d::extent_type extent) {
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

void process_normalmap(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
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

    std::string tint_name = utils::relabel_texture(texture_name, "T");
    std::filesystem::path tint_path = normal_path.parent_path() / tint_name;
    tint_path.replace_extension(".png");
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, tint_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(tint_map.get(), pixels.size()), tint_path, extent)) {
        logger::info("Saved tint map to {}", tint_path.lexically_relative(output_directory).string());
    }
}

void process_specular(std::string texture_name, std::vector<uint8_t> specular_data, std::vector<uint8_t> albedo_data, std::filesystem::path output_directory) {
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

    std::string metallic_roughness_name = utils::relabel_texture(texture_name, "MR");
    std::filesystem::path metallic_roughness_path(metallic_roughness_name);
    metallic_roughness_path.replace_extension(".png");
    metallic_roughness_path = output_directory / "textures" / metallic_roughness_path;
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, metallic_roughness_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(metallic_roughness.get(), albedo_pixels.size()), metallic_roughness_path, extent)){
        logger::info("Saved metallic roughness map to {}", metallic_roughness_path.lexically_relative(output_directory).string());
    }

    std::string emissive_name = utils::relabel_texture(texture_name, "E");
    std::filesystem::path emissive_path = metallic_roughness_path.parent_path() / emissive_name;
    emissive_path.replace_extension(".png");
    logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, emissive_path.lexically_relative(output_directory).string());
    if(write_texture(std::span<uint32_t>(emissive.get(), albedo_pixels.size()), emissive_path, extent)) {
        logger::info("Saved emissive map to {}", emissive_path.lexically_relative(output_directory).string());
    }
}

void process_detailcube(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
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
        logger::debug("Cube map {} face info:", detailcube_faces[face]);
        logger::debug("    Base level: {}", face_texture.base_level());
        logger::debug("    Max level:  {}", face_texture.max_level());
        logger::debug("    Base face:  {}", face_texture.base_face());
        logger::debug("    Max face:   {}", face_texture.max_face());
        logger::debug("    Base layer: {}", face_texture.base_layer());
        logger::debug("    Max layer:  {}", face_texture.max_layer());
        auto extent = face_texture.extent();
        texture_path = output_directory / "textures" / (std::filesystem::path(texture_name).stem().string() + "_" + detailcube_faces.at(face));
        texture_path.replace_extension(".png");
        logger::debug("Writing image of size ({}, {}) to {}", extent.x, extent.y, texture_path.lexically_relative(output_directory).string());
        if(write_texture(std::span<uint32_t>(face_texture.data<uint32_t>(), face_texture.size<uint32_t>()), texture_path, extent)){
            logger::info("   Saved face {} to {}", detailcube_faces.at(face), texture_path.lexically_relative(output_directory).string());
        }
    }
}

void save_texture(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::info("Saving {} as png...", texture_name);
    gli::texture2d texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
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

void process_images(
    const synthium::Manager& manager, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>>& queue, 
    std::filesystem::path output_directory
) {
    logger::debug("Got output directory {}", output_directory.string());
    while(!queue.is_closed()) {
        auto texture_info = queue.try_dequeue({"", Parameter::Semantic::UNKNOWN});
        std::string texture_name = texture_info.first, albedo_name;
        size_t index;
        Parameter::Semantic semantic = texture_info.second;
        if(semantic == Parameter::Semantic::UNKNOWN) {
            logger::info("Got default value from try_dequeue, stopping thread.");
            break;
        }

        switch (semantic)
        {
        case Parameter::Semantic::BASE_COLOR:
        case Parameter::Semantic::BASE_CAMO:
        case Parameter::Semantic::DETAIL_SELECT:
        case Parameter::Semantic::OVERLAY0:
        case Parameter::Semantic::OVERLAY1:
            save_texture(texture_name, manager.get(texture_name).get_data(), output_directory);
            break;
        case Parameter::Semantic::NORMAL_MAP:
            process_normalmap(texture_name, manager.get(texture_name).get_data(), output_directory);
            break;
        case Parameter::Semantic::SPECULAR:
            albedo_name = texture_name;
            index = albedo_name.find_last_of('_');
            albedo_name[index + 1] = 'C';
            process_specular(texture_name, manager.get(texture_name).get_data(), manager.get(albedo_name).get_data(), output_directory);
            break;
        case Parameter::Semantic::DETAIL_CUBE:
            process_detailcube(texture_name, manager.get(texture_name).get_data(), output_directory);
            break;
        default:
            logger::warn("Skipping unimplemented semantic: {}", texture_name);
            break;
        }
    }
}

int add_texture_to_gltf(gltf2::Model &gltf, std::filesystem::path texture_path, std::filesystem::path output_filename, std::optional<std::string> label = {}) {
    int index = (int)gltf.textures.size();
    gltf2::Texture tex;
    tex.source = (int)gltf.images.size();
    tex.name = label ? *label : texture_path.filename().string();
    tex.sampler = 0;
    gltf2::Image img;
    img.uri = texture_path.lexically_relative(output_filename.parent_path()).string();
    
    gltf.textures.push_back(tex);
    gltf.images.push_back(img);
    return index;
}

std::optional<gltf2::TextureInfo> load_texture_info(
    gltf2::Model &gltf,
    const DME &dme, 
    uint32_t i, 
    std::unordered_map<uint32_t, uint32_t> &texture_indices, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
    std::filesystem::path output_filename,
    Parameter::Semantic semantic
) {
    std::optional<std::string> texture_name = dme.dmat()->material(i)->texture(semantic);
    if(!texture_name) {
        return {};
    }
    gltf2::TextureInfo info;
    std::string original_name;
    uint32_t hash = jenkins::oaat(*texture_name);
    std::unordered_map<uint32_t, uint32_t>::iterator value;
    if((value = texture_indices.find(hash)) == texture_indices.end()) {
        image_queue.enqueue({*texture_name, semantic});
        
        std::filesystem::path texture_path(*texture_name);
        texture_path.replace_extension(".png");
        texture_path = output_filename.parent_path() / "textures" / texture_path;
        
        texture_indices[hash] = (uint32_t)gltf.textures.size();
        info.index = add_texture_to_gltf(gltf, texture_path, output_filename);
    } else {
        info.index = value->second;
    }
    return info;
}


std::optional<std::pair<gltf2::TextureInfo, gltf2::TextureInfo>> load_specular_info(
    gltf2::Model &gltf,
    const DME &dme, 
    uint32_t i, 
    std::unordered_map<uint32_t, uint32_t> &texture_indices, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
    std::filesystem::path output_filename,
    Parameter::Semantic semantic
) {
    gltf2::TextureInfo metallic_roughness_info, emissive_info;
    std::optional<std::string> texture_name = dme.dmat()->material(i)->texture(semantic);
    if(!texture_name) {
        return {};
    }
    uint32_t hash = jenkins::oaat(*texture_name);
    std::unordered_map<uint32_t, uint32_t>::iterator value;
    if((value = texture_indices.find(hash)) == texture_indices.end()) {
        image_queue.enqueue({*texture_name, semantic});
        std::string metallic_roughness_name = utils::relabel_texture(*texture_name, "MR");

        std::filesystem::path metallic_roughness_path(metallic_roughness_name);
        metallic_roughness_path.replace_extension(".png");
        metallic_roughness_path = output_filename.parent_path() / "textures" / metallic_roughness_path;
        
        texture_indices[hash] = (uint32_t)gltf.textures.size();
        metallic_roughness_info.index = add_texture_to_gltf(gltf, metallic_roughness_path, output_filename);
        
        std::string emissive_name = utils::relabel_texture(*texture_name, "E");
        std::filesystem::path emissive_path = metallic_roughness_path.parent_path() / emissive_name;
        emissive_path.replace_extension(".png");

        hash = jenkins::oaat(emissive_name);
        texture_indices[hash] = (uint32_t)gltf.textures.size();
        emissive_info.index = add_texture_to_gltf(gltf, emissive_path, output_filename);
    } else {
        metallic_roughness_info.index = value->second;
        std::string emissive_name = utils::relabel_texture(*texture_name, "E");
        hash = jenkins::oaat(emissive_name);
        emissive_info.index = texture_indices.at(hash);
    }
    return std::make_pair(metallic_roughness_info, emissive_info);
}

std::vector<uint8_t> expand_vertex_stream(json &layout, std::span<uint8_t> data, uint32_t stream, bool is_rigid, const DME &dme) {
    VertexStream vertices(data);
    logger::debug("{}['{}']", layout.at("sizes").dump(), std::to_string(stream));
    uint32_t stride = layout.at("sizes")
                            .at(std::to_string(stream))
                            .get<uint32_t>();
    logger::info("Data stride: {}", stride);
    std::vector<std::pair<uint32_t, bool>> offsets;
    bool conversion_required = false;
    int tangent_index = -1;
    int binormal_index = -1;
    int blend_indices_index = -1;
    int vert_index_offset = 0;
    bool has_normals = false, bone_remapping = false;

    for(int i = 0; i < layout.at("entries").size(); i++) {
        json &entry = layout.at("entries").at(i);
        if(entry.at("stream").get<uint32_t>() != stream) {
            logger::debug("Skipping entry...");
            if(entry.at("stream").get<uint32_t>() < stream)
                vert_index_offset++;
            continue;
        }
        logger::debug("{}", entry.dump());
        std::string type = entry.at("type").get<std::string>();
        std::string usage = entry.at("usage").get<std::string>();
        bool needs_conversion = type == "Float16_2" || type == "float16_2";
        offsets.push_back({
            sizes.at(type), 
            needs_conversion
        });
        if(needs_conversion) {
            conversion_required = true;
            entry.at("type") = "Float2";
            layout.at("sizes").at(std::to_string(stream)) = layout.at("sizes").at(std::to_string(stream)).get<uint32_t>() + 4;
        }
        
        if(usage == "Normal") {
            has_normals = true;
        } else if(usage == "Binormal") {
            binormal_index = i;
        } else if(usage == "Tangent") {
            tangent_index = i;
        } else if (usage == "BlendIndices") {
            bone_remapping = true;
            blend_indices_index = i;
        }
    }
    
    std::string binormal_type, tangent_type;
    if(binormal_index != -1)
        binormal_type = layout.at("entries").at(binormal_index).at("type");
    if(tangent_index != -1)
        tangent_type = layout.at("entries").at(tangent_index).at("type");
    
    
    bool calculate_normals = !has_normals && binormal_index != -1 && tangent_index != -1;
    bool add_rigid_bones = is_rigid && binormal_type == "ubyte4n";

    if(!conversion_required && !calculate_normals && !add_rigid_bones && !bone_remapping) {
        logger::info("No conversion required!");
        return std::vector<uint8_t>(vertices.buf_.begin(), vertices.buf_.end());
    }
    
    if(calculate_normals) {
        logger::info("Calculating normals from tangents and binormals");
        layout.at("sizes").at(std::to_string(stream)) = layout.at("sizes").at(std::to_string(stream)).get<uint32_t>() + 12;
        layout.at("entries") += json::parse("{\"stream\":"+std::to_string(stream)+",\"type\":\"Float3\",\"usage\":\"Normal\",\"usageIndex\":0}");
    }

    if(add_rigid_bones) {
        logger::info("Adding rigid bone weights");
        layout.at("sizes").at(std::to_string(stream)) = layout.at("sizes").at(std::to_string(stream)).get<uint32_t>() + 20;
        layout.at("entries") += json::parse("{\"stream\":"+std::to_string(stream)+",\"type\":\"D3dcolor\",\"usage\":\"BlendIndices\",\"usageIndex\":0}");
        layout.at("entries") += json::parse("{\"stream\":"+std::to_string(stream)+",\"type\":\"Float4\",\"usage\":\"BlendWeight\",\"usageIndex\":0}");
    }

    logger::info("Converting {} entries", std::count_if(offsets.begin(), offsets.end(), [](auto pair) { return pair.second; }));
    std::vector<uint8_t> output;
    for(uint32_t vertex_offset = 0; vertex_offset < vertices.size(); vertex_offset += stride) {
        uint32_t entry_offset = 0;
        float binormal[3] = {0, 0, 0};
        float tangent[3] = {0, 0, 0};
        float sign = 0;
        float normal[3] = {0, 0, 0};
        uint16_t rigid_joint_index = 0;

        float converter[2] = {0, 0};
        for(auto iter = offsets.begin(); iter != offsets.end(); iter++) {
            int index = (int)(iter - offsets.begin());
            if(iter->second) {
                converter[0] = (float)(half)vertices.get<half>(vertex_offset + entry_offset);
                converter[1] = (float)(half)vertices.get<half>(vertex_offset + entry_offset + 2);
                output.insert(output.end(), reinterpret_cast<uint8_t*>(converter), reinterpret_cast<uint8_t*>(converter) + 8);
            } else {
                std::span<uint8_t> to_add = vertices.buf_.subspan(vertex_offset + entry_offset, iter->first);
                if(index == blend_indices_index - vert_index_offset) {
                    for(uint32_t bone_index = 0; bone_index < to_add.size(); bone_index++) {
                        to_add[bone_index] = (uint8_t)dme.map_bone(to_add[bone_index]);
                    }
                }
                output.insert(output.end(), to_add.begin(), to_add.end());
            }

            if(index == binormal_index - vert_index_offset) {
                if(calculate_normals) {
                    utils::load_vector(binormal_type, vertex_offset, entry_offset, vertices, binormal);
                }
                if(is_rigid && binormal_type == "ubyte4n") {
                    rigid_joint_index = dme.map_bone(vertices.get<uint8_t>(vertex_offset + entry_offset + 3));
                }
            }

            if(calculate_normals && index == tangent_index - vert_index_offset) {
                utils::load_vector(tangent_type, vertex_offset, entry_offset, vertices, tangent);
                if(tangent_type == "ubyte4n") {
                    sign = vertices.get<uint8_t>(vertex_offset + entry_offset + 3) / 255.0f * 2 - 1;
                } else {
                    sign = -1;
                }
            }

            entry_offset += iter->first;
        }

        if(calculate_normals) {
            sign /= std::fabsf(sign);
            utils::normalize(binormal);
            utils::normalize(tangent);
            logger::debug("Tangent {}:  ({: 0.2f} {: 0.2f} {: 0.2f})", tangent_index, tangent[0], tangent[1], tangent[2]);
            logger::debug("Binormal {}: ({: 0.2f} {: 0.2f} {: 0.2f})", binormal_index, binormal[0], binormal[1], binormal[2]);
            normal[0] = binormal[1] * tangent[2] - binormal[2] * tangent[1];
            normal[1] = binormal[2] * tangent[0] - binormal[0] * tangent[2];
            normal[2] = binormal[0] * tangent[1] - binormal[1] * tangent[0];
            utils::normalize(normal);
            normal[0] *= sign;
            normal[1] *= sign;
            normal[2] *= sign;
            logger::debug("Normal:     ({: 0.2f} {: 0.2f} {: 0.2f})", normal[0], normal[1], normal[2], sign);
            logger::debug("Entry offset/stride: {} / {}", entry_offset, stride);
            output.insert(output.end(), reinterpret_cast<uint8_t*>(normal), reinterpret_cast<uint8_t*>(normal) + 12);
        }

        if(add_rigid_bones) {
            uint8_t blend_indices[4] = {(uint8_t)rigid_joint_index, 0, 0, 0};
            float blend_weights[4] = {1, 0, 0, 0};
            output.insert(output.end(), blend_indices, blend_indices + 4);
            output.insert(output.end(), reinterpret_cast<uint8_t*>(blend_weights), reinterpret_cast<uint8_t*>(blend_weights) + 16);
        }
    }
    return output;
}

int add_mesh_to_gltf(gltf2::Model &gltf, const DME &dme, uint32_t index) {
    int texcoord = 0;
    int color = 0;
    gltf2::Mesh gltf_mesh;
    gltf2::Primitive primitive;
    std::shared_ptr<const Mesh> mesh = dme.mesh(index);
    std::vector<uint32_t> offsets((std::size_t)mesh->vertex_stream_count(), 0);
    json input_layout = get_input_layout(dme.dmat()->material(index)->definition());
    std::string layout_name = input_layout.at("name").get<std::string>();
    logger::info("Using input layout {}", layout_name);
    bool rigid = utils::uppercase(layout_name).find("RIGID") != std::string::npos;
    
    std::vector<gltf2::Buffer> buffers;
    for(uint32_t j = 0; j < mesh->vertex_stream_count(); j++) {
        std::span<uint8_t> vertex_stream = mesh->vertex_stream(j);
        gltf2::Buffer buffer;
        logger::info("Expanding vertex stream {}", j);
        buffer.data = expand_vertex_stream(input_layout, vertex_stream, j, rigid, dme);
        buffers.push_back(buffer);
    }

    for(json entry : input_layout.at("entries")) {
        std::string type = entry.at("type").get<std::string>();
        std::string usage = entry.at("usage").get<std::string>();
        int stream = entry.at("stream").get<int>();
        if(usage == "Binormal") {
            offsets.at(stream) += sizes.at(type);
            continue;
        }
        logger::info("Adding accessor for {} {} data", type, usage);
        gltf2::Accessor accessor;
        accessor.bufferView = (int)gltf.bufferViews.size();
        accessor.byteOffset = 0;
        accessor.componentType = component_types.at(type);
        accessor.type = types.at(type);
        accessor.count = mesh->vertex_count();

        gltf2::BufferView bufferview;
        bufferview.buffer = (int)gltf.buffers.size() + stream;
        bufferview.byteLength = buffers.at(stream).data.size() - offsets.at(stream);
        bufferview.byteStride = input_layout.at("sizes").at(std::to_string(stream)).get<uint32_t>();
        bufferview.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        bufferview.byteOffset = offsets.at(stream);
        std::string attribute = usages.at(usage);
        if(usage == "Texcoord") {
            attribute += std::to_string(texcoord);
            texcoord++;
        } else if (usage == "Color") {
            attribute += std::to_string(color);
            color++;
            accessor.normalized = true;
        } else if(usage == "Position") {
            AABB aabb = dme.aabb();
            accessor.minValues = {aabb.min.x, aabb.min.y, aabb.min.z};
            accessor.maxValues = {aabb.max.x, aabb.max.y, aabb.max.z};
        } else if(usage == "Tangent") {
            accessor.normalized = true;
        }
        primitive.attributes[attribute] = (int)gltf.accessors.size();
        gltf.accessors.push_back(accessor);
        gltf.bufferViews.push_back(bufferview);

        offsets.at(stream) += sizes.at(type);
    }

    gltf.buffers.insert(gltf.buffers.end(), buffers.begin(), buffers.end());

    std::span<uint8_t> indices = mesh->index_data();
    gltf2::Accessor accessor;
    accessor.bufferView = (int)gltf.bufferViews.size();
    accessor.byteOffset = 0;
    accessor.componentType = mesh->index_size() == 2 ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT : TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    accessor.type = TINYGLTF_TYPE_SCALAR;
    accessor.count = mesh->index_count();

    gltf2::BufferView bufferview;
    bufferview.buffer = (int)gltf.buffers.size();
    bufferview.byteLength = indices.size();
    bufferview.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    bufferview.byteOffset = 0;

    gltf2::Buffer buffer;
    buffer.data = std::vector<uint8_t>(indices.begin(), indices.end());

    primitive.indices = (int)gltf.accessors.size();
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    primitive.material = index;
    gltf_mesh.primitives.push_back(primitive);

    gltf.accessors.push_back(accessor);
    gltf.bufferViews.push_back(bufferview);
    gltf.buffers.push_back(buffer);

    gltf.scenes.at(gltf.defaultScene).nodes.push_back((int)gltf.nodes.size());

    int node_index = (int)gltf.nodes.size();

    gltf2::Node node;
    node.mesh = (int)gltf.meshes.size();
    gltf.nodes.push_back(node);
    
    gltf.meshes.push_back(gltf_mesh);

    return node_index;
}

void update_transforms(gltf2::Model &gltf, int root, glm::vec4 global_offset = glm::vec4(0, 0, 0, 1), glm::quat global_rotation = glm::quat(0, 0, 0, 1)) {
    for(int child : gltf.nodes.at(root).children) {
        glm::vec4 translation_vec(gltf.nodes.at(root).translation[0], gltf.nodes.at(root).translation[1], gltf.nodes.at(root).translation[2], 1);
        glm::quat rotation(
            gltf.nodes.at(root).rotation[0], 
            gltf.nodes.at(root).rotation[1], 
            gltf.nodes.at(root).rotation[2], 
            gltf.nodes.at(root).rotation[3]
        );
        update_transforms(gltf, child, global_offset + rotation * translation_vec, global_rotation * rotation);
        std::vector<double> translation = gltf.nodes.at(child).translation;
        translation_vec = glm::inverse(global_rotation) * global_offset;
        gltf.nodes.at(child).translation = {
            translation[0] - translation_vec.x, 
            translation[1] - translation_vec.y, 
            translation[2] - translation_vec.z
        };
        glm::quat child_rotation(
            gltf.nodes.at(child).rotation[0], 
            gltf.nodes.at(child).rotation[1], 
            gltf.nodes.at(child).rotation[2], 
            gltf.nodes.at(child).rotation[3]
        );

        child_rotation = child_rotation * glm::inverse(global_rotation);
        gltf.nodes.at(child).rotation = {child_rotation.x, child_rotation.y, child_rotation.z, child_rotation.w};
        // glm::vec4 translation_vec(translation[0], translation[1], translation[2], 1);
        // glm::quat rotation(
        //     gltf.nodes.at(root).rotation[0], 
        //     gltf.nodes.at(root).rotation[1], 
        //     gltf.nodes.at(root).rotation[2], 
        //     gltf.nodes.at(root).rotation[3]
        // );
        // translation_vec = rotation * translation_vec;
        // gltf.nodes.at(child).translation = {translation_vec.x - gltf.nodes.at(root).translation[0], translation_vec.y - gltf.nodes.at(root).translation[1], translation_vec.z - gltf.nodes.at(root).translation[2]};
        // gltf.nodes.at(child).rotation = {0, 0, 0, 1};
    }

}

int main(int argc, const char* argv[]) {
    argparse::ArgumentParser parser("dme_converter", CPPDMOD_VERSION);
    parser.add_description("C++ DMEv4 to GLTF2 model conversion tool");
    parser.add_argument("input_file");
    parser.add_argument("output_file");
    parser.add_argument("--format", "-f")
        .help("Select the format in which to save the file {glb, gltf}")
        .required()
        .action([](const std::string& value) {
            static const std::vector<std::string> choices = { "gltf", "glb" };
            if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
                return value;
            }
            return std::string{ "" };
        });
    int log_level = logger::level::warn;
    parser.add_argument("--verbose", "-v")
        .help("Increase log level. May be specified multiple times")
        .action([&](const auto &){ 
            if(log_level > 0) {
                log_level--; 
            }
        })
        .append()
        .nargs(0)
        .default_value(false)
        .implicit_value(true);
    
    parser.add_argument("--threads", "-t")
        .help("The number of threads to use for image processing")
        .default_value(4u)
        .scan<'u', uint32_t>();

    parser.add_argument("--no-skeleton", "-s")
        .help("Exclude the skeleton from the output")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);
    
    parser.add_argument("--no-textures", "-i")
        .help("Exclude the skeleton from the output")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);
    
    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
    std::string input_str = parser.get<std::string>("input_file");
    logger::info("Converting file {} using dme_converter {}", input_str, CPPDMOD_VERSION);
    uint32_t image_processor_thread_count = parser.get<uint32_t>("--threads");
    logger::set_level(logger::level::level_enum(log_level));
    std::filesystem::path server = "C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/";
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 24; i++) {
        assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
    }
    logger::info("Loading packs...");
    synthium::Manager manager(assets);
    logger::info("Manager loaded.");
    init_materials();

    std::filesystem::path input_filename(input_str);
    std::unique_ptr<uint8_t[]> data;
    std::vector<uint8_t> data_vector;
    std::span<uint8_t> data_span;
    if(manager.contains(input_str)) {
        data_vector = manager.get(input_str).get_data();
        data_span = std::span<uint8_t>(data_vector.data(), data_vector.size());
    } else {
        std::ifstream input(input_filename, std::ios::binary | std::ios::ate);
        if(input.fail()) {
            logger::error("Failed to open file '{}'", input_filename.string());
            std::exit(2);
        }
        size_t length = input.tellg();
        input.seekg(0);
        data = std::make_unique<uint8_t[]>(length);
        input.read((char*)data.get(), length);
        input.close();
        data_span = std::span<uint8_t>(data.get(), length);
    }
    
    std::filesystem::path output_filename(parser.get<std::string>("output_file"));
    output_filename = std::filesystem::weakly_canonical(output_filename);
    std::filesystem::path output_directory;
    if(output_filename.has_parent_path()) {
        output_directory = output_filename.parent_path();
    }

    try {
        if(!std::filesystem::exists(output_filename.parent_path())) {
            std::filesystem::create_directories(output_directory / "textures");
        }
    } catch (std::filesystem::filesystem_error& err) {
        logger::error("Failed to create directory {}: {}", err.path1().string(), err.what());
        std::exit(3);
    }

    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> image_queue;

    bool export_textures = !parser.get<bool>("--no-textures");
    std::vector<std::thread> image_processor_pool;
    if(export_textures) {
        logger::info("Using {} image processing thread{}", image_processor_thread_count, image_processor_thread_count == 1 ? "" : "s");
        for(uint32_t i = 0; i < image_processor_thread_count; i++) {
            image_processor_pool.push_back(std::thread{
                process_images, 
                std::cref(manager), 
                std::ref(image_queue), 
                output_directory
            });
        }
    } else {
        logger::info("Not exporting textures by user request.");
    }

    DME dme(data_span);
    gltf2::Model gltf;
    gltf2::Sampler sampler;
    sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
    sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
    gltf.samplers.push_back(sampler);
    
    gltf.defaultScene = (int)gltf.scenes.size();
    gltf.scenes.push_back({});

    std::vector<Parameter::Semantic> semantics = {
        Parameter::Semantic::BASE_COLOR,
        Parameter::Semantic::NORMAL_MAP,
        Parameter::Semantic::SPECULAR,
        Parameter::Semantic::DETAIL_SELECT,
        Parameter::Semantic::DETAIL_CUBE,
        Parameter::Semantic::OVERLAY0,
        Parameter::Semantic::OVERLAY1,
        Parameter::Semantic::BASE_CAMO,
    };

    std::unordered_map<uint32_t, uint32_t> texture_indices;
    std::vector<int> mesh_nodes;

    for(uint32_t i = 0; i < dme.mesh_count(); i++) {
        gltf2::Material material;
        if(export_textures) {
            std::optional<gltf2::TextureInfo> info;
            std::optional<std::pair<gltf2::TextureInfo, gltf2::TextureInfo>> info_pair;
            std::optional<std::string> texture_name, label = {};
            std::filesystem::path temp;
            for(Parameter::Semantic semantic : semantics) {
                switch(semantic) {
                case Parameter::Semantic::NORMAL_MAP:
                    info = load_texture_info(gltf, dme, i, texture_indices, image_queue, output_filename, semantic);
                    if(!info)
                        break;
                    material.normalTexture.index = info->index;
                    break;
                case Parameter::Semantic::BASE_COLOR:
                    info = load_texture_info(gltf, dme, i, texture_indices, image_queue, output_filename, semantic);
                    if(!info)
                        break;
                    material.pbrMetallicRoughness.baseColorTexture = *info;
                    break;
                case Parameter::Semantic::SPECULAR:
                    info_pair = load_specular_info(
                        gltf, dme, i, texture_indices, image_queue, output_filename, semantic
                    );
                    if(!info_pair)
                        break;
                    material.pbrMetallicRoughness.metallicRoughnessTexture = info_pair->first;
                    material.emissiveTexture = info_pair->second;
                    material.emissiveFactor = {1.0, 1.0, 1.0};
                    break;
                default:
                    // Just export the texture
                    label = Parameter::semantic_name(semantic);
                    texture_name = dme.dmat()->material(i)->texture(semantic);
                    if(texture_name) {
                        image_queue.enqueue({*texture_name, semantic});
                        if (semantic != Parameter::Semantic::DETAIL_CUBE) {
                            add_texture_to_gltf(gltf, (output_directory / "textures" / *texture_name).replace_extension(".png"), output_filename, label);
                        } else {
                            temp = std::filesystem::path(*texture_name);
                            for(std::string face : detailcube_faces) {
                                add_texture_to_gltf(
                                    gltf, 
                                    (output_directory / "textures" / (temp.stem().string() + "_" + face)).replace_extension(".png"),
                                    output_filename,
                                    *label + " " + face
                                );
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            material.pbrMetallicRoughness.baseColorFactor = { 0.133, 0.545, 0.133 }; // Forest Green
        }
        material.doubleSided = true;
        gltf.materials.push_back(material);

        int node_index = add_mesh_to_gltf(gltf, dme, i);
        mesh_nodes.push_back(node_index);
        
        logger::info("Added mesh {} to gltf", i);
    }

    bool include_skeleton = !parser.get<bool>("--no-skeleton");
    if(dme.bone_count() > 0 && include_skeleton) {
        for(int node_index : mesh_nodes) {
            gltf.nodes.at(node_index).skin = (int)gltf.skins.size();
        }

        gltf2::Buffer bone_buffer;
        gltf2::Skin skin;
        skin.inverseBindMatrices = (int)gltf.accessors.size();

        std::unordered_map<uint32_t, size_t> skeleton_map;
        for(uint32_t bone_index = 0; bone_index < dme.bone_count(); bone_index++) {
            gltf2::Node bone_node;
            Bone bone = dme.bone(bone_index);
            PackedMat4 packed_inv = bone.inverse_bind_matrix;
            uint32_t namehash = bone.namehash;
            glm::mat4 inverse_bind_matrix(glm::mat4x3(
                packed_inv[0][0], packed_inv[0][1], packed_inv[0][2],
                packed_inv[1][0], packed_inv[1][1], packed_inv[1][2],
                packed_inv[2][0], packed_inv[2][1], packed_inv[2][2],
                packed_inv[3][0], packed_inv[3][1], packed_inv[3][2]
            ));
            glm::mat4 bind_matrix = glm::inverse(inverse_bind_matrix);
            glm::mat4 rotated = glm::rotate(inverse_bind_matrix, (float)M_PI, glm::vec3(0, 1, 0));
            std::span<float> inverse_matrix_data(&inverse_bind_matrix[0].x, 16);
            std::span<float> matrix_data(&bind_matrix[0].x, 16);
            //bone_node.matrix = std::vector<double>(matrix_data.begin(), matrix_data.end());
            bone_node.translation = {bind_matrix[3].x, bind_matrix[3].y, bind_matrix[3].z};
            bind_matrix[3].x = 0;
            bind_matrix[3].y = 0;
            bind_matrix[3].z = 0;
            //bone_node.scale = {glm::length(bind_matrix[0]), glm::length(bind_matrix[1]), glm::length(bind_matrix[2])};
            bind_matrix[0] /= glm::length(bind_matrix[0]);
            bind_matrix[1] /= glm::length(bind_matrix[1]);
            bind_matrix[2] /= glm::length(bind_matrix[2]);
            glm::quat quat(bind_matrix);
            bone_node.rotation = {quat.x, quat.y, quat.z, quat.w};
            bone_node.name = utils::bone_hashmap.at(bone.namehash);
            skeleton_map[bone.namehash] = gltf.nodes.size();
            skin.joints.push_back((int)gltf.nodes.size());

            bone_buffer.data.insert(
                bone_buffer.data.end(), 
                reinterpret_cast<uint8_t*>(inverse_matrix_data.data()), 
                reinterpret_cast<uint8_t*>(inverse_matrix_data.data()) + inverse_matrix_data.size_bytes()
            );

            gltf.nodes.push_back(bone_node);
        }

        for(auto iter = skeleton_map.begin(); iter != skeleton_map.end(); iter++) {
            uint32_t curr_hash = iter->first;
            size_t curr_index = iter->second;
            uint32_t parent = utils::bone_hierarchy.at(curr_hash);
            std::unordered_map<uint32_t, size_t>::iterator parent_index;
            while(parent != 0 && (parent_index = skeleton_map.find(parent)) == skeleton_map.end()) {
                parent = utils::bone_hierarchy.at(parent);
            }
            if(parent != 0) {
                // Bone has parent in the skeleton, add it to its parent's children
                gltf.nodes.at(parent_index->second).children.push_back((int)curr_index);
            } else if(parent == 0) {
                // Bone is the root of the skeleton, add it as the skin's skeleton
                skin.skeleton = (int)curr_index;
                gltf.scenes.at(gltf.defaultScene).nodes.push_back((int)curr_index);
            }
        }

        update_transforms(gltf, skin.skeleton);

        gltf2::Accessor accessor;
        accessor.bufferView = (int)gltf.bufferViews.size();
        accessor.byteOffset = 0;
        accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        accessor.type = TINYGLTF_TYPE_MAT4;
        accessor.count = dme.bone_count();

        gltf2::BufferView bufferview;
        bufferview.buffer = (int)gltf.buffers.size();
        bufferview.byteLength = bone_buffer.data.size();
        bufferview.byteOffset = 0;
        
        gltf.accessors.push_back(accessor);
        gltf.buffers.push_back(bone_buffer);
        gltf.bufferViews.push_back(bufferview);
        gltf.skins.push_back(skin);
    }
    
    gltf.asset.version = "2.0";
    gltf.asset.generator = "DME Converter (C++) " + std::string(CPPDMOD_VERSION) + " via tinygltf";

    std::string format = parser.get<std::string>("--format");

    logger::info("Writing GLTF2 file {}...", output_filename.filename().string());
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&gltf, output_filename.string(), false, format == "glb", format == "gltf", format == "glb");
    
    image_queue.close();
    logger::info("Joining image processing thread{}...", image_processor_pool.size() == 1 ? "" : "s");
    for(uint32_t i = 0; i < image_processor_pool.size(); i++) {
        image_processor_pool.at(i).join();
    }
    logger::info("Done.");
    return 0;
}