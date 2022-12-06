#include "parameter.h"
#include <string>

using namespace warpgate;

Parameter::Parameter(std::span<uint8_t> subspan): buf_(subspan) {
    buf_ = buf_.first(16 + length());
}

Parameter::ref<int32_t> Parameter::semantic_hash() const {
    return get<int32_t>(0);
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

std::string Parameter::semantic_name(int32_t semantic) {
    switch(Parameter::Semantic(semantic)) {
    case Parameter::Semantic::BASE_COLOR:
        return "Base Color";
    case Parameter::Semantic::NORMAL_MAP:
        return "Normal Map";
    case Parameter::Semantic::SPECULAR:
        return "Specular";
    case Parameter::Semantic::DETAIL_CUBE:
        return "Detail Cube";
    case Parameter::Semantic::DETAIL_SELECT:
        return "Detail Select";
    case Parameter::Semantic::OVERLAY0:
        return "Overlay 1";
    case Parameter::Semantic::OVERLAY1:
        return "Overlay 2";
    case Parameter::Semantic::BASE_CAMO:
        return "Base Camo";
    default:
        return "Unknown (" + std::to_string(semantic) + ")";
    }
}

std::string Parameter::semantic_name(Parameter::Semantic semantic) {
    return semantic_name((int32_t)semantic);
}