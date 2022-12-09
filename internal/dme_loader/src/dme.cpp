#include "dme.h"
#include "bone.h"

#include <spdlog/spdlog.h>

namespace logger = spdlog;
using namespace warpgate;

DME::DME(std::span<uint8_t> subspan, std::string name_): buf_(subspan), name(name_) {
    logger::info("Parsing DME file...");
    parse_dmat();
    parse_meshes();
    parse_bones();
    logger::info("DME file parsed");
}

DME::DME(std::span<uint8_t> subspan, std::string name_, std::shared_ptr<DMAT> dmat): buf_(subspan), name(name_), dmat_(dmat) {
    logger::info("Parsing DME file...");
    parse_meshes();
    parse_bones();
    logger::info("DME file parsed");
}

void DME::parse_dmat() {
    dmat_ = std::make_shared<DMAT>(buf_.subspan(dmat_offset(), dmat_length()));
}

void DME::parse_meshes() {
    uint32_t mesh_count = this->mesh_count();
    meshes_size = 0;
    for(uint32_t i = 0; i < mesh_count; i++) {
        auto mesh = std::make_shared<Mesh>(buf_.subspan(meshes_offset() + 4 + meshes_size));
        meshes_size += mesh->size();
        meshes.push_back(mesh);
    }
    spdlog::info("Loaded {} mesh{}", meshes.size(), meshes.size() != 1 ? "es" : "");
}

void DME::parse_bones() {
    uint32_t count = bone_count();
    uint32_t ivm_offset = bones_offset() + 4;
    uint32_t bbox_offset = ivm_offset + count * sizeof(PackedMat4);
    uint32_t namehash_offset = bbox_offset + count * sizeof(AABB);
    for(uint32_t i = 0; i < count; i++) {
        bones.push_back({get<PackedMat4>(ivm_offset), get<AABB>(bbox_offset), get<uint32_t>(namehash_offset)});
        ivm_offset += sizeof(PackedMat4);
        bbox_offset += sizeof(AABB);
        namehash_offset += sizeof(uint32_t);
    }
    spdlog::info("Loaded {} bones", bones.size());
}

DME::ref<uint32_t> DME::magic() const { return get<uint32_t>(0); }

DME::ref<uint32_t> DME::version() const { return get<uint32_t>(4); }

uint32_t DME::dmat_offset() const { return 12; }

DME::ref<uint32_t> DME::dmat_length() const { return get<uint32_t>(8); }

std::shared_ptr<const DMAT> DME::dmat() const {
    return dmat_;
}

void DME::set_dmat(std::shared_ptr<DMAT> new_dmat) {
    dmat_ = new_dmat;
}

uint32_t DME::aabb_offset() const {
    return dmat_offset() + dmat_length();
}

DME::ref<AABB> DME::aabb() const {
    return get<AABB>(aabb_offset());
}

uint32_t DME::meshes_offset() const {
    return aabb_offset() + sizeof(AABB);
}

DME::ref<uint32_t> DME::mesh_count() const {
    return get<uint32_t>(meshes_offset());
}

std::shared_ptr<const Mesh> DME::mesh(uint32_t index) const {
    return meshes.at(index);
}

uint32_t DME::drawcall_offset() const {
    return meshes_offset() + 4 + (uint32_t)meshes_size;
}

DME::ref<uint32_t> DME::drawcall_count() const {
    return get<uint32_t>(drawcall_offset());
}

std::span<DrawCall> DME::drawcalls() const {
    std::span<uint8_t> data = buf_.subspan(drawcall_offset() + 4, drawcall_count() * sizeof(DrawCall));
    return std::span<DrawCall>(reinterpret_cast<DrawCall*>(data.data()), drawcall_count());
}

uint32_t DME::bonemap_offset() const {
    return drawcall_offset() + 4 + drawcall_count() * sizeof(DrawCall);
}

DME::ref<uint32_t> DME::bme_count() const {
    return get<uint32_t>(bonemap_offset());
}

std::span<BoneMapEntry> DME::bone_map() const {
    std::span<uint8_t> data = buf_.subspan(bonemap_offset() + 4, bme_count() * sizeof(BoneMapEntry));
    return std::span<BoneMapEntry>(reinterpret_cast<BoneMapEntry*>(data.data()), bme_count());
}

uint16_t DME::map_bone(uint16_t global_bone) const {
    if(global_bone > bme_count()) {
        return 0;
    }
    return bone_map()[global_bone].bone_index;
}

uint32_t DME::bones_offset() const {
    return bonemap_offset() + 4 + bme_count() * sizeof(BoneMapEntry);
}

DME::ref<uint32_t> DME::bone_count() const {
    return get<uint32_t>(bones_offset());
}

const Bone DME::bone(uint32_t index) const {
    return bones.at(index);
}
