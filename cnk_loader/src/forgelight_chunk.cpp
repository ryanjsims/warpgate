#include "forgelight_chunk.h"

#include <spdlog/spdlog.h>

constexpr auto operator""_MB( unsigned long long const x )
    -> long
{ return 1024L*1024L*x; }

constexpr long BUF_SIZE = 1_MB;
constexpr uint32_t window_bits = 20;

#define LZHAM_DEFINE_ZLIB_API
#include "lzham.h"

Chunk::Chunk(std::span<uint8_t> subspan): buf_(subspan) {}

Chunk::ref<Header> Chunk::header() const {
    return get<Header>(0);
}

Chunk::ref<uint32_t> Chunk::decompressed_size() const {
    return get<uint32_t>(sizeof(Header));
}

Chunk::ref<uint32_t> Chunk::compressed_size() const {
    return get<uint32_t>(sizeof(Header) + sizeof(uint32_t));
}

std::span<uint8_t> Chunk::compressed_data() const {
    return buf_.subspan(sizeof(Header) + 2 * sizeof(uint32_t));
}

std::unique_ptr<uint8_t[]> Chunk::decompress() const {
    z_stream stream{};
    std::unique_ptr<uint8_t[]> output = std::make_unique<uint8_t[]>(sizeof(Header) + decompressed_size());
    std::shared_ptr<uint8_t[]> input_buffer = std::make_shared<uint8_t[]>(BUF_SIZE);
    std::shared_ptr<uint8_t[]> output_buffer = std::make_shared<uint8_t[]>(BUF_SIZE);

    std::memcpy(output.get(), buf_.data(), sizeof(Header));

    stream.next_in = input_buffer.get();
    stream.avail_in = 0;
    stream.next_out = output_buffer.get();
    stream.avail_out = BUF_SIZE;

    int status;
    if(status = inflateInit2(&stream, window_bits)) {
        spdlog::error("inflateInit2() failed with status {}!", lzham_z_error(status));
        std::exit(status);
    }

    uint32_t remaining = compressed_size() - 4, index = 0, output_index = sizeof(Header);
    while(true) {
        if(!stream.avail_in) {
            uint32_t count = std::min((uint32_t)BUF_SIZE, remaining);
            std::memcpy(input_buffer.get(), compressed_data().data() + index, count);
            stream.next_in = input_buffer.get();
            stream.avail_in = count;
            index += count;
            remaining -= count;
        }

        status = inflate(&stream, Z_SYNC_FLUSH);

        if(status == Z_STREAM_END || !stream.avail_out) {
            uint32_t count = BUF_SIZE - stream.avail_out;
            std::memcpy(output.get() + output_index, output_buffer.get(), count);
            output_index += count;
            stream.next_out = output_buffer.get();
            stream.avail_out = BUF_SIZE;
        }

        if(status == Z_STREAM_END) {
            break;
        } else if (status != Z_OK) {
            spdlog::error("inflate() failed with status {}!", lzham_z_error(status));
            std::exit(status);
        }
    }

    if((status = inflateEnd(&stream)) != Z_OK) {
        spdlog::error("inflate() failed with status {}!", lzham_z_error(status));
        std::exit(status);
    }

    return std::move(output);
}