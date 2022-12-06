#pragma once
#include <stdexcept>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "material.h"

namespace warpgate {
    struct DMAT {
        mutable std::span<uint8_t> buf_;

        DMAT(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("DMAT: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> magic() const;
        ref<uint32_t> version() const;
        ref<uint32_t> filenames_length() const;
        std::span<uint8_t> texturename_data() const;
        const std::vector<std::string> textures() const;

        ref<uint32_t> material_count() const;
        std::shared_ptr<const Material> material(uint32_t index) const;

    private:
        std::vector<std::shared_ptr<Material>> materials;
        std::vector<std::string> texture_names;
        size_t materials_size;
        uint32_t material_offset() const;
        void parse_filenames();
        void parse_materials();
    };
}