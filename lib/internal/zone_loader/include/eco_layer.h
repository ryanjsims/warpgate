#pragma once
#include <span>
#include <stdexcept>
#include <vector>

#include "structs.h"

namespace warpgate::zone {
    struct EcoLayer {
        mutable std::span<uint8_t> buf_;

        EcoLayer(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("EcoLayer: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<float> density() const;
        ref<float> min_scale() const;
        ref<float> max_scale() const;
        ref<float> slope_peak() const;
        ref<float> slope_extent() const;
        ref<float> min_elevation() const;
        ref<float> max_elevation() const;
        ref<uint8_t> min_alpha() const;
        ref<uint32_t> tint_count() const;

        std::string flora_name() const;
        std::span<EcoTint> tints() const;

    private:
        std::string flora_name_;
        uint32_t tints_offset() const;
    };
}