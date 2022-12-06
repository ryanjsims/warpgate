#include "tile.h"

using namespace warpgate;

Tile::Tile(std::span<uint8_t> subspan): buf_(subspan) {
    uint32_t offset = 20;
    uint32_t eco_count = this->eco_count();
    for(uint32_t eco_index = 0; eco_index < eco_count; eco_index++) {
        Eco eco(buf_.subspan(offset));
        ecos_.push_back(eco);
        offset += (uint32_t)eco.size();
    }
    ecos_byte_size = offset - 20;
    buf_ = buf_.first(layer_offset() + 4 + layer_length());
}

Tile::ref<int32_t> Tile::x() const {
    return get<int32_t>(0);
}

Tile::ref<int32_t> Tile::y() const {
    return get<int32_t>(4);
}

Tile::ref<int32_t> Tile::unk1() const {
    return get<int32_t>(8);
}

Tile::ref<int32_t> Tile::unk2() const {
    return get<int32_t>(12);
}

Tile::ref<uint32_t> Tile::eco_count() const {
    return get<uint32_t>(16);
}

std::vector<Eco> Tile::ecos() const {
    return ecos_;
}

Tile::ref<uint32_t> Tile::index() const {
    return get<uint32_t>(20 + ecos_byte_size);
}

Tile::ref<uint32_t> Tile::image_id() const {
    return get<uint32_t>(24 + ecos_byte_size);
}

bool Tile::has_image() const {
    return image_id() != 0;
}

Tile::ref<uint32_t> Tile::image_length() const {
    if(!has_image()) {
        return 0;
    }
    return get<uint32_t>(28 + ecos_byte_size);
}

std::span<uint8_t> Tile::image_data() const {
    return buf_.subspan(32 + ecos_byte_size, image_length());
}

Tile::ref<uint32_t> Tile::layer_length() const {
    return get<uint32_t>(layer_offset());
}

std::span<uint8_t> Tile::layer_textures() const {
    return buf_.subspan(layer_offset() + 4, layer_length());
}

uint32_t Tile::layer_offset() const {
    if(!has_image()) {
        return 28 + ecos_byte_size;
    }
    return 32 + ecos_byte_size + image_length();
}
