#include "material.h"
#include "jenkins.h"
#include "utils.h"

#include <spdlog/spdlog.h>

namespace logger = spdlog;

Material::Material(std::span<uint8_t> subspan): buf_(subspan) {
    buf_ = buf_.first(length() + 8);
    parse_parameters();
}

Material::Material(std::span<uint8_t> subspan, std::vector<std::string> textures): buf_(subspan) {
    buf_ = buf_.first(length() + 8);
    parse_parameters();
    parse_semantics(textures);
}

void Material::parse_parameters() {
    uint32_t count = param_count();
    uint32_t offset = 0;
    for(uint32_t i = 0; i < count; i++) {
        Parameter p(buf_.subspan(16 + offset));
        offset += (uint32_t)p.size();
        parameters.push_back(p);
    }
}

void Material::parse_semantics(std::vector<std::string> textures) {
    logger::info("Parsing semantics");
    std::unordered_map<uint32_t, std::string> hash_to_names;
    for(std::string texture : textures) {
        std::string upper = utils::uppercase(texture);
        uint32_t namehash = jenkins::oaat(upper);
        hash_to_names[namehash] = texture;
    }

    for(Parameter p : parameters) {
        std::unordered_map<uint32_t, std::string>::iterator value;
        if(p.type() == Parameter::D3DXParamType::TEXTURE && (value = hash_to_names.find(p.get<uint32_t>(16))) != hash_to_names.end()) {
            semantic_textures[p.semantic_hash()] = value->second;
            std::string name = Parameter::semantic_name(p.semantic_hash());
            logger::info("Semantic '{}' = '{}'", name, value->second);
        }
    }
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

Parameter Material::parameter(uint32_t index) const {
    return parameters.at(index);
}

std::optional<std::string> Material::texture(int32_t semantic) const {
    auto value = semantic_textures.find(semantic);
    if(value != semantic_textures.end()) {
        return value->second;
    }
    return {};
}

std::optional<std::string> Material::texture(Parameter::Semantic semantic) const {
    return texture((int32_t)semantic);
}