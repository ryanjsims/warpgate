#pragma once
#include <cstring>
#include <stdexcept>
#include <optional>
#include <memory>
#include <span>
#include <vector>

#include "structs.h"

namespace warpgate::mrn {
    struct NSAStaticSegment {
        mutable std::span<uint8_t> buf_;

        NSAStaticSegment(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("NSAStaticSegment: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> translation_bone_count() const;
        ref<uint32_t> rotation_bone_count() const;
        ref<uint32_t> scale_bone_count() const;

        ref<DequantizationFactors> translation_factors() const;
        ref<DequantizationFactors> rotation_factors() const;
        std::optional<ref<DequantizationFactors>> scale_factors() const;

        std::span<glm::u16vec3> translation_data() const;
        std::span<glm::u16vec3> rotation_data() const;
        std::span<glm::u16vec3> scale_data() const;

    private:
        ref<uint64_t> translation_data_ptr() const;
        ref<uint64_t> rotation_data_ptr() const;
        ref<uint64_t> scale_data_ptr() const;
    };

    struct NSADynamicSegment {
        mutable std::span<uint8_t> buf_;

        NSADynamicSegment(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("NSADynamicSegment: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> sample_count() const;
        ref<uint32_t> translation_bone_count() const;
        ref<uint32_t> rotation_bone_count() const;
        ref<uint32_t> scale_bone_count() const;

        std::vector<std::span<uint32_t>> translation_data() const;
        std::span<DequantizationInfo> translation_dequantization_info() const;

        std::vector<std::span<glm::u16vec3>> rotation_data() const;
        std::span<DequantizationInfo> rotation_dequantization_info() const;

        std::vector<std::span<uint32_t>> scale_data() const;
        std::span<DequantizationInfo> scale_dequantization_info() const;
    private:
        std::vector<std::span<uint32_t>> m_translation_data, m_scale_data;
        std::vector<std::span<glm::u16vec3>> m_rotation_data;

        ref<uint64_t> translation_data_ptr() const;
        ref<uint64_t> translation_dequantization_info_ptr() const;

        ref<uint64_t> rotation_data_ptr() const;
        ref<uint64_t> rotation_dequantization_info_ptr() const;

        ref<uint64_t> scale_data_ptr() const;
        ref<uint64_t> scale_dequantization_info_ptr() const;
    };

    struct NSARootSegment {
        mutable std::span<uint8_t> buf_;

        NSARootSegment(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("NSARootSegment: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint32_t> version() const;
        ref<uint32_t> data_length() const;
        ref<uint32_t> alignment() const;
        ref<float> sample_rate() const;
        ref<uint32_t> sample_count() const;

        ref<DequantizationFactors> translation_factors() const;
        std::optional<ref<DequantizationFactors>> rotation_factors() const;
        std::optional<ref<glm::quat>> constant_rotation() const;

        std::span<uint32_t> translation_data() const;
        std::span<glm::u16vec3> rotation_data() const;
    
    private:
        ref<uint64_t> translation_data_ptr() const;
        ref<uint64_t> rotation_data_ptr() const;

    };

    struct NSAFile {
        mutable std::span<uint8_t> buf_;

        NSAFile(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("NSAFile: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        uint32_t crc32hash() const;
        ref<uint32_t> version() const;
        ref<uint32_t> static_length() const;
        ref<uint32_t> alignment() const;
        ref<float> duration() const;
        ref<float> sample_rate() const;
        ref<uint32_t> bone_count() const;
        ref<uint32_t> animated_bone_count() const;

        std::span<uint16_t> static_translation_bone_indices() const;
        std::span<uint16_t> static_rotation_bone_indices() const;
        std::span<uint16_t> static_scale_bone_indices() const;

        std::span<uint16_t> dynamic_translation_bone_indices() const;
        std::span<uint16_t> dynamic_rotation_bone_indices() const;
        std::span<uint16_t> dynamic_scale_bone_indices() const;

        ref<DequantizationFactors> initial_translation_factors() const;
        ref<uint32_t> dynamic_translation_factors_count() const;
        ref<uint32_t> dynamic_rotation_factors_count() const;
        ref<uint32_t> dynamic_scale_factors_count() const;

        std::span<DequantizationFactors> translation_factors() const;
        std::span<DequantizationFactors> rotation_factors() const;
        std::span<DequantizationFactors> scale_factors() const;

        std::shared_ptr<NSAStaticSegment> static_segment() const;
        std::shared_ptr<NSADynamicSegment> dynamic_segment() const;
        std::shared_ptr<NSARootSegment> root_segment() const;

        void dequantize();

        std::vector<glm::vec3> static_translation() const;
        std::vector<glm::quat> static_rotation() const;

        std::vector<std::vector<glm::vec3>> dynamic_translation() const;
        std::vector<std::vector<glm::quat>> dynamic_rotation() const;

        std::vector<glm::vec3> root_translation() const;
        std::vector<glm::quat> root_rotation() const;
    
    private:
        std::shared_ptr<NSAStaticSegment> m_static_segment;
        std::shared_ptr<NSADynamicSegment> m_dynamic_segment;
        std::shared_ptr<NSARootSegment> m_root_segment;

        std::vector<glm::vec3> m_static_translation;
        std::vector<glm::quat> m_static_rotation;
        std::vector<std::vector<glm::vec3>> m_dynamic_translation;
        std::vector<std::vector<glm::quat>> m_dynamic_rotation;
        std::vector<glm::vec3> m_root_translation;
        std::vector<glm::quat> m_root_rotation;

        void dequantize_static_segment();
        void dequantize_dynamic_segment();
        void dequantize_root_segment();

        ref<uint64_t> static_translation_indices_ptr() const;
        ref<uint64_t> static_rotation_indices_ptr() const;
        ref<uint64_t> static_scale_indices_ptr() const;

        ref<uint64_t> dynamic_translation_indices_ptr() const;
        ref<uint64_t> dynamic_rotation_indices_ptr() const;
        ref<uint64_t> dynamic_scale_indices_ptr() const;

        ref<uint64_t> translation_factors_ptr() const;
        ref<uint64_t> rotation_factors_ptr() const;
        ref<uint64_t> scale_factors_ptr() const;

        ref<uint64_t> static_segment_ptr() const;
        ref<uint64_t> dynamic_segment_ptr() const;
        ref<uint64_t> root_segment_ptr() const;
    };
}