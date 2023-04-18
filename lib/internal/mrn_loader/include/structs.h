#pragma once
#include <stdint.h>

namespace warpgate::mrn {
    struct Quaternion {
        float x, y, z, w;
    };

    struct Vector4 {
        float x, y, z, w;
    };

    struct Vector3 {
        float x, y, z;
    };

    struct Vector3Short {
        uint16_t x, y, z;
    };

    struct Vector3Byte {
        uint8_t x, y, z;
    };

    struct BoneHierarchyEntry {
        int32_t unknown, parent_index, start_index, chain_length;
    };

    struct DequantizationFactors {
        union {
            Vector3 v_min;
            float a_min[3];
        };
        union {
            Vector3 v_scaled_extent;
            float a_scaled_extent[3];
        };
    };

    struct DequantizationInfo {
        union {
            Vector3Byte v_init;
            uint8_t a_init[3];
        };
        union {
            Vector3Byte v_factor_index;
            uint8_t a_factor_index[3];
        };
    };
}