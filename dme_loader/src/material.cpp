#include "material.h"

Material::Material(std::span<uint8_t> subspan): buf_(subspan) {
    buf_ = buf_.first(length() + 8);
}

Material::ref<uint32_t> Material::namehash() const {
    return get<uint32_t>(0);
}

Material::ref<uint32_t> Material::length() const {
    return get<uint32_t>(4);
}

Material::ref<uint32_t> Material::definition() const {
    return get<uint32_t>(8);
}

Material::ref<uint32_t> Material::param_count() const {
    return get<uint32_t>(12);
}

std::span<uint8_t> Material::param_data() const {
    return buf_.subspan(16, length() - 8);
}
