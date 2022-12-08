#pragma once
#include <span>
#include <stdexcept>

#include "structs.h"

namespace warpgate {
    struct Light {
        mutable std::span<uint8_t> buf_;

        Light(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Light: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<LightType> type() const;
        ref<bool> unk_bool() const;
        ref<Float4> translation() const;
        ref<Float4> rotation() const;
        // Probably radii of effect
        ref<Float2> unk_floats() const;
        ref<Color4ARGB> color() const;

        std::span<uint8_t> unk_data() const;


    private:
        std::string name, color_name;
    };
}