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

#define THREAD_COUNT 3

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
        logger::info("Compressed texture (format {})", (int)texture.format());
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
    logger::info("Writing image of size ({}, {})", extent.x, extent.y);
    if(write_texture(std::span<uint32_t>(unpacked_normal.get(), pixels.size()), normal_path, extent)) {
        logger::info("Saved normal map to {}", normal_path.string());
    }

    std::string tint_name = utils::relabel_texture(texture_name, "T");
    std::filesystem::path tint_path = normal_path.parent_path() / tint_name;
    tint_path.replace_extension(".png");
    if(write_texture(std::span<uint32_t>(tint_map.get(), pixels.size()), tint_path, extent)) {
        logger::info("Saved tint map to {}", tint_path.string());
    }
}

void process_specular(std::string texture_name, std::vector<uint8_t> specular_data, std::vector<uint8_t> albedo_data, std::filesystem::path output_directory) {
    logger::info("Processing specular...");
    gli::texture2d specular(gli::load_dds((char*)specular_data.data(), specular_data.size()));
    if(specular.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    if(gli::is_compressed(specular.format())) {
        logger::info("Compressed texture (format {})", (int)specular.format());
        specular = gli::convert(specular, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    gli::texture2d albedo(gli::load_dds((char*)albedo_data.data(), albedo_data.size()));
    if(albedo.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load albedo from memory");
    }
    if(gli::is_compressed(albedo.format())) {
        logger::info("Compressed texture (format {})", (int)albedo.format());
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
    logger::info("Writing image of size ({}, {})", extent.x, extent.y);
    if(write_texture(std::span<uint32_t>(metallic_roughness.get(), albedo_pixels.size()), metallic_roughness_path, extent)){
        logger::info("Saved metallic roughness map to {}", metallic_roughness_path.string());
    }

    std::string emissive_name = utils::relabel_texture(texture_name, "E");
    std::filesystem::path emissive_path = metallic_roughness_path.parent_path() / emissive_name;
    emissive_path.replace_extension(".png");
    if(write_texture(std::span<uint32_t>(emissive.get(), albedo_pixels.size()), emissive_path, extent)) {
        logger::info("Saved emissive map to {}", emissive_path.string());
    }
}

void save_base_color(std::string texture_name, std::vector<uint8_t> texture_data, std::filesystem::path output_directory) {
    logger::info("Saving {} as png...", texture_name);
    gli::texture2d texture(gli::load_dds((char*)texture_data.data(), texture_data.size()));
    if(texture.format() == gli::format::FORMAT_UNDEFINED) {
        logger::error("Failed to load {} from memory", texture_name);
    }
    if(gli::is_compressed(texture.format())) {
        logger::info("Compressed texture (format {})", (int)texture.format());
        texture = gli::convert(texture, gli::format::FORMAT_RGBA8_UNORM_PACK8);
    }
    std::filesystem::path texture_path(texture_name);
    texture_path.replace_extension(".png");
    texture_path = output_directory / "textures" / texture_path;
    auto extent = texture.extent();
    logger::info("Writing image of size ({}, {})", extent.x, extent.y);
    if(write_texture(std::span<uint32_t>(texture.data<uint32_t>(), texture.size<uint32_t>()), texture_path, extent)){
        logger::info("Saved base color to {}", texture_path.string());
    }
}

void process_images(
    const synthium::Manager& manager, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>>& queue, 
    std::filesystem::path output_directory
) {
    logger::info("Got output directory {}", output_directory.string());
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
            save_base_color(texture_name, manager.get(texture_name).get_data(), output_directory);
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
        default:
            logger::warn("Skipping unimplemented semantic: {}", texture_name);
            break;
        }
    }
}

int add_texture_to_gltf(gltf2::Model &gltf, std::filesystem::path texture_path, std::filesystem::path output_filename) {
    int index = (int)gltf.textures.size();
    gltf2::Texture tex;
    tex.source = (int)gltf.images.size();
    tex.name = texture_path.filename().string();
    tex.sampler = 0;
    gltf2::Image img;
    img.uri = texture_path.lexically_relative(output_filename.parent_path()).string();
    
    gltf.textures.push_back(tex);
    gltf.images.push_back(img);
    return index;
}

gltf2::TextureInfo load_texture_info(
    gltf2::Model &gltf,
    const DME &dme, 
    uint32_t i, 
    std::unordered_map<uint32_t, uint32_t> &texture_indices, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
    std::filesystem::path output_filename,
    Parameter::Semantic semantic
) {
    gltf2::TextureInfo info;
    std::string texture_name = dme.dmat()->material(i)->texture(semantic), original_name;
    uint32_t hash = jenkins::oaat(utils::uppercase(texture_name));
    std::unordered_map<uint32_t, uint32_t>::iterator value;
    if((value = texture_indices.find(hash)) == texture_indices.end()) {
        image_queue.enqueue({texture_name, semantic});
        
        std::filesystem::path texture_path(texture_name);
        texture_path.replace_extension(".png");
        texture_path = output_filename.parent_path() / "textures" / texture_path;
        
        texture_indices[hash] = (uint32_t)gltf.textures.size();
        info.index = add_texture_to_gltf(gltf, texture_path, output_filename);
    } else {
        info.index = value->second;
    }
    return info;
}


std::pair<gltf2::TextureInfo, gltf2::TextureInfo> load_specular_info(
    gltf2::Model &gltf,
    const DME &dme, 
    uint32_t i, 
    std::unordered_map<uint32_t, uint32_t> &texture_indices, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> &image_queue,
    std::filesystem::path output_filename,
    Parameter::Semantic semantic
) {
    gltf2::TextureInfo metallic_roughness_info, emissive_info;
    std::string texture_name = dme.dmat()->material(i)->texture(semantic);
    uint32_t hash = jenkins::oaat(utils::uppercase(texture_name));
    std::unordered_map<uint32_t, uint32_t>::iterator value;
    if((value = texture_indices.find(hash)) == texture_indices.end()) {
        image_queue.enqueue({texture_name, semantic});
        std::string metallic_roughness_name = utils::relabel_texture(texture_name, "MR");

        std::filesystem::path metallic_roughness_path(metallic_roughness_name);
        metallic_roughness_path.replace_extension(".png");
        metallic_roughness_path = output_filename.parent_path() / "textures" / metallic_roughness_path;
        
        texture_indices[hash] = (uint32_t)gltf.textures.size();
        metallic_roughness_info.index = add_texture_to_gltf(gltf, metallic_roughness_path, output_filename);
        
        std::string emissive_name = utils::relabel_texture(texture_name, "E");
        std::filesystem::path emissive_path = metallic_roughness_path.parent_path() / emissive_name;
        emissive_path.replace_extension(".png");

        hash = jenkins::oaat(emissive_name);
        texture_indices[hash] = (uint32_t)gltf.textures.size();
        emissive_info.index = add_texture_to_gltf(gltf, emissive_path, output_filename);
    } else {
        metallic_roughness_info.index = value->second;
        std::string emissive_name = utils::relabel_texture(texture_name, "E");
        hash = jenkins::oaat(emissive_name);
        emissive_info.index = texture_indices.at(hash);
    }
    return std::make_pair(metallic_roughness_info, emissive_info);
}

std::vector<uint8_t> expand_halves(json &layout, std::span<uint8_t> data, uint32_t stream) {
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
    int vert_index_offset = 0;
    bool has_normals = false;

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
        }
    }

    bool calculate_normals = !has_normals && binormal_index != -1 && tangent_index != -1;

    if(!conversion_required && !calculate_normals) {
        logger::info("No conversion required!");
        return std::vector<uint8_t>(vertices.buf_.begin(), vertices.buf_.end());
    } else if(calculate_normals) {
        logger::info("Calculating normals from tangents and binormals");
        layout.at("sizes").at(std::to_string(stream)) = layout.at("sizes").at(std::to_string(stream)).get<uint32_t>() + 12;
        layout.at("entries") += json::parse("{\"stream\":"+std::to_string(stream)+",\"type\":\"Float3\",\"usage\":\"Normal\",\"usageIndex\":0}");
    }

    std::string binormal_type, tangent_type;
    if(binormal_index != -1)
        binormal_type = layout.at("entries").at(binormal_index).at("type");
    if(tangent_index != -1)
        tangent_type = layout.at("entries").at(tangent_index).at("type");

    logger::info("Converting {} entries", std::count_if(offsets.begin(), offsets.end(), [](auto pair) { return pair.second; }));
    std::vector<uint8_t> output;
    for(uint32_t vertex_offset = 0; vertex_offset < vertices.size(); vertex_offset += stride) {
        uint32_t entry_offset = 0;
        float binormal[3] = {0, 0, 0};
        float tangent[3] = {0, 0, 0};
        float sign = 0;
        float normal[3] = {0, 0, 0};

        float converter[2] = {0, 0};
        for(auto iter = offsets.begin(); iter != offsets.end(); iter++) {
            int index = (int)(iter - offsets.begin());
            if(iter->second) {
                converter[0] = (float)(half)vertices.get<half>(vertex_offset + entry_offset);
                converter[1] = (float)(half)vertices.get<half>(vertex_offset + entry_offset + 2);
                output.insert(output.end(), reinterpret_cast<uint8_t*>(converter), reinterpret_cast<uint8_t*>(converter) + 8);
            } else {
                std::span<uint8_t> to_add = vertices.buf_.subspan(vertex_offset + entry_offset, iter->first);
                output.insert(output.end(), to_add.begin(), to_add.end());
            }

            if(calculate_normals && index == binormal_index - vert_index_offset) {
                utils::load_vector(binormal_type, vertex_offset, entry_offset, vertices, binormal);
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

    }
    return output;
}

void add_mesh_to_gltf(gltf2::Model &gltf, const DME &dme, uint32_t index) {
    int texcoord = 0;
    int color = 0;
    gltf2::Mesh gltf_mesh;
    gltf2::Primitive primitive;
    std::shared_ptr<const Mesh> mesh = dme.mesh(index);
    std::vector<uint32_t> offsets((std::size_t)mesh->vertex_stream_count(), 0);
    json input_layout = get_input_layout(dme.dmat()->material(index)->definition());
    
    std::vector<gltf2::Buffer> buffers;
    for(uint32_t j = 0; j < mesh->vertex_stream_count(); j++) {
        std::span<uint8_t> vertex_stream = mesh->vertex_stream(j);
        gltf2::Buffer buffer;
        logger::info("Expanding vertex stream {}", j);
        buffer.data = expand_halves(input_layout, vertex_stream, j);
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
            offsets.at(stream) += sizes.at(type);
            continue;
            attribute += std::to_string(color);
            color++;
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
    bufferview.byteStride = mesh->index_size() & 0xFF;
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

    gltf2::Node node;
    node.mesh = (int)gltf.meshes.size();
    gltf.nodes.push_back(node);
    
    gltf.meshes.push_back(gltf_mesh);
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
    
    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
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

    std::string input_str = parser.get<std::string>("input_file");
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

    std::vector<std::thread> image_processor_pool;
    for(int i = 0; i < THREAD_COUNT; i++) {
        image_processor_pool.push_back(std::thread{
            process_images, 
            std::cref(manager), 
            std::ref(image_queue), 
            output_directory
        });
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
        Parameter::Semantic::OVERLAY1
    };

    std::unordered_map<uint32_t, uint32_t> texture_indices;

    for(uint32_t i = 0; i < dme.mesh_count(); i++) {
        gltf2::Material material;
        std::pair<gltf2::TextureInfo, gltf2::TextureInfo> info_pair;
        for(Parameter::Semantic semantic : semantics) {
            switch(semantic) {
            case Parameter::Semantic::NORMAL_MAP:
                material.normalTexture.index = load_texture_info(
                    gltf, dme, i, texture_indices, image_queue, output_filename, semantic
                ).index;
                break;
            case Parameter::Semantic::BASE_COLOR:
                material.pbrMetallicRoughness.baseColorTexture = load_texture_info(
                    gltf, dme, i, texture_indices, image_queue, output_filename, semantic
                );
                break;
            case Parameter::Semantic::SPECULAR:
                info_pair = load_specular_info(
                    gltf, dme, i, texture_indices, image_queue, output_filename, semantic
                );
                material.pbrMetallicRoughness.metallicRoughnessTexture = info_pair.first;
                material.emissiveTexture = info_pair.second;
                material.emissiveFactor = {1.0, 1.0, 1.0};
                break;
            default:
                break;
            }
        }
        material.doubleSided = true;
        gltf.materials.push_back(material);

        add_mesh_to_gltf(gltf, dme, i);

        logger::info("Added mesh {} to gltf", i);
    }
    image_queue.close();
    logger::info("Joining image processing thread...");
    for(uint32_t i = 0; i < image_processor_pool.size(); i++) {
        image_processor_pool.at(i).join();
    }


    gltf.asset.version = "2.0";
    gltf.asset.generator = "DME Converter (C++) " + std::string(CPPDMOD_VERSION) + " via tinygltf";

    std::string format = parser.get<std::string>("--format");

    logger::info("Writing output file...");
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&gltf, output_filename.string(), false, format == "glb", format == "gltf", format == "glb");

    return 0;
}