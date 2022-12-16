#include "cnk1.h"

using namespace warpgate::chunk;

CNK1::CNK1(std::span<uint8_t> subspan): buf_(subspan) {
    uint32_t offset = sizeof(ChunkHeader) + sizeof(uint32_t);
    for(uint32_t texture_index = 0; texture_index < textures_count(); texture_index++) {
        Texture texture(buf_.subspan(offset));
        textures_.push_back(texture);
        offset += (uint32_t)texture.size();
    }
}

CNK1::ref<ChunkHeader> CNK1::header() const {
    return get<ChunkHeader>(0);
}

CNK1::ref<uint32_t> CNK1::textures_count() const {
    return get<uint32_t>(sizeof(ChunkHeader));
}

std::vector<Texture> CNK1::textures() const {
    return textures_;
}
