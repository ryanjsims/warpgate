#pragma once
#include <stdexcept>
#include <memory>
#include <span>
#include <vector>

#include "structs.h"
#include "dmat.h"
#include "mesh.h"

namespace warpgate {
    struct Bone;

    struct DME {
        mutable std::span<uint8_t> buf_;

        DME(std::span<uint8_t> subspan, std::string name);
        DME(std::span<uint8_t> subspan, std::string name, std::shared_ptr<DMAT> dmat);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("DME: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        const std::string &get_name() const {
            return name;
        }

        ref<uint32_t> magic() const;
        ref<uint32_t> version() const;

        ref<uint32_t> dmat_length() const;
        std::shared_ptr<const DMAT> dmat() const;
        void set_dmat(std::shared_ptr<DMAT> new_dmat);

        ref<AABB> aabb() const;

        ref<uint32_t> mesh_count() const;
        std::shared_ptr<const Mesh> mesh(uint32_t index) const;

        ref<uint32_t> drawcall_count() const;
        std::span<DrawCall> drawcalls() const;

        ref<uint32_t> bme_count() const;
        std::span<BoneMapEntry> bone_map() const;

        uint16_t map_bone(uint16_t local_bone) const;

        ref<uint32_t> bone_count() const;
        const Bone bone(uint32_t index) const;


    private:
        std::shared_ptr<DMAT> dmat_ = nullptr;
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<Bone> bones;
        std::string name;
        size_t meshes_size;

        uint32_t aabb_offset() const;
        uint32_t bonemap_offset() const;
        uint32_t bones_offset() const;
        uint32_t dmat_offset() const;
        uint32_t drawcall_offset() const;
        uint32_t meshes_offset() const;
        void parse_meshes();
        void parse_dmat();
        void parse_bones();
    };
}