#pragma once
#include <cstring>
#include <span>
#include <stdexcept>

namespace warpgate::zone {
    struct TextureInfo {
        mutable std::span<uint8_t> buf_;

        TextureInfo(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("TextureInfo: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> detail_repeat() const;
        ref<float> blend_strength() const;
        ref<float> spec_min() const;
        ref<float> spec_max() const;
        ref<float> smoothness_min() const;
        ref<float> smoothness_max() const;

        std::string name() const;
        std::string cnx_map_name() const;
        std::string sbny_map_name() const;
        std::string physics_material_name() const;

    private:
        std::string name_, cnx_map_name_, sbny_map_name_, physics_material_name_;
        uint32_t detail_repeat_offset() const;
    };
}