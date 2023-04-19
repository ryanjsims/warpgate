#pragma once
#include <stdint.h>

#include "glm/vec3.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/mat4x4.hpp"

namespace warpgate::mrn {
    struct BoneHierarchyEntry {
        int32_t unknown, parent_index, start_index, chain_length;
    };

    struct DequantizationFactors {
        union {
            glm::vec3 v_min;
            float a_min[3];
        };
        union {
            glm::vec3 v_scaled_extent;
            float a_scaled_extent[3];
        };
    };

    struct DequantizationInfo {
        union {
            glm::vec<3, uint8_t> v_init;
            uint8_t a_init[3];
        };
        union {
            glm::vec<3, uint8_t> v_factor_index;
            uint8_t a_factor_index[3];
        };
    };
}