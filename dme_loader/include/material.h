#pragma once
#include <stdexcept>
#include <span>
#include <vector>

#include "parameter.h"

struct Material {
    mutable std::span<uint8_t> buf_;

    Material(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Material: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }
    
    ref<uint32_t> namehash() const;
    ref<uint32_t> length() const;
    ref<uint32_t> definition() const;
    ref<uint32_t> param_count() const;
    std::vector<Parameter> parameters() const;
};