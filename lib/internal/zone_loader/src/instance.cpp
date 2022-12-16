#include "instance.h"

#include <spdlog/spdlog.h>

using namespace warpgate::zone;

Instance::Instance(std::span<uint8_t> subspan, uint32_t version_): buf_(subspan), version(version_) {
    uint32_t base_size = 3 * sizeof(Float4);
    switch(version) {
    case 2:
        buf_ = buf_.first(base_size + 10);
        break;
    case 4:
        buf_ = buf_.first(base_size + 29);
        break;
    case 5:
        buf_ = buf_.first(
            base_size + 10 + sizeof(float) + sizeof(uint32_t) * 4 
            + sizeof(UIntMapEntry) * uint_map_entries_count()
            + sizeof(FloatMapEntry) * float_map_entries_count()
            + sizeof(Vector4MapEntry) * vector4_map_entries_count()
        );
        break;
    default:
        buf_ = buf_.first(base_size + 9);
        break;
    }
}

Instance::ref<Float4> Instance::translation() const {
    return get<Float4>(0);
}

Instance::ref<Float4> Instance::rotation() const {
    return get<Float4>(sizeof(Float4));
}

Instance::ref<Float4> Instance::scale() const {
    return get<Float4>(sizeof(Float4) * 2);
}

Instance::ref<uint32_t> Instance::unk_int32() const {
    switch(version) {
    case 4:
        spdlog::warn("Zone v4 unknown field");
        return get<uint32_t>(0);
    case 5:
        return get<uint32_t>(
            5 + sizeof(float) + sizeof(uint32_t) * 2
            + sizeof(UIntMapEntry) * uint_map_entries_count()
            + sizeof(FloatMapEntry) * float_map_entries_count()
        );
    default:
        return get<uint32_t>(sizeof(Float4) * 3);
    }
}

Instance::ref<uint8_t> Instance::unk_byte() const {
    switch(version) {
        case 4:
        case 5:
            spdlog::warn("Zone v4/5 unknown field");
            return get<uint8_t>(0);
        default:
            return get<uint8_t>(sizeof(Float4) * 3 + sizeof(uint32_t));
    }
}

Instance::ref<uint8_t> Instance::unk_byte2() const {
    switch(version) {
        case 2:
            return get<uint8_t>(sizeof(Float4) * 3 + sizeof(uint32_t) + sizeof(uint8_t));
        default:
            spdlog::warn("Field only valid in zone v2");
            return get<uint8_t>(0);
    }
}

Instance::ref<float> Instance::unk_float() const {
    switch(version) {
        case 1:
            return get<float>(sizeof(Float4) * 3 + sizeof(uint32_t) + sizeof(uint8_t));
        case 2:
            return get<float>(sizeof(Float4) * 3 + sizeof(uint32_t) + sizeof(uint8_t) * 2);
        default:
            spdlog::warn("Zone v4/5 unknown field");
            return get<float>(0);
    }
}

Instance::ref<uint32_t> Instance::uint_map_entries_count() const {
    switch(version) {
        case 5:
            return get<uint32_t>(sizeof(Float4) * 3 + 5 + sizeof(float));
        default:
            spdlog::warn("uint_map_entries_count field only valid in zone v5");
            return get<uint32_t>(0);
    }
}

Instance::ref<uint32_t> Instance::float_map_entries_count() const {
    switch(version) {
        case 5:
            return get<uint32_t>(
                sizeof(Float4) * 3 + 5 + sizeof(float) + sizeof(uint32_t) 
                + sizeof(UIntMapEntry) * uint_map_entries_count()
            );
        default:
            spdlog::warn("float_map_entries_count field only valid in zone v5");
            return get<uint32_t>(0);
    }
}

Instance::ref<uint32_t> Instance::vector4_map_entries_count() const {
    switch(version) {
        case 5:
            return get<uint32_t>(
                sizeof(Float4) * 3 + 5 + sizeof(float) + sizeof(uint32_t) * 3
                + sizeof(UIntMapEntry) * uint_map_entries_count()
                + sizeof(FloatMapEntry) * float_map_entries_count()
            );
        default:
            spdlog::warn("vector4_map_entries_count field only valid in zone v5");
            return get<uint32_t>(0);
    }
}

std::span<UIntMapEntry> Instance::uint_map() const {
    switch(version) {
        case 5:
            return std::span<UIntMapEntry>(
                reinterpret_cast<UIntMapEntry*>(buf_.data() + sizeof(Float4) * 3 + 5 + sizeof(float) + sizeof(uint32_t)),
                sizeof(UIntMapEntry) * uint_map_entries_count()
            );
        default:
            spdlog::warn("uint_map field only valid in zone v5");
            return {};
    }
}

std::span<FloatMapEntry> Instance::float_map() const {
    switch(version) {
        case 5:
            return std::span<FloatMapEntry>(
                reinterpret_cast<FloatMapEntry*>(buf_.data() + sizeof(Float4) * 3 + 5 + sizeof(float) + sizeof(uint32_t) * 2 + sizeof(UIntMapEntry) * uint_map_entries_count()),
                sizeof(FloatMapEntry) * float_map_entries_count()
            );
        default:
            spdlog::warn("float_map field only valid in zone v5");
            return {};
    }
}

std::span<Vector4MapEntry> Instance::vector4_map() const {
    switch(version) {
        case 5:
            return std::span<Vector4MapEntry>(
                reinterpret_cast<Vector4MapEntry*>(buf_.data() + sizeof(Float4) * 3 + 5 + sizeof(float) + sizeof(uint32_t) * 4 + sizeof(UIntMapEntry) * uint_map_entries_count() + sizeof(FloatMapEntry) * float_map_entries_count()),
                sizeof(Vector4MapEntry) * vector4_map_entries_count()
            );
        default:
            spdlog::warn("vector4_map field only valid in zone v5");
            return {};
    }
}

std::span<uint8_t> Instance::unk_data() const {
    switch(version) {
        case 4:
            return buf_.subspan(sizeof(Float4) * 3, 29);
        case 5:
            return buf_.subspan(sizeof(Float4) * 3, 5);
        default:
            spdlog::warn("unk_data field only valid in zone v4/5");
            return {};
    }
}

std::span<uint8_t> Instance::unk_data2() const {
    switch(version) {
        case 5:
            return buf_.subspan(
                sizeof(Float4) * 3 + 5 + sizeof(float) 
                  + sizeof(uint32_t) * 4 
                  + sizeof(UIntMapEntry) * uint_map_entries_count() 
                  + sizeof(FloatMapEntry) * float_map_entries_count()
                  + sizeof(Vector4MapEntry) * vector4_map_entries_count(),
                5
            );
        default:
            spdlog::warn("unk_data2 field only valid in zone v5");
            return {};
    }
}