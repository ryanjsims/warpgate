#include "light.h"

using namespace warpgate;

Light::Light(std::span<uint8_t> subspan): buf_(subspan) {

}

Light::ref<LightType> Light::type() const {

}

Light::ref<bool> Light::unk_bool() const {

}

Light::ref<Float4> Light::translation() const {

}

Light::ref<Float4> Light::rotation() const {

}

Light::ref<Float2> Light::unk_floats() const {

}

Light::ref<Color4ARGB> Light::color() const {

}

std::span<uint8_t> Light::unk_data() const {

}
