#pragma once
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "flora.h"

namespace warpgate::chunk {
    struct Eco {
        mutable std::span<uint8_t> buf_;

        Eco(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Eco: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> id() const;
        ref<uint32_t> flora_count() const;

        std::vector<Flora> floras() const;

    private:
        std::vector<Flora> floras_;

    };
}