#include "packet.h"

using namespace warpgate::mrn;

Header::Header(std::span<uint8_t> subspan): buf_(subspan) {
    bool align = 40 % alignment() != 0;
    uint32_t alignment_offset = align ? (alignment() - 40 % alignment()) : 0;
    buf_ = buf_.first(40 + alignment_offset);
}

Header::ref<uint64_t> Header::magic() const {
    return get<uint64_t>(0);
}

Header::ref<PacketType> Header::type() const {
    return get<PacketType>(8);
}

Header::ref<uint32_t> Header::index() const {
    return get<uint32_t>(12);
}

std::span<uint32_t, 4> Header::unknown_array() const {
    return std::span<uint32_t, 4>((uint32_t*)(buf_.data() + 16), 4);
}

Header::ref<uint32_t> Header::data_length() const {
    return get<uint32_t>(32);
}

Header::ref<uint32_t> Header::alignment() const {
    return get<uint32_t>(36);
}

Packet::Packet(std::span<uint8_t> subspan): buf_(subspan) {
    m_header = std::make_shared<Header>(buf_);
    buf_ = buf_.first(m_header->size() + m_header->data_length());
}

std::shared_ptr<const Header> Packet::header() const {
    return m_header;
}

std::span<uint8_t> Packet::data() const {
    return buf_.subspan(m_header->size());
}

SkeletonPacket::SkeletonPacket(std::shared_ptr<Packet> packet): Packet(*packet) {
    m_skeleton = std::make_shared<SkeletonData>(data());
}

SkeletonPacket::SkeletonPacket(std::span<uint8_t> subspan): Packet(subspan) {
    m_skeleton = std::make_shared<SkeletonData>(data());
}

std::shared_ptr<SkeletonData> SkeletonPacket::skeleton() const {
    return m_skeleton;
}

FilenamesPacket::FilenamesPacket(std::shared_ptr<Packet> packet): Packet(*packet) {
    m_files = std::make_shared<FileData>(data());
}

FilenamesPacket::FilenamesPacket(std::span<uint8_t> subspan): Packet(subspan) {
    m_files = std::make_shared<FileData>(data());
}

std::shared_ptr<FileData> FilenamesPacket::files() const {
    return m_files;
}

SkeletonNamesPacket::SkeletonNamesPacket(std::shared_ptr<Packet> packet) : Packet(*packet) {
    m_skeleton_names = std::make_shared<ExpandedStringTable>(data().subspan(skeleton_names_ptr()));
}

SkeletonNamesPacket::SkeletonNamesPacket(std::span<uint8_t> subspan) : Packet(subspan) {
    m_skeleton_names = std::make_shared<ExpandedStringTable>(data().subspan(skeleton_names_ptr()));
}

std::shared_ptr<ExpandedStringTable> SkeletonNamesPacket::skeleton_names() const {
    return m_skeleton_names;
}

SkeletonNamesPacket::ref<uint64_t> SkeletonNamesPacket::skeleton_names_ptr() const {
    return get<uint64_t>(header()->size());
}
