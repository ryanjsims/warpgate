#pragma once
#include <cstdint>

struct PackedMat4 {
    float data[4][3];
};

struct AABB {
    struct {
        float x, y, z;
    } min, max;
};

struct BoneMapEntry {
    uint16_t bone_index, global_index;
};

struct DrawCall {
    uint32_t unk0, bone_start, bone_count, delta, unk1, vertex_offset, vertex_count, index_offset, index_count;
};

