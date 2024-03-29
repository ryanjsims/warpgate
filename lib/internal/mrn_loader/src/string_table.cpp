#include "string_table.h"
#include <algorithm>

using namespace warpgate::mrn;

StringTable::StringTable(std::span<uint8_t> subspan): buf_(subspan) {
    m_offsets = std::span<uint32_t>((uint32_t*)(buf_.data() + offsets_ptr()), count());
    uint64_t strings_offset = strings_ptr();
    for(auto it = m_offsets.begin(); it != m_offsets.end(); it++) {
        m_strings.push_back(std::string((char*)(buf_.data() + strings_offset + *it)));
    }
    set_size();
}

StringTable::StringTable() {}

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

ExpandedStringTable::ExpandedStringTable(std::span<uint8_t> subspan) {
    buf_ = subspan;
    m_ids = std::span<uint32_t>((uint32_t*)(buf_.data() + ids_ptr()), count());
    m_offsets = std::span<uint32_t>((uint32_t*)(buf_.data() + offsets_ptr()), count());
    std::vector<uint32_t> sorted_offsets{m_offsets.begin(), m_offsets.end()};
    std::sort(sorted_offsets.begin(), sorted_offsets.end());
    m_unknowns = std::span<uint32_t>((uint32_t*)(buf_.data() + ids_ptr()), count());
    uint64_t strings_offset = strings_ptr();
    for(auto it = sorted_offsets.begin(); it != sorted_offsets.end(); it++) {
        m_strings.push_back(std::string((char*)(buf_.data() + strings_offset + *it)));
    }
    set_size();
}

std::span<const uint32_t> ExpandedStringTable::ids() const {
    return m_ids;
}

std::span<const uint32_t> ExpandedStringTable::unknowns() const {
    return m_unknowns;
}

StringTable::ref<uint64_t> ExpandedStringTable::offsets_ptr() const {
    return get<uint64_t>(16);
}

StringTable::ref<uint64_t> ExpandedStringTable::strings_ptr() const {
    return get<uint64_t>(32);
}

StringTable::ref<uint64_t> ExpandedStringTable::ids_ptr() const {
    return get<uint64_t>(8);
}

StringTable::ref<uint64_t> ExpandedStringTable::unknowns_ptr() const {
    return get<uint64_t>(24);
}
