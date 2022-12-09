#include "utils/gltf/chunk.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/gtx/quaternion.hpp>

#include "utils/textures.h"
#include "utils/gltf/common.h"
#include "utils/tsqueue.h"

using namespace warpgate;

struct Float2 {
    float u, v;
};

struct Float3 {
    float x, y, z;
};

struct Color2 {
    uint32_t color1, color2;
};

int utils::gltf::chunk::add_chunks_to_gltf(
    tinygltf::Model &gltf,
    const warpgate::chunk::CNK0 &chunk0,
    const warpgate::chunk::CNK1 &chunk1,
    utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>> &image_queue,
    std::filesystem::path output_directory, 
    std::string name,
    int sampler_index,
    bool export_textures
) {
    int base_index = -1;
    if(export_textures) {
        base_index = add_materials_to_gltf(gltf, chunk1, image_queue, output_directory, name, sampler_index);
    }
    return add_mesh_to_gltf(gltf, chunk0, base_index, name);
}

int utils::gltf::chunk::add_mesh_to_gltf(
    tinygltf::Model &gltf, 
    const warpgate::chunk::CNK0 &chunk,
    int material_base_index,
    std::string name,
    bool include_colors
) {
    uint32_t render_batch_count = chunk.render_batch_count();
    std::span<warpgate::chunk::RenderBatch> render_batches = chunk.render_batches();
    tinygltf::Node parent;
    parent.name = name;
    int parent_index = (int)gltf.nodes.size();
    gltf.scenes.at(gltf.defaultScene).nodes.push_back((int)gltf.nodes.size());
    gltf.nodes.push_back(parent);

    for(uint32_t i = 0; i < render_batch_count; i++) {
        tinygltf::Node node;
        tinygltf::Mesh mesh;
        tinygltf::Primitive primitive;
        // Vertices
        tinygltf::Accessor vertex_accessor;
        vertex_accessor.bufferView = (int)gltf.bufferViews.size();
        vertex_accessor.byteOffset = 0;
        vertex_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        vertex_accessor.type = TINYGLTF_TYPE_VEC3;
        vertex_accessor.count = render_batches[i].vertex_count;

        auto[minimum, maximum] = chunk.aabb(i);
        vertex_accessor.minValues = {(double)minimum.x, (double)minimum.y, (double)minimum.height_near};
        vertex_accessor.maxValues = {(double)maximum.x, (double)maximum.y, (double)maximum.height_near};

        tinygltf::BufferView vertex_bufferview;
        vertex_bufferview.buffer = (int)gltf.buffers.size();
        vertex_bufferview.byteLength = render_batches[i].vertex_count * sizeof(Float3);
        vertex_bufferview.byteStride = sizeof(Float3);
        vertex_bufferview.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        vertex_bufferview.byteOffset = render_batches[i].vertex_offset * sizeof(Float3);

        primitive.attributes["POSITION"] = (int)gltf.accessors.size();

        gltf.bufferViews.push_back(vertex_bufferview);
        gltf.accessors.push_back(vertex_accessor);

        // Indices
        tinygltf::Accessor index_accessor;
        index_accessor.bufferView = (int)gltf.bufferViews.size();
        index_accessor.byteOffset = 0;
        index_accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
        index_accessor.type = TINYGLTF_TYPE_SCALAR;
        index_accessor.count = render_batches[i].index_count;

        tinygltf::BufferView index_bufferview;
        index_bufferview.buffer = (int)gltf.buffers.size() + 1;
        index_bufferview.byteLength = render_batches[i].index_count * sizeof(uint16_t);
        index_bufferview.byteStride = sizeof(uint16_t);
        index_bufferview.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        index_bufferview.byteOffset = render_batches[i].index_offset * sizeof(uint16_t);

        primitive.indices = (int)gltf.accessors.size();
        gltf.bufferViews.push_back(index_bufferview);
        gltf.accessors.push_back(index_accessor);

        // Texcoords
        tinygltf::Accessor texcoord_accessor;
        texcoord_accessor.bufferView = (int)gltf.bufferViews.size();
        texcoord_accessor.byteOffset = 0;
        texcoord_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        texcoord_accessor.type = TINYGLTF_TYPE_VEC2;
        texcoord_accessor.count = render_batches[i].vertex_count;

        tinygltf::BufferView texcoord_bufferview;
        texcoord_bufferview.buffer = (int)gltf.buffers.size() + 2;
        texcoord_bufferview.byteLength = render_batches[i].vertex_count * sizeof(Float2);
        texcoord_bufferview.byteStride = sizeof(Float2);
        texcoord_bufferview.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        texcoord_bufferview.byteOffset = render_batches[i].vertex_offset * sizeof(Float2);

        primitive.attributes["TEXCOORD_0"] = (int)gltf.accessors.size();
        gltf.bufferViews.push_back(texcoord_bufferview);
        gltf.accessors.push_back(texcoord_accessor);

        // Colors
        if(include_colors) {
            tinygltf::Accessor color_0_accessor;
            color_0_accessor.bufferView = (int)gltf.bufferViews.size();
            color_0_accessor.byteOffset = 0;
            color_0_accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
            color_0_accessor.type = TINYGLTF_TYPE_VEC4;
            color_0_accessor.count = render_batches[i].vertex_count;
            color_0_accessor.normalized = true;

            tinygltf::BufferView color_0_bufferview;
            color_0_bufferview.buffer = (int)gltf.buffers.size() + 3;
            color_0_bufferview.byteLength = render_batches[i].vertex_count * sizeof(Color2);
            color_0_bufferview.byteStride = sizeof(Color2);
            color_0_bufferview.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            color_0_bufferview.byteOffset = render_batches[i].vertex_offset * sizeof(Color2);

            primitive.attributes["COLOR_0"] = (int)gltf.accessors.size();
            gltf.bufferViews.push_back(color_0_bufferview);
            gltf.accessors.push_back(color_0_accessor);

            tinygltf::Accessor color_1_accessor;
            color_1_accessor.bufferView = (int)gltf.bufferViews.size();
            color_1_accessor.byteOffset = 0;
            color_1_accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
            color_1_accessor.type = TINYGLTF_TYPE_VEC4;
            color_1_accessor.count = render_batches[i].vertex_count;
            color_1_accessor.normalized = true;

            tinygltf::BufferView color_1_bufferview;
            color_1_bufferview.buffer = (int)gltf.buffers.size() + 3;
            color_1_bufferview.byteLength = render_batches[i].vertex_count * sizeof(Color2) - sizeof(uint32_t);
            color_1_bufferview.byteStride = sizeof(Color2);
            color_1_bufferview.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            color_1_bufferview.byteOffset = render_batches[i].vertex_offset * sizeof(Color2) + sizeof(uint32_t);

            primitive.attributes["COLOR_1"] = (int)gltf.accessors.size();
            gltf.bufferViews.push_back(color_1_bufferview);
            gltf.accessors.push_back(color_1_accessor);
        }

        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        if(material_base_index >= 0) {
            primitive.material = material_base_index + (i & 2) + ((i >> 3) & 1);
        }
        mesh.primitives.push_back(primitive);

        node.mesh = (int)gltf.meshes.size();
        node.translation = {(i >> 2) * 64.0, 0, (i % 4) * 64.0};

        gltf.nodes.at(parent_index).children.push_back((int)gltf.nodes.size());

        gltf.meshes.push_back(mesh);
        gltf.nodes.push_back(node);
    }

    std::span<warpgate::chunk::Vertex> raw_vertices = chunk.vertices();
    std::vector<Float3> vertices;
    std::vector<Float2> texcoords;
    std::vector<Color2> colors;
    uint32_t vertex_mesh = 0;
    for(uint32_t i = 0; i < raw_vertices.size(); i++) {
        for(uint32_t render_batch = vertex_mesh; true; render_batch = (render_batch + 1) % render_batches.size()) {
            if(i - render_batches[render_batch].vertex_offset < render_batches[render_batch].vertex_count) {
               vertex_mesh = render_batch;
               break; 
            }
        }
        warpgate::chunk::Vertex raw_vertex = raw_vertices[i];
        Float2 texcoord;
        texcoord.u = (float)raw_vertex.y / 128.0f + (((vertex_mesh >> 2) & 1) * 0.5f);
        texcoord.v = (float)raw_vertex.x / 128.0f + ((vertex_mesh & 1) * 0.5f);
        texcoords.push_back(texcoord);

        Float3 vertex;
        vertex.x = (float)raw_vertex.y;
        vertex.y = (float)raw_vertex.height_near / 32.0f;
        vertex.z = (float)raw_vertex.x;
        vertices.push_back(vertex);

        Color2 color;
        color.color1 = raw_vertex.color1;
        color.color2 = raw_vertex.color2;
        colors.push_back(color);
    }
    std::span<uint16_t> indices = chunk.indices();
    tinygltf::Buffer vertex_buffer;
    vertex_buffer.data = std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(vertices.data()), 
        reinterpret_cast<uint8_t*>(vertices.data()) + vertices.size() * sizeof(Float3)
    );

    gltf.buffers.push_back(vertex_buffer);

    tinygltf::Buffer index_buffer;
    index_buffer.data = std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(indices.data()), 
        reinterpret_cast<uint8_t*>(indices.data()) + indices.size_bytes()
    );

    gltf.buffers.push_back(index_buffer);

    tinygltf::Buffer texcoord_buffer;
    texcoord_buffer.data = std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(texcoords.data()), 
        reinterpret_cast<uint8_t*>(texcoords.data()) + texcoords.size() * sizeof(Float2)
    );

    gltf.buffers.push_back(texcoord_buffer);

    if(include_colors) {
        tinygltf::Buffer colors_buffer;
        colors_buffer.data = std::vector<uint8_t>(
            reinterpret_cast<uint8_t*>(colors.data()), 
            reinterpret_cast<uint8_t*>(colors.data()) + colors.size() * sizeof(Color2)
        );

        gltf.buffers.push_back(colors_buffer);
    }

    return parent_index;
}

int utils::gltf::chunk::add_materials_to_gltf(
    tinygltf::Model &gltf,
    const warpgate::chunk::CNK1 &chunk,
    utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>> &image_queue,
    std::filesystem::path output_directory,
    std::string name,
    int sampler_index
) {
    int material_start_index = (int)gltf.materials.size();
    for(uint32_t texture = 0; texture < chunk.textures_count(); texture++) {
        tinygltf::Material material;
        std::span<uint8_t> cnx_map = chunk.textures()[texture].color_nx_map();
        std::span<uint8_t> sny_map = chunk.textures()[texture].specular_ny_map();
        std::shared_ptr<uint8_t[]> cnx_data = std::make_shared<uint8_t[]>(cnx_map.size());
        std::shared_ptr<uint8_t[]> sny_data = std::make_shared<uint8_t[]>(sny_map.size());
        std::memcpy(cnx_data.get(), cnx_map.data(), cnx_map.size());
        std::memcpy(sny_data.get(), sny_map.data(), sny_map.size());
        std::string texture_basename = name + "_" + std::to_string(texture);
        std::filesystem::path save_path = std::filesystem::path(name) / texture_basename;
        if(!std::filesystem::exists(output_directory / "textures" / name)) {
            std::filesystem::create_directories(output_directory / "textures" / name);
        }

        image_queue.enqueue({save_path.string(), cnx_data, (uint32_t)cnx_map.size(), sny_data, (uint32_t)sny_map.size()});

        material.pbrMetallicRoughness.baseColorTexture.index = utils::gltf::add_texture_to_gltf(gltf, output_directory / "textures" / name / (texture_basename + "_C.png"), output_directory, sampler_index);
        material.pbrMetallicRoughness.metallicRoughnessTexture.index = utils::gltf::add_texture_to_gltf(gltf, output_directory / "textures" / name / (texture_basename + "_S.png"), output_directory, sampler_index);
        material.normalTexture.index = utils::gltf::add_texture_to_gltf(gltf, output_directory / "textures" / name / (texture_basename + "_N.png"), output_directory, sampler_index);
        material.doubleSided = true;
        material.name = name;
        gltf.materials.push_back(material);
    }
    return material_start_index;
}

tinygltf::Model utils::gltf::chunk::build_gltf_from_chunks(
    const warpgate::chunk::CNK0 &chunk0,
    const warpgate::chunk::CNK1 &chunk1,
    std::filesystem::path output_directory,
    bool export_textures,
    utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>> &image_queue,
    std::string name
) {
    tinygltf::Model gltf;
    tinygltf::Sampler sampler;
    sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    sampler.wrapS = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
    sampler.wrapT = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
    int sampler_index = (int)gltf.samplers.size();
    gltf.samplers.push_back(sampler);
    
    gltf.defaultScene = (int)gltf.scenes.size();
    gltf.scenes.push_back({});

    add_chunks_to_gltf(gltf, chunk0, chunk1, image_queue, output_directory, name, sampler_index, export_textures);

    gltf.asset.version = "2.0";
    gltf.asset.generator = "warpgate " + std::string(WARPGATE_VERSION) + " via tinygltf";
    return gltf;
}