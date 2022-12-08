#pragma once
#include <span>
#include <stdexcept>

namespace warpgate {
    struct Flora {
        mutable std::span<uint8_t> buf_;

        Flora(std::span<uint8_t> subspan, uint32_t version);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Flora: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<bool> unk_bool() const;
        ref<float> unk_float0() const;
        ref<float> unk_float1() const;
        std::span<uint8_t> unk_data_v45() const;

        std::string name() const;
        std::string texture() const;
        std::string model() const;
    
    private:
        std::string name_, texture_, model_;
        uint32_t version;

        uint32_t bool_offset() const;
    };
}