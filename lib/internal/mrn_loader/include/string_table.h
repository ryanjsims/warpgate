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
    
    protected:
        std::span<uint32_t> m_offsets;
        std::vector<std::string> m_strings;

        StringTable();

        virtual ref<uint64_t> offsets_ptr() const;
        virtual ref<uint64_t> strings_ptr() const;
        virtual void set_size() const {
            buf_ = buf_.first(24 + count() * sizeof(uint32_t) + data_length());
        }
    };

    struct ExpandedStringTable : StringTable {
        ExpandedStringTable(std::span<uint8_t> subspan);

        std::span<const uint32_t> ids() const;
        std::span<const uint32_t> unknowns() const;

    protected:
        std::span<uint32_t> m_ids, m_unknowns;

        ref<uint64_t> offsets_ptr() const override;
        ref<uint64_t> strings_ptr() const override;
        void set_size() const override {
            buf_ = buf_.first(40 + 3 * count() * sizeof(uint32_t) + data_length());
        }
    
    private:
        ref<uint64_t> ids_ptr() const;
        ref<uint64_t> unknowns_ptr() const;
    };
}