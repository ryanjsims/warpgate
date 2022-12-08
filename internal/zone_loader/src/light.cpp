#include "light.h"

using namespace warpgate;

Light::Light(std::span<uint8_t> subspan): buf_(subspan) {
    name_ = std::string((char*)buf_.data());
    color_name_ = std::string((char*)buf_.data() + name_.size() + 1);
    buf_ = buf_.first(data_offset() + 26);
}

Light::ref<LightType> Light::type() const {
    return get<LightType>(type_offset());
}

Light::ref<bool> Light::unk_bool() const {
    return get<bool>(bool_offset());
}

Light::ref<Float4> Light::translation() const {
    return get<Float4>(translation_offset());
}

Light::ref<Float4> Light::rotation() const {
    return get<Float4>(rotation_offset());
}

Light::ref<Float2> Light::unk_floats() const {
    return get<Float2>(floats_offset());
}

Light::ref<Color4ARGB> Light::color() const {
    return get<Color4ARGB>(color_offset());
}

std::span<uint8_t> Light::unk_data() const {
    return buf_.subspan(data_offset(), 26);
}

std::string Light::name() const {
    return name_;
}

std::string Light::color_name() const {
    return color_name_;
}

uint32_t Light::type_offset() const {
    return (uint32_t)(name_.size() + color_name_.size() + 2);
}

uint32_t Light::bool_offset() const {
    return type_offset() + sizeof(LightType);
}

uint32_t Light::translation_offset() const {
    return bool_offset() + sizeof(bool);
}

uint32_t Light::rotation_offset() const {
    return translation_offset() + sizeof(Float4);
}

uint32_t Light::floats_offset() const {
    return rotation_offset() + sizeof(Float4);
}

uint32_t Light::color_offset() const {
    return floats_offset() + sizeof(Float2);
}

uint32_t Light::data_offset() const {
    return color_offset() + sizeof(Color4ARGB);
}
