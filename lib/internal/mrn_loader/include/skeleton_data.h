#pragma once
#include <cstring>
#include <stdexcept>
#include <memory>
#include <span>

#include "string_table.h"
#include "structs.h"

namespace warpgate::mrn {
    struct OrientationData {
        mutable std::span<uint8_t> buf_;

        OrientationData(std::span<uint8_t> subspan, size_t bone_count);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("OrientationData: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> data_length() const;
        ref<uint32_t> alignment() const;

        ref<uint32_t> element_count() const;
        ref<bool> full() const;
        ref<uint32_t> element_type() const; // not sure
        ref<uint64_t> unknown_ptr1() const;
        ref<uint64_t> unknown_ptr2() const;

        std::span<glm::vec4> offsets();
        std::span<glm::quat> rotations();

    private:
        std::span<glm::vec4> m_offsets;
        std::span<glm::quat> m_rotations;
        ref<uint64_t> element_type_ptr() const;
        ref<uint64_t> transforms_ptr() const;
        uint64_t transforms_ptr_base() const {
            return 16;
        }
    };

    struct Bone {
        std::string name;
        uint32_t index;
        glm::vec3 position;
        glm::quat rotation;
        std::vector<uint32_t> children;
        glm::mat4 global_transform = glm::identity<glm::mat4>();
    };

    struct Skeleton {
        Skeleton(std::shared_ptr<StringTable> bone_names, std::span<BoneHierarchyEntry> chains, std::shared_ptr<OrientationData> orientations);
        std::vector<Bone> bones;
    };

    struct SkeletonData {
        mutable std::span<uint8_t> buf_;

        SkeletonData(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("SkeletonData: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<glm::quat> rotation() const;
        ref<glm::vec4> position() const;
        ref<uint32_t> chain_count() const;
        ref<uint32_t> bone_count() const;

        ref<uint64_t> unknown_ptr1() const;
        ref<uint64_t> unknown_ptr2() const;
        ref<uint64_t> unknown_ptr3() const;
        ref<uint64_t> end_data_ptr() const;

        std::shared_ptr<StringTable> bone_names() const;
        std::span<BoneHierarchyEntry> chains() const;
        std::shared_ptr<OrientationData> orientations() const;

    private:
        std::shared_ptr<StringTable> m_bone_names;
        std::shared_ptr<OrientationData> m_orientations;
        std::span<BoneHierarchyEntry> m_chains;

        ref<uint64_t> bone_count_ptr() const;
        ref<uint64_t> bone_name_table_ptr() const;
        ref<uint64_t> orientations_packet_ptr() const;
        uint64_t chains_offset() const {
            return 128;
        }
    };
}