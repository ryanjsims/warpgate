#include "utils.h"

using namespace warpgate::mrn;

Vector3 warpgate::mrn::unpack_translation(Vector3Short quantized, DequantizationFactors factors) {
    return {
        factors.v_scaled_extent.x * quantized.x + factors.v_min.x,
        factors.v_scaled_extent.y * quantized.y + factors.v_min.y,
        factors.v_scaled_extent.z * quantized.z + factors.v_min.z
    };
}

Vector3 warpgate::mrn::unpack_translation(uint32_t bit_packed_value, DequantizationFactors factors) {
    Vector3Short quantized = {(bit_packed_value >> 21), (bit_packed_value >> 10) & 0x7FF, bit_packed_value & 0x3FF};
    return unpack_translation(quantized, factors);
}

Quaternion warpgate::mrn::unpack_rotation(Vector3Short quantized, DequantizationFactors factors) {
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

Quaternion warpgate::mrn::unpack_initial_rotation(Vector3Byte quantized) {
    DequantizationFactors factors;
    factors.v_min = {-1, -1, -1};
    factors.v_scaled_extent = {1 / 128.0f, 1 / 128.0f, 1 / 128.0f};
    return unpack_rotation({quantized.x, quantized.y, quantized.z}, factors);
}