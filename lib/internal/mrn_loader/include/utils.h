#pragma once

#include <algorithm>
#include <span>

#include "structs.h"

namespace warpgate {
    template<typename T>
    T swap_endianness(const T& val)
    {
        auto in = std::as_bytes(std::span(&val, 1));
        T result;
        auto out = std::as_writable_bytes(std::span(&result, 1));
        std::copy(in.rbegin(), in.rend(), out.begin());
        return result;
    }
}

namespace warpgate::mrn {
    Vector3 unpack_translation(Vector3Short quantized, DequantizationFactors factors) {
        return {
            factors.v_scaled_extent.x * quantized.x + factors.v_min.x,
            factors.v_scaled_extent.y * quantized.y + factors.v_min.y,
            factors.v_scaled_extent.z * quantized.z + factors.v_min.z
        };
    }

    Vector3 unpack_translation(uint32_t bit_packed_value, DequantizationFactors factors) {
        Vector3Short quantized = {(bit_packed_value >> 21), (bit_packed_value >> 10) & 0x7FF, bit_packed_value & 0x3FF};
        return unpack_translation(quantized, factors);
    }

    Quaternion unpack_rotation(Vector3Short quantized, DequantizationFactors factors) {
        Vector3 dequantized = unpack_translation(quantized, factors);

        float sq_magn = dequantized.x * dequantized.x + dequantized.y * dequantized.y + dequantized.z * dequantized.z;
        float scalar = 2.0f / (sq_magn + 1.0f);

        return {
            scalar * dequantized.x,
            scalar * dequantized.y,
            scalar * dequantized.z,
            (1.0f - sq_magn) / (1.0f + sq_magn)
        };
    }

    Quaternion unpack_initial_rotation(Vector3Byte quantized) {
        DequantizationFactors factors;
        factors.v_min = {-1, -1, -1};
        factors.v_scaled_extent = {1 / 128.0f, 1 / 128.0f, 1 / 128.0f};
        return unpack_rotation({quantized.x, quantized.y, quantized.z}, factors);
    }
}