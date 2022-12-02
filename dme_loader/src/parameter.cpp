#include "parameter.h"

Parameter::Parameter(std::span<uint8_t> subspan): buf_(subspan) {
    buf_ = buf_.first(16 + length());
}

Parameter::ref<uint32_t> Parameter::namehash() const {
    return get<uint32_t>(0);
}

Parameter::ref<Parameter::D3DXParamClass> Parameter::_class() const {
    return get<D3DXParamClass>(4);
}

Parameter::ref<Parameter::D3DXParamType> Parameter::type() const {
    return get<D3DXParamType>(8);
}

Parameter::ref<uint32_t> Parameter::length() const {
    return get<uint32_t>(12);
}

std::span<uint8_t> Parameter::data() const {
    return buf_.subspan(16, length());
}
