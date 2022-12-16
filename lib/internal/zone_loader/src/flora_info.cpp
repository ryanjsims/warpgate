#include "flora_info.h"

using namespace warpgate::zone;

FloraInfo::FloraInfo(std::span<uint8_t> subspan): buf_(subspan) {
    uint32_t offset = 4;
    uint32_t count = layer_count();
    for(uint32_t i = 0; i < count; i++) {
        EcoLayer layer(buf_.subspan(offset));
        layers_.push_back(layer);
        offset += (uint32_t)layer.size();
    }
    buf_ = buf_.first(offset);
}

FloraInfo::ref<uint32_t> FloraInfo::layer_count() const {
    return get<uint32_t>(0);
}

std::vector<EcoLayer> FloraInfo::layers() const {
    return layers_;
}
