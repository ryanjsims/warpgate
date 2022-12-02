#include "mesh.h"
#include <spdlog/spdlog.h>

Mesh::Mesh(std::span<uint8_t> subspan): buf_(subspan) {
    vertex_data_size = 0;
    uint32_t vertex_stream_count = this->vertex_stream_count();
    uint32_t vertex_count = this->vertex_count();
    spdlog::info("Loading mesh with vertex_stream_count = {} and vertex count = {}", vertex_stream_count, vertex_count);
    for(uint32_t i = 0; i < vertex_stream_count; i++) {
        spdlog::info("Loading vertex stream: {}...", i);
        vertex_stream_offsets.push_back(32 + 4 * i + vertex_data_size);
        vertex_data_size += bytes_per_vertex(i) * vertex_count;
        spdlog::info("Loaded vertex stream {}", i);
    }
    buf_ = buf_.first(index_offset() + index_count() * (index_size() & 0xFF));
}

Mesh::ref<uint32_t> Mesh::draw_offset() const {
    return get<uint32_t>(0);
}

Mesh::ref<uint32_t> Mesh::draw_count() const {
    return get<uint32_t>(4);
}

Mesh::ref<uint32_t> Mesh::bone_count() const {
    return get<uint32_t>(8);
}

Mesh::ref<uint32_t> Mesh::unknown() const {
    return get<uint32_t>(12);
}

Mesh::ref<uint32_t> Mesh::vertex_stream_count() const {
    return get<uint32_t>(16);
}

Mesh::ref<uint32_t> Mesh::index_size() const {
    return get<uint32_t>(20);
}

Mesh::ref<uint32_t> Mesh::index_count() const {
    return get<uint32_t>(24);
}

Mesh::ref<uint32_t> Mesh::vertex_count() const {
    return get<uint32_t>(28);
}

Mesh::ref<uint32_t> Mesh::bytes_per_vertex(uint32_t vertex_stream_index) const {
    return get<uint32_t>(vertex_stream_offsets[vertex_stream_index]);
}

std::span<uint8_t> Mesh::vertex_stream(uint32_t vertex_stream_index) const {
    return buf_.subspan(vertex_stream_offsets[vertex_stream_index] + 4, bytes_per_vertex(vertex_stream_index) * vertex_count());
}

size_t Mesh::index_offset() const {
    return 32 + 4 * vertex_stream_count() + vertex_data_size;
}

std::span<uint8_t> Mesh::index_data() const {
    return buf_.subspan(index_offset(), index_count() * (index_size() & 0xFF));
}