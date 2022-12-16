#include "texture.h"

using namespace warpgate::chunk;

Texture::Texture(std::span<uint8_t> subspan): buf_(subspan) {
    buf_ = buf_.first(
        6 * sizeof(uint32_t) + color_length() + specular_length() 
        + extra1_length() + extra2_length() + extra3_length() + extra4_length()
    );
}

Texture::ref<uint32_t> Texture::color_length() const {
    return get<uint32_t>(color_offset());
}

std::span<uint8_t> Texture::color_nx_map() const {
    return buf_.subspan(color_offset() + sizeof(uint32_t), color_length());
}

Texture::ref<uint32_t> Texture::specular_length() const {
    return get<uint32_t>(specular_offset());
}

std::span<uint8_t> Texture::specular_ny_map() const {
    return buf_.subspan(specular_offset() + sizeof(uint32_t), specular_length());
}

Texture::ref<uint32_t> Texture::extra1_length() const {
    return get<uint32_t>(extra1_offset());
}

std::span<uint8_t> Texture::extra1() const {
    return buf_.subspan(extra1_offset() + sizeof(uint32_t), extra1_length());
}

Texture::ref<uint32_t> Texture::extra2_length() const {
    return get<uint32_t>(extra2_offset());
}

std::span<uint8_t> Texture::extra2() const {
    return buf_.subspan(extra2_offset() + sizeof(uint32_t), extra2_length());
}

Texture::ref<uint32_t> Texture::extra3_length() const {
    return get<uint32_t>(extra3_offset());
}

std::span<uint8_t> Texture::extra3() const {
    return buf_.subspan(extra3_offset() + sizeof(uint32_t), extra3_length());
}

Texture::ref<uint32_t> Texture::extra4_length() const {
    return get<uint32_t>(extra4_offset());
}

std::span<uint8_t> Texture::extra4() const {
    return buf_.subspan(extra4_offset() + sizeof(uint32_t), extra4_length());
}

uint32_t Texture::color_offset() const {
    return 0;
}

uint32_t Texture::specular_offset() const {
    return sizeof(uint32_t) + color_length();
}

uint32_t Texture::extra1_offset() const {
    return specular_offset() + sizeof(uint32_t) + specular_length();
}

uint32_t Texture::extra2_offset() const {
    return extra1_offset() + sizeof(uint32_t) + extra1_length();
}

uint32_t Texture::extra3_offset() const {
    return extra2_offset() + sizeof(uint32_t) + extra2_length();
}

uint32_t Texture::extra4_offset() const {
    return extra3_offset() + sizeof(uint32_t) + extra3_length();
}
