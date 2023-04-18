#pragma once
#include <stdint.h>

namespace warpgate::mrn {
    struct Quaternion {
        float x, y, z, w;

        Quaternion operator*(Quaternion &other) {
            return {
                w * other.x + x * other.w - y * other.z + z * other.y, //t1
                w * other.y + x * other.z + y * other.w - z * other.x, //t2
                w * other.z - x * other.z + y * other.x + z * other.w, //t3
                w * other.w - x * other.x - y * other.y - z * other.z, //t0
            };
        }
    };

    struct Vector4 {
        float x, y, z, w;
    };

    struct Vector3 {
        float x, y, z;

        Vector3 operator+(Vector3 &other) {
            return {x + other.x, y + other.y, z + other.z};
        }
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