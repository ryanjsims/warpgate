#include "cnk0.h"

#include <spdlog/spdlog.h>

using namespace warpgate;

CNK0::CNK0(std::span<uint8_t> subspan): buf_(subspan) {
    ChunkHeader header = this->header();
    if(std::strncmp(header.magic, "CNK0", 4) != 0) {
        spdlog::error(
            "warpgate::cnk_loader: not a CNK0 file (got magic \"{}{}{}{}\")",
            header.magic[0], header.magic[1], header.magic[2], header.magic[3]
        );
        throw std::invalid_argument("CNK0: invalid magic");
    }
    uint32_t offset = tiles_offset() + sizeof(uint32_t);
    uint32_t tile_count = this->tile_count();
    for(uint32_t tile_index = 0; tile_index < tile_count; tile_index++) {
        Tile tile(buf_.subspan(offset));
        tiles_.push_back(tile);
        offset += (uint32_t)tile.size();
    }
    tiles_size = offset - (tiles_offset() + sizeof(uint32_t));
}

CNK0::ref<ChunkHeader> CNK0::header() const {
    return get<ChunkHeader>(0);
}

CNK0::ref<uint32_t> CNK0::tile_count() const {
    return get<uint32_t>(tiles_offset());
}

std::vector<Tile> CNK0::tiles() const {
    return tiles_;
}

CNK0::ref<uint32_t> CNK0::unk1() const {
    return get<uint32_t>(unk1_offset());
}

CNK0::ref<uint32_t> CNK0::unk_array1_length() const {
    return get<uint32_t>(unk_array1_offset());
}

std::span<Unknown> CNK0::unk_array1() const {
    return std::span<Unknown>(
        reinterpret_cast<Unknown*>(buf_.subspan(unk_array1_offset() + sizeof(uint32_t)).data()), 
        unk_array1_length()
    );
}

CNK0::ref<uint32_t> CNK0::index_count() const {
    return get<uint32_t>(indices_offset());
}

std::span<uint16_t> CNK0::indices() const {
    return std::span<uint16_t>(
        reinterpret_cast<uint16_t*>(buf_.subspan(indices_offset() + sizeof(uint32_t)).data()),
        index_count()
    );
}

CNK0::ref<uint32_t> CNK0::vertex_count() const {
    uint32_t offset = vertices_offset();
    return get<uint32_t>(offset);
}

std::span<Vertex> CNK0::vertices() const {
    return std::span<Vertex>(
        reinterpret_cast<Vertex*>(buf_.subspan(vertices_offset() + sizeof(uint32_t)).data()),
        vertex_count()
    );
}

CNK0::ref<uint32_t> CNK0::render_batch_count() const {
    return get<uint32_t>(render_batches_offset());
}

std::span<RenderBatch> CNK0::render_batches() const {
    return std::span<RenderBatch>(
        reinterpret_cast<RenderBatch*>(buf_.subspan(render_batches_offset() + sizeof(uint32_t)).data()),
        render_batch_count()
    );
}

CNK0::ref<uint32_t> CNK0::optimized_draw_count() const {
    return get<uint32_t>(optimized_draw_offset());
}

std::span<OptimizedDraw> CNK0::optimized_draws() const {
    return std::span<OptimizedDraw>(
        reinterpret_cast<OptimizedDraw*>(buf_.subspan(optimized_draw_offset() + sizeof(uint32_t)).data()),
        optimized_draw_count()
    );
}

CNK0::ref<uint32_t> CNK0::unk_shorts_count() const {
    return get<uint32_t>(unk_shorts_offset());
}

std::span<uint16_t> CNK0::unk_shorts() const {
    return std::span<uint16_t>(
        reinterpret_cast<uint16_t*>(buf_.subspan(unk_shorts_offset() + sizeof(uint32_t)).data()),
        unk_shorts_count()
    );
}

CNK0::ref<uint32_t> CNK0::unk_vectors_count() const {
    return get<uint32_t>(unk_vectors_offset());
}

std::span<Vector3> CNK0::unk_vectors() const {
    return std::span<Vector3>(
        reinterpret_cast<Vector3*>(buf_.subspan(unk_vectors_offset() + sizeof(uint32_t)).data()),
        unk_vectors_count()
    );
}

CNK0::ref<uint32_t> CNK0::tile_occluder_info_count() const {
    return get<uint32_t>(tile_occluder_info_offset());
}

std::span<TileOccluderInfo> CNK0::tile_occluder_infos() const {
    return std::span<TileOccluderInfo>(
        reinterpret_cast<TileOccluderInfo*>(buf_.subspan(tile_occluder_info_offset() + sizeof(uint32_t)).data()),
        tile_occluder_info_count()
    );
}

uint32_t CNK0::tiles_offset() const {
    return sizeof(ChunkHeader);
}

uint32_t CNK0::unk1_offset() const {
    return tiles_offset() + sizeof(uint32_t) + tiles_size;
}

uint32_t CNK0::unk_array1_offset() const {
    return unk1_offset() + sizeof(uint32_t);
}

uint32_t CNK0::indices_offset() const {
    return unk_array1_offset() + sizeof(uint32_t) + unk_array1_length() * sizeof(Unknown);
}

uint32_t CNK0::vertices_offset() const {
    return indices_offset() + sizeof(uint32_t) + index_count() * sizeof(uint16_t);
}

uint32_t CNK0::render_batches_offset() const {
    return vertices_offset() + sizeof(uint32_t) + vertex_count() * sizeof(Vertex);
}

uint32_t CNK0::optimized_draw_offset() const {
    return render_batches_offset() + sizeof(uint32_t) + render_batch_count() * sizeof(RenderBatch);
}

uint32_t CNK0::unk_shorts_offset() const {
    return optimized_draw_offset() + sizeof(uint32_t) + optimized_draw_count() * sizeof(OptimizedDraw);
}

uint32_t CNK0::unk_vectors_offset() const {
    return unk_shorts_offset() + sizeof(uint32_t) + unk_shorts_count() * sizeof(uint16_t);
}

uint32_t CNK0::tile_occluder_info_offset() const {
    return unk_vectors_offset() + sizeof(uint32_t) + unk_vectors_count() * sizeof(Vector3);
}
