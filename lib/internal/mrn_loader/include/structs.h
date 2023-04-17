#pragma once
#include <stdint.h>

namespace warpgate::mrn {
    struct Quaternion {
        float x, y, z, w;
    };

    struct Vector4 {
        float x, y, z, w;
    };

    struct BoneHierarchyEntry {
        int32_t unknown, parent_index, start_index, chain_length;
    };
}