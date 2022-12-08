#include "flora.h"

using namespace warpgate;

Flora::Flora(std::span<uint8_t> subspan, uint32_t version_): buf_(subspan), version(version_) {
    name_ = std::string((char*)buf_.data());
    texture_ = std::string((char*)buf_.data() + name_.size() + 1);
    model_ = std::string((char*)buf_.data() + name_.size() + texture_.size() + 2);
    buf_ = buf_.first(name_.size() + texture_.size() + model_.size() + 3 + sizeof(bool) + 2 * sizeof(float) + (version > 3 ? 12 : 0));
}

Flora::ref<bool> Flora::unk_bool() const {
    return get<bool>(bool_offset());
}

Flora::ref<float> Flora::unk_float0() const {
    return get<float>(bool_offset() + sizeof(bool));
}

Flora::ref<float> Flora::unk_float1() const {
    return get<float>(bool_offset() + sizeof(bool) + sizeof(float));
}

std::span<uint8_t> Flora::unk_data_v45() const {
    if(version > 3) {
        return buf_.subspan(bool_offset() + sizeof(bool) + sizeof(float) * 2, 12);
    }
    return {};
}

std::string Flora::name() const {
    return name_;
}

std::string Flora::texture() const {
    return texture_;
}

std::string Flora::model() const {
    return model_;
}

uint32_t Flora::bool_offset() const {
    return (uint32_t)(name_.size() + texture_.size() + model_.size() + 3);
}