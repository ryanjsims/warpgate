#include "string_table.h"

using namespace warpgate::mrn;

StringTable::StringTable(std::span<uint8_t> subspan): buf_(subspan) {
    m_offsets = std::span<uint32_t>((uint32_t*)(buf_.data() + offsets_ptr()), count());
    uint64_t strings_offset = strings_ptr();
    for(auto it = m_offsets.begin(); it != m_offsets.end(); it++) {
        m_strings.push_back(std::string((char*)(buf_.data() + strings_offset + *it)));
    }
    buf_ = buf_.first(24 + count() * sizeof(uint32_t) + data_length());
}

StringTable::ref<uint32_t> StringTable::count() const {
    return get<uint32_t>(0);
}

StringTable::ref<uint32_t> StringTable::data_length() const {
    return get<uint32_t>(4);
}

std::span<const uint32_t> StringTable::offsets() const {
    return m_offsets;
}

std::vector<std::string> StringTable::strings() const {
    return m_strings;
}

StringTable::ref<uint64_t> StringTable::offsets_ptr() const {
    return get<uint64_t>(8);
}

StringTable::ref<uint64_t> StringTable::strings_ptr() const {
    return get<uint64_t>(16);
}