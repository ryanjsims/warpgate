#pragma once
#include "dme.h"

namespace warpgate {
    struct Bone {
        DME::ref<PackedMat4> inverse_bind_matrix;
        DME::ref<AABB> bounding_box;
        DME::ref<uint32_t> namehash;
    };
}