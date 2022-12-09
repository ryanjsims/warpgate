#pragma once
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "structs.h"
#include "tile.h"

namespace warpgate::chunk {
    struct CNK0 {
        mutable std::span<uint8_t> buf_;

        CNK0(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("CNK0: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<ChunkHeader> header() const;
        ref<uint32_t> tile_count() const;
        std::vector<Tile> tiles() const;

        ref<uint32_t> unk1() const;
        ref<uint32_t> unk_array1_length() const;
        std::span<Unknown> unk_array1() const;

        ref<uint32_t> index_count() const;
        std::span<uint16_t> indices() const;

        ref<uint32_t> vertex_count() const;
        std::span<Vertex> vertices() const;

        ref<uint32_t> render_batch_count() const;
        std::span<RenderBatch> render_batches() const;

        std::pair<Vertex, Vertex> aabb(uint32_t render_batch) const;

        ref<uint32_t> optimized_draw_count() const;
        std::span<OptimizedDraw> optimized_draws() const;

        ref<uint32_t> unk_shorts_count() const;
        std::span<uint16_t> unk_shorts() const;

        ref<uint32_t> unk_vectors_count() const;
        std::span<Vector3> unk_vectors() const;

        ref<uint32_t> tile_occluder_info_count() const;
        std::span<TileOccluderInfo> tile_occluder_infos() const;

    private:
        uint32_t tiles_offset() const;
        uint32_t unk1_offset() const;
        uint32_t unk_array1_offset() const;
        uint32_t indices_offset() const;
        uint32_t vertices_offset() const;
        uint32_t render_batches_offset() const;
        uint32_t optimized_draw_offset() const;
        uint32_t unk_shorts_offset() const;
        uint32_t unk_vectors_offset() const;
        uint32_t tile_occluder_info_offset() const;

        uint32_t tiles_size;
        std::vector<Tile> tiles_;
        std::vector<std::pair<Vertex, Vertex>> aabbs_;
    };
}