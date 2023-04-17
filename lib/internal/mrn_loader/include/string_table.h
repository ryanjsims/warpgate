#pragma once
#include <cstring>
#include <stdexcept>
#include <memory>
#include <span>
#include <vector>

namespace warpgate::mrn {
    struct StringTable {
        mutable std::span<uint8_t> buf_;

        StringTable(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("StringTable: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> count() const;
        ref<uint32_t> data_length() const;
        std::span<const uint32_t> offsets() const;
        std::vector<std::string> strings() const;

        std::string operator[](size_t index) {
            return m_strings[index];
        }
    
    private:
        std::span<uint32_t> m_offsets;
        std::vector<std::string> m_strings;

        ref<uint64_t> offsets_ptr() const;
        ref<uint64_t> strings_ptr() const;
    };
}