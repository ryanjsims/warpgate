#pragma once
#include <span>
#include <stdexcept>

#include "structs.h"

namespace warpgate::zone {
    struct Instance {
        mutable std::span<uint8_t> buf_;

        Instance(std::span<uint8_t> subspan, uint32_t version);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Instance: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<Float4> translation() const;
        ref<Float4> rotation() const;
        ref<Float4> scale() const;

        ref<uint32_t> unk_int32() const;

        // Version 1 and 2 only
        ref<uint8_t> unk_byte() const;

        // Version 2 only
        ref<uint8_t> unk_byte2() const;

        // Versions 1, 2, 5 only
        ref<float> unk_float() const;

        // Version 5 only
        ref<uint32_t> uint_map_entries_count() const;
        ref<uint32_t> float_map_entries_count() const;
        ref<uint32_t> vector4_map_entries_count() const;

        std::span<UIntMapEntry> uint_map() const;
        std::span<FloatMapEntry> float_map() const;
        std::span<Vector4MapEntry> vector4_map() const;

        // Version 4 and 5 only
        std::span<uint8_t> unk_data() const;

        // Version 5 only
        std::span<uint8_t> unk_data2() const;
    
    private:
        uint32_t version;
    };
}