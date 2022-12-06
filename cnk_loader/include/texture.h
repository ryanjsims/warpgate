#pragma once
#include <memory>
#include <span>
#include <stdexcept>

namespace warpgate {
    struct Texture {
        mutable std::span<uint8_t> buf_;

        Texture(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Texture: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> color_length() const;
        std::span<uint8_t> color_nx_map() const;

        ref<uint32_t> specular_length() const;
        std::span<uint8_t> specular_ny_map() const;

        ref<uint32_t> extra1_length() const;
        std::span<uint8_t> extra1() const;

        ref<uint32_t> extra2_length() const;
        std::span<uint8_t> extra2() const;

        ref<uint32_t> extra3_length() const;
        std::span<uint8_t> extra3() const;

        ref<uint32_t> extra4_length() const;
        std::span<uint8_t> extra4() const;
    
    private:
        uint32_t color_offset() const;
        uint32_t specular_offset() const;
        uint32_t extra1_offset() const;
        uint32_t extra2_offset() const;
        uint32_t extra3_offset() const;
        uint32_t extra4_offset() const;
    };
}