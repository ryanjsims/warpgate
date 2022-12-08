#include "eco.h"

using namespace warpgate;

Eco::Eco(std::span<uint8_t> subspan): buf_(subspan)  {
    texture_info_ = std::make_shared<TextureInfo>(buf_.subspan(4));
    flora_info_ = std::make_shared<FloraInfo>(buf_.subspan(4 + texture_info_->size()));
    buf_ = buf_.first(4 + texture_info_->size() + flora_info_->size());
}

Eco::ref<uint32_t> Eco::index() const {
    return get<uint32_t>(0);
}

std::shared_ptr<TextureInfo> Eco::texture_info() const {
    return texture_info_;
}

std::shared_ptr<FloraInfo> Eco::flora_info() const {
    return flora_info_;
}
