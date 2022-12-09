#pragma once
#include <span>
#include <stdexcept>
#include <vector>

#include "eco_layer.h"

namespace warpgate::zone {
    struct FloraInfo {
        mutable std::span<uint8_t> buf_;

        FloraInfo(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("FloraInfo: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> layer_count() const;
        std::vector<EcoLayer> layers() const;
    
    private:
        std::vector<EcoLayer> layers_;
    };
}