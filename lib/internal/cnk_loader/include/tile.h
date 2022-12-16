#pragma once
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "eco.h"

namespace warpgate::chunk {
    struct Tile {
        mutable std::span<uint8_t> buf_;

        Tile(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Tile: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<int32_t> x() const;
        ref<int32_t> y() const;
        ref<int32_t> unk1() const;
        ref<int32_t> unk2() const; 
        ref<uint32_t> eco_count() const;

        std::vector<Eco> ecos() const;

        ref<uint32_t> index() const;
        ref<uint32_t> image_id() const;

        bool has_image() const;
        ref<uint32_t> image_length() const;
        std::span<uint8_t> image_data() const;

        ref<uint32_t> layer_length() const;
        std::span<uint8_t> layer_textures() const;

    private:
        std::vector<Eco> ecos_;
        uint32_t ecos_byte_size;

        uint32_t layer_offset() const;
    };
}