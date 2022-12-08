#pragma once
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "instance.h"

namespace warpgate {
    struct RuntimeObject {
        mutable std::span<uint8_t> buf_;

        RuntimeObject(std::span<uint8_t> subspan, uint32_t version);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("RuntimeObject: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        std::string actor_file() const;
        // Not actually sure if this is render distance, but the values given would make sense as that
        ref<float> render_distance() const;
        ref<uint32_t> instance_count() const;

        Instance instance(uint32_t index) const;


    private:
        std::string actor_file_;
        std::vector<Instance> instances;
    };
}