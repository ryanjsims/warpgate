#include "eco_layer.h"

using namespace warpgate;

EcoLayer::EcoLayer(std::span<uint8_t> subspan): buf_(subspan) {
    flora_name_ = std::string((char*)buf_.data() + sizeof(float) * 7 + 1);
    buf_ = buf_.first(sizeof(float) * 7 + flora_name_.size() + 2 + sizeof(uint32_t) + tint_count() * sizeof(EcoTint));
}

EcoLayer::ref<float> EcoLayer::density() const {
    return get<float>(0);
}

EcoLayer::ref<float> EcoLayer::min_scale() const {
    return get<float>(4);
}

EcoLayer::ref<float> EcoLayer::max_scale() const {
    return get<float>(8);
}

EcoLayer::ref<float> EcoLayer::slope_peak() const {
    return get<float>(12);
}

EcoLayer::ref<float> EcoLayer::slope_extent() const {
    return get<float>(16);
}

EcoLayer::ref<float> EcoLayer::min_elevation() const {
    return get<float>(20);
}

EcoLayer::ref<float> EcoLayer::max_elevation() const {
    return get<float>(24);
}

EcoLayer::ref<uint8_t> EcoLayer::min_alpha() const {
    return get<uint8_t>(28);
}

EcoLayer::ref<uint32_t> EcoLayer::tint_count() const {
    return get<uint32_t>(tints_offset());
}

std::string EcoLayer::flora_name() const {
    return flora_name_;
}

std::span<EcoTint> EcoLayer::tints() const {
    return std::span<EcoTint>(reinterpret_cast<EcoTint*>(buf_.data() + tints_offset() + sizeof(uint32_t)), tint_count());
}

uint32_t EcoLayer::tints_offset() const {
    return (uint32_t)(sizeof(float) * 7 + flora_name_.size() + 2);
}