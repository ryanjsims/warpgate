#pragma once
#include <cstdint>

namespace warpgate::chunk {
    struct ChunkHeader {
        char magic[4];
        uint32_t version;
    };

    struct Layer {
        uint32_t unk1, unk2;
    };

    struct Vertex {
        int16_t x, y, height_near, height_far;
        uint32_t color1, color2;
    };

    struct RenderBatch {
        uint32_t index_offset, index_count, vertex_offset, vertex_count;
    };

    struct OptimizedDraw {
        uint8_t data[320];
    };

    struct TileOccluderInfo {
        uint8_t data[64];
    };

    struct Unknown {
        int16_t short_;
        uint8_t byte1, byte2;
    };

    struct Vector3 {
        float x, y, z;
    };
}