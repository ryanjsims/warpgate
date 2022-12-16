#pragma once
#include <cstdint>
#include <string_view>

namespace warpgate::zone {
    struct ChunkInfo {
        uint32_t tile_count;
        int32_t start_x, start_y;
        uint32_t count_x, count_y;
    };

    struct Color4 {
        uint8_t r, g, b, a;
    };

    struct Color4ARGB {
        uint8_t a, r, g, b;
    };

    struct EcoTint {
        Color4 color_rgba;
        uint32_t strength;
    };

    struct Float2 {
        float x, y;
    };

    struct Float4 {
        float x, y, z, w;
    };

    struct FloatMapEntry {
        uint32_t key;
        float value;
    };

    struct InvisWall {
        // No layout info known :(
    };

    enum class LightType: uint32_t {
        Point = 1,
        Spot
    };

    struct Offsets {
        uint32_t ecos, floras, invis_walls, objects, lights, unknown;
    };

    struct Offsetsv45 {
        uint32_t ecos, floras, invis_walls, objects, lights, unknown, decals;
    };

    struct PerTileInfo {
        uint32_t quad_count;
        float width, height;
        uint32_t vertex_count;
    };

    struct UIntMapEntry {
        uint32_t key, value;
    };

    struct Vector4MapEntry {
        uint32_t key;
        Float4 value;
    };

    struct ZoneVersionHeader {
        char magic_[4];
        uint32_t version;

        std::string_view magic() {
            return std::string_view(magic_, 4);
        }
    };

    struct ZoneHeader {
        char magic_[4];
        uint32_t version;
        Offsets offsets;
        PerTileInfo per_tile;
        ChunkInfo chunk_info;

        std::string_view magic() {
            return std::string_view(magic_, 4);
        }
    };

    struct ZoneHeaderv45 {
        char magic_[4];
        uint32_t version, unknown;
        Offsetsv45 offsets;
        PerTileInfo per_tile;
        ChunkInfo chunk_info;

        std::string_view magic() {
            return std::string_view(magic_, 4);
        }
    };
}