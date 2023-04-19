#include "skeleton_data.h"

using namespace warpgate::mrn;

OrientationData::OrientationData(std::span<uint8_t> subspan, size_t bone_count): buf_(subspan) {
    uint64_t transforms_offset = transforms_ptr() + transforms_ptr_base();
    uint64_t translation_offset = get<uint64_t>(transforms_offset) + transforms_ptr_base();
    uint64_t rotation_offset = get<uint64_t>(transforms_offset + 8) + transforms_ptr_base();

    m_offsets = std::span<glm::vec4>((glm::vec4*)(buf_.data() + translation_offset), bone_count);
    m_rotations = std::span<glm::quat>((glm::quat*)(buf_.data() + rotation_offset), bone_count);

    buf_ = buf_.first(16 + data_length());
}

OrientationData::ref<uint32_t> OrientationData::data_length() const {
    return get<uint32_t>(16);
}

OrientationData::ref<uint32_t> OrientationData::alignment() const {
    return get<uint32_t>(20);
}

OrientationData::ref<uint32_t> OrientationData::element_count() const {
    return get<uint32_t>(24);
}

OrientationData::ref<bool> OrientationData::full() const {
    return get<bool>(28);
}

OrientationData::ref<uint32_t> OrientationData::element_type() const {
    return get<uint32_t>(32);
}

OrientationData::ref<uint64_t> OrientationData::unknown_ptr1() const {
    return get<uint64_t>(40);
}

OrientationData::ref<uint64_t> OrientationData::unknown_ptr2() const {
    return get<uint64_t>(56);
}

std::span<glm::vec4> OrientationData::offsets() {
    return m_offsets;
}

std::span<glm::quat> OrientationData::rotations() {
    return m_rotations;
}

OrientationData::ref<uint64_t> OrientationData::element_type_ptr() const {
    return get<uint64_t>(0);
}

OrientationData::ref<uint64_t> OrientationData::transforms_ptr() const {
    return get<uint64_t>(48);
}


SkeletonData::SkeletonData(std::span<uint8_t> subspan): buf_(subspan) {
    uint64_t bone_names_offset = bone_name_table_ptr();
    m_bone_names = std::make_shared<StringTable>(buf_.subspan(bone_names_offset));
    uint64_t orientations_offset = orientations_packet_ptr() + 16;
    m_orientations = std::make_shared<OrientationData>(buf_.subspan(orientations_offset), bone_count());
    m_chains = std::span<BoneHierarchyEntry>((BoneHierarchyEntry*)(buf_.data() + chains_offset()), chain_count());
}

SkeletonData::ref<glm::quat> SkeletonData::rotation() const {
    return get<glm::quat>(0);
}

SkeletonData::ref<glm::vec4> SkeletonData::position() const {
    return get<glm::vec4>(16);
}

SkeletonData::ref<uint32_t> SkeletonData::chain_count() const {
    return get<uint32_t>(32);
}

SkeletonData::ref<uint32_t> SkeletonData::bone_count() const {
    return get<uint32_t>(bone_count_ptr());
}

SkeletonData::ref<uint64_t> SkeletonData::unknown_ptr1() const {
    return get<uint64_t>(40);
}

SkeletonData::ref<uint64_t> SkeletonData::unknown_ptr2() const {
    return get<uint64_t>(104);
}

SkeletonData::ref<uint64_t> SkeletonData::unknown_ptr3() const {
    return get<uint64_t>(112);
}

SkeletonData::ref<uint64_t> SkeletonData::orientations_packet_ptr() const {
    return get<uint64_t>(72);
}

SkeletonData::ref<uint64_t> SkeletonData::end_data_ptr() const {
    return get<uint64_t>(88);
}

std::shared_ptr<StringTable> SkeletonData::bone_names() const {
    return m_bone_names;
}

std::span<BoneHierarchyEntry> SkeletonData::chains() const {
    return m_chains;
}

std::shared_ptr<OrientationData> SkeletonData::orientations() const {
    return m_orientations;
}

SkeletonData::ref<uint64_t> SkeletonData::bone_count_ptr() const {
    return get<uint64_t>(48);
}

SkeletonData::ref<uint64_t> SkeletonData::bone_name_table_ptr() const {
    return get<uint64_t>(64);
}
