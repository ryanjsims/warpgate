#include "file_data.h"
#include "utils.h"

using namespace warpgate::mrn;

FileData::FileData(std::span<uint8_t> subspan): buf_(subspan) {
    m_filenames = std::make_shared<StringTable>(buf_.subspan(filenames_ptr()));
    m_filetypes = std::make_shared<StringTable>(buf_.subspan(filetypes_ptr()));
    m_source_filenames = std::make_shared<StringTable>(buf_.subspan(source_filenames_ptr()));
    m_animation_names = std::make_shared<StringTable>(buf_.subspan(animation_names_ptr()));
    
    std::span<uint32_t> crc32_hashes_be((uint32_t*)(buf_.data() + crc32_hashes_ptr()), m_filenames->count());
    for(auto it = crc32_hashes_be.begin(); it != crc32_hashes_be.end(); it++) {
        m_crc32_hashes.push_back(swap_endianness(*it));
    }
    buf_ = buf_.first(40 
        + m_filenames->size() 
        + m_filetypes->size() 
        + m_source_filenames->size() 
        + m_animation_names->size() 
        + m_crc32_hashes.size() * sizeof(uint32_t)
    );
}

std::shared_ptr<StringTable> FileData::filenames() const {
    return m_filenames;
}

std::shared_ptr<StringTable> FileData::filetypes() const {
    return m_filetypes;
}

std::shared_ptr<StringTable> FileData::source_filenames() const {
    return m_source_filenames;
}

std::shared_ptr<StringTable> FileData::animation_names() const {
    return m_animation_names;
}

std::vector<uint32_t> FileData::crc32_hashes() const {
    return m_crc32_hashes;
}

FileData::ref<uint64_t> FileData::filenames_ptr() const {
    return get<uint64_t>(0);
}

FileData::ref<uint64_t> FileData::filetypes_ptr() const {
    return get<uint64_t>(8);
}

FileData::ref<uint64_t> FileData::source_filenames_ptr() const {
    return get<uint64_t>(16);
}

FileData::ref<uint64_t> FileData::animation_names_ptr() const {
    return get<uint64_t>(24);
}

FileData::ref<uint64_t> FileData::crc32_hashes_ptr() const {
    return get<uint64_t>(32);
}
