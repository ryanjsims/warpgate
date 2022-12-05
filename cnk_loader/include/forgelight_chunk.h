#pragma once
#include <memory>
#include <span>
#include <stdexcept>

struct Header {
    char magic[4];
    uint32_t version;
};

struct Chunk {
    mutable std::span<uint8_t> buf_;

    Chunk(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Chunk: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }

    ref<Header> header() const;
    ref<uint32_t> decompressed_size() const;
    ref<uint32_t> compressed_size() const;

    std::span<uint8_t> compressed_data() const;
    std::unique_ptr<uint8_t[]> decompress() const;
};