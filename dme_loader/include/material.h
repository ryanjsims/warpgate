#pragma once
#include <stdexcept>
#include <span>
#include <unordered_map>
#include <vector>

#include "parameter.h"

struct Material {
    mutable std::span<uint8_t> buf_;

    Material(std::span<uint8_t> subspan);
    Material(std::span<uint8_t> subspan, std::vector<std::string> textures);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Material: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }
    
    ref<uint32_t> namehash() const;
    ref<uint32_t> length() const;
    ref<uint32_t> definition() const;
    ref<uint32_t> param_count() const;
    Parameter parameter(uint32_t index) const;
    std::string texture(int32_t semantic) const;
    std::string texture(Parameter::Semantic semantic) const;
private:
    std::vector<Parameter> parameters;
    std::unordered_map<int32_t, std::string> semantic_textures;

    void parse_parameters();
    void parse_semantics(std::vector<std::string> textures);
};