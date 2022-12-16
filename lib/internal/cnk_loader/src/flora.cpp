#include "flora.h"

using namespace warpgate::chunk;

Flora::Flora(std::span<uint8_t> subspan): buf_(subspan) {
    uint32_t layer_count = this->layer_count();
    buf_ = buf_.first(sizeof(uint32_t) + layer_count * sizeof(Layer));
}

Flora::ref<uint32_t> Flora::layer_count() const {
    return get<uint32_t>(0);
}

std::span<Layer> Flora::layers() const {
    return std::span<Layer>(reinterpret_cast<Layer*>(buf_.data() + 4), layer_count());
}