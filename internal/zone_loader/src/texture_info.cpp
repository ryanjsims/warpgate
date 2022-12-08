#include "texture_info.h"

using namespace warpgate;

TextureInfo::TextureInfo(std::span<uint8_t> subspan): buf_(subspan) {
    name_ = std::string((char*)buf_.data());
    cnx_map_name_ = std::string((char*)buf_.data() + name_.size() + 1);
    sbny_map_name_ = std::string((char*)buf_.data() + name_.size() + cnx_map_name_.size() + 2);
    physics_material_name_ = std::string((char*)buf_.data() + detail_repeat_offset() + sizeof(uint32_t) + 5 * sizeof(float));
    buf_ = buf_.first(detail_repeat_offset() + sizeof(uint32_t) + 5 * sizeof(float) + physics_material_name_.size() + 1);
}

TextureInfo::ref<uint32_t> TextureInfo::detail_repeat() const {
    return get<uint32_t>(detail_repeat_offset());
}

TextureInfo::ref<float> TextureInfo::blend_strength() const {
    return get<float>(detail_repeat_offset() + sizeof(uint32_t));
}

TextureInfo::ref<float> TextureInfo::spec_min() const {
    return get<float>(detail_repeat_offset() + sizeof(uint32_t) + sizeof(float));
}

TextureInfo::ref<float> TextureInfo::spec_max() const {
    return get<float>(detail_repeat_offset() + sizeof(uint32_t) + sizeof(float) * 2);
}

TextureInfo::ref<float> TextureInfo::smoothness_min() const {
    return get<float>(detail_repeat_offset() + sizeof(uint32_t) + sizeof(float) * 3);
}

TextureInfo::ref<float> TextureInfo::smoothness_max() const {
    return get<float>(detail_repeat_offset() + sizeof(uint32_t) + sizeof(float) * 4);
}

std::string TextureInfo::name() const {
    return name_;
}

std::string TextureInfo::cnx_map_name() const {
    return cnx_map_name_;
}

std::string TextureInfo::sbny_map_name() const {
    return sbny_map_name_;
}

std::string TextureInfo::physics_material_name() const {
    return physics_material_name_;
}

uint32_t TextureInfo::detail_repeat_offset() const {
    return (uint32_t)(name_.size() + cnx_map_name_.size() + sbny_map_name_.size() + 3);
}
