#include "eco.h"

using namespace warpgate;

Eco::Eco(std::span<uint8_t> subspan): buf_(subspan) {
    uint32_t offset = 8;
    uint32_t flora_count = this->flora_count();
    for(uint32_t flora_index = 0; flora_index < flora_count; flora_index++) {
        Flora flora(buf_.subspan(offset));
        floras_.push_back(flora);
        offset += (uint32_t)flora.size();
    }
    buf_ = buf_.first(offset);
}

Eco::ref<uint32_t> Eco::id() const {
    return get<uint32_t>(0);
}

Eco::ref<uint32_t> Eco::flora_count() const {
    return get<uint32_t>(4);
}

std::vector<Flora> Eco::floras() const {
    return floras_;
}