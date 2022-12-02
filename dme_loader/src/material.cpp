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

std::vector<Parameter> Material::parameters() const {
    std::vector<Parameter> parameters;
    uint32_t count = param_count();
    uint32_t offset = 0;
    for(uint32_t i = 0; i < count; i++) {
        Parameter p(buf_.subspan(16 + offset));
        offset += (uint32_t)p.size();
        parameters.push_back(p);
    }
    return parameters;
}
