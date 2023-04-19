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
    Vector3 unpack_translation(Vector3Short quantized, DequantizationFactors factors);
    Vector3 unpack_translation(uint32_t bit_packed_value, DequantizationFactors factors);
    Quaternion unpack_rotation(Vector3Short quantized, DequantizationFactors factors);
    Quaternion unpack_initial_rotation(Vector3Byte quantized);
}