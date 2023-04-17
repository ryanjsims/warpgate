#pragma once
#include <span>
#include <memory>
#include <vector>

#include "string_table.h"

namespace warpgate::mrn {    
    struct FileData {
        mutable std::span<uint8_t> buf_;

        FileData(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Packet: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        std::shared_ptr<StringTable> filenames() const;
        std::shared_ptr<StringTable> filetypes() const;
        std::shared_ptr<StringTable> source_filenames() const;
        std::shared_ptr<StringTable> animation_names() const;

        std::vector<uint32_t> crc32_hashes() const;

    private:
        std::shared_ptr<StringTable> m_filenames, m_filetypes, m_source_filenames, m_animation_names;
        std::vector<uint32_t> m_crc32_hashes;

        ref<uint64_t> filenames_ptr() const;
        ref<uint64_t> filetypes_ptr() const;
        ref<uint64_t> source_filenames_ptr() const;
        ref<uint64_t> animation_names_ptr() const;
        ref<uint64_t> crc32_hashes_ptr() const;
    };
}