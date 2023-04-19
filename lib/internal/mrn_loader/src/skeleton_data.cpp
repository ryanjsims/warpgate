#include "skeleton_data.h"
#include <spdlog/spdlog.h>

#include "glm/gtc/type_ptr.hpp"

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

std::shared_ptr<Skeleton> SkeletonData::skeleton() {
    if(m_skeleton == nullptr) {
        spdlog::debug("Building skeleton...");
        m_skeleton = std::make_shared<Skeleton>(m_bone_names, m_chains, m_orientations);
    }
    return m_skeleton;
}


Skeleton::Skeleton(
    std::shared_ptr<StringTable> bone_names, 
    std::span<BoneHierarchyEntry> chains, 
    std::shared_ptr<OrientationData> orientations
) {
    for(uint32_t i = 0; i < bone_names->strings().size(); i++) {
        bones.push_back({bone_names->strings()[i], i, 0, orientations->offsets()[i], orientations->rotations()[i], {}});
    }
    for(uint32_t i = 0; i < chains.size(); i++) {
        int32_t parent = chains[i].parent_index;
        for(uint32_t j = 0; j < chains[i].chain_length; j++) {
            if(parent != -1) {
                bones[parent].children.push_back(chains[i].start_index + j);
                bones[chains[i].start_index + j].parent = parent;
            }
            parent = chains[i].start_index + j;
        }
    }

    calculate_transforms(0);
}

void Skeleton::calculate_transforms(uint32_t root_index) {
    for(auto child_index_it = bones[root_index].children.begin(); child_index_it != bones[root_index].children.end(); child_index_it++) {
        glm::mat4 local_translation = glm::translate(glm::identity<glm::mat4>(), bones[*child_index_it].position);
        glm::mat4 local_rotation = glm::toMat4(bones[*child_index_it].rotation);
        bones[*child_index_it].global_transform = bones[root_index].global_transform *  local_translation * local_rotation;
        calculate_transforms(*child_index_it);
    }
}

void Skeleton::log_recursive() {
    log_recursive_impl(0, 0);
}

void Skeleton::log_recursive_impl(uint32_t root_index, uint32_t depth) {
    spdlog::info("{:>{}}", bones[root_index].name, depth * 4 + bones[root_index].name.size());
    for(auto it = bones[root_index].children.begin(); it != bones[root_index].children.end(); it++) {
        log_recursive_impl(*it, depth + 1);
    }
}