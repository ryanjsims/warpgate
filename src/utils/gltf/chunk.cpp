#include "utils/gltf/chunk.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/gtx/quaternion.hpp>

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

int utils::gltf::chunk::add_mesh_to_gltf(
    tinygltf::Model &gltf, 
    const CNK0 &chunk,
    int material_base_index,
    std::string name
) {
    uint32_t render_batch_count = chunk.render_batch_count();
    std::span<RenderBatch> render_batches = chunk.render_batches();
    tinygltf::Node parent;
    parent.name = name;
    glm::dquat quat(glm::dvec3(M_PI / 2, 0.0, M_PI));
    parent.rotation = {quat.x, quat.y, quat.z, quat.w};
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
        vertex_accessor.minValues = {(double)minimum.x, (double)minimum.y, (double)minimum.height_far};
        vertex_accessor.maxValues = {(double)maximum.x, (double)maximum.y, (double)maximum.height_far};

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

        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        //primitive.material = material_base_index + i;
        mesh.primitives.push_back(primitive);

        node.mesh = (int)gltf.meshes.size();
        node.translation = {(i % 4) * 64.0, (i >> 2) * 64.0, 0};
        node.scale = {1.0, 1.0, 1.0 / 32};

        gltf.nodes.at(parent_index).children.push_back((int)gltf.nodes.size());

        gltf.meshes.push_back(mesh);
        gltf.nodes.push_back(node);
    }

    std::span<Vertex> raw_vertices = chunk.vertices();
    std::vector<Float3> vertices;
    std::vector<Float2> texcoords;
    std::vector<Color2> colors;
    for(Vertex raw_vertex : raw_vertices) {
        Float2 texcoord;
        texcoord.u = (float)raw_vertex.x / 64.0f;
        texcoord.v = (float)raw_vertex.y / 64.0f;
        texcoords.push_back(texcoord);

        Float3 vertex;
        vertex.x = (float)raw_vertex.x;
        vertex.y = (float)raw_vertex.y;
        vertex.z = (float)raw_vertex.height_near;
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

    tinygltf::Buffer colors_buffer;
    colors_buffer.data = std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(colors.data()), 
        reinterpret_cast<uint8_t*>(colors.data()) + colors.size() * sizeof(Color2)
    );

    gltf.buffers.push_back(colors_buffer);

    return parent_index;
}