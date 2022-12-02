#pragma once
#include <stdexcept>
#include <span>
#include <vector>

struct Mesh {
    mutable std::span<uint8_t> buf_;

    Mesh(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Mesh: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }

    static size_t check_size(std::span<uint8_t> subspan) {
        Mesh mesh(subspan);
        size_t vertex_data_length = 0;
        for(uint32_t i = 0; i < mesh.vertex_stream_count(); i++) {
            vertex_data_length += mesh.vertex_count() * mesh.bytes_per_vertex(i);
        }
        return 8 * sizeof(uint32_t) + mesh.vertex_stream_count() * sizeof(uint32_t) + vertex_data_length;
    }

    ref<uint32_t> draw_offset() const;
    ref<uint32_t> draw_count() const;
    ref<uint32_t> bone_count() const;
    ref<uint32_t> unknown() const;
    ref<uint32_t> vertex_stream_count() const;
    ref<uint32_t> index_size() const;
    ref<uint32_t> index_count() const;
    ref<uint32_t> vertex_count() const;
    ref<uint32_t> bytes_per_vertex(uint32_t vertex_stream_index) const;
    std::span<uint8_t> vertex_stream(uint32_t vertex_stream_index) const;
    std::span<uint8_t> index_data() const;

private:
    std::vector<size_t> vertex_stream_offsets;
    size_t vertex_data_size;
    size_t index_offset() const;
};