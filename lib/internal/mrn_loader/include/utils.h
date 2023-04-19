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
    glm::vec3 unpack_translation(glm::u16vec3 quantized, DequantizationFactors factors);
    glm::vec3 unpack_translation(uint32_t bit_packed_value, DequantizationFactors factors);
    glm::quat unpack_rotation(glm::u16vec3 quantized, DequantizationFactors factors);
    glm::quat unpack_initial_rotation(glm::u8vec3 quantized);
}