#pragma once

#include <algorithm>
#include <span>

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