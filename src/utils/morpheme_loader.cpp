#include "utils/morpheme_loader.h"

using namespace warpgate::utils;
using namespace warpgate::mrn;

MorphemeLoader::MorphemeLoader(std::shared_ptr<mrn::NSAFile> animation) : m_animation(animation) {
    dequantize_static_segment();
    dequantize_dynamic_segment();
    dequantize_root_segment();
}

void MorphemeLoader::dequantize_static_segment() {
    std::span<Vector3Short> quantized_data = m_animation->static_segment()->translation_data();
    DequantizationFactors factors = m_animation->static_segment()->translation_factors();
    for(uint32_t i = 0; i < m_animation->static_segment()->translation_bone_count(); i++) {
        Vector3 dequantized = unpack_translation(quantized_data[i], factors);
        m_static_translation.push_back(dequantized);
    }

    quantized_data = m_animation->static_segment()->rotation_data();
    factors = m_animation->static_segment()->rotation_factors();
    for(uint32_t i = 0; i < m_animation->static_segment()->rotation_bone_count(); i++) {
        Quaternion dequantized = unpack_rotation(quantized_data[i], factors);
        m_static_rotation.push_back(dequantized);
    }
}

void MorphemeLoader::dequantize_dynamic_segment() {
    std::vector<std::span<uint32_t>> bitpacked_data = m_animation->dynamic_segment()->translation_data();
    std::span<DequantizationInfo> dequantization_info = m_animation->dynamic_segment()->translation_dequantization_info();
    std::span<DequantizationFactors> dynamic_factors = m_animation->translation_factors();
    DequantizationFactors init_factors = m_animation->initial_translation_factors(), factors = {};
    for(uint32_t sample_index = 0; sample_index < m_animation->dynamic_segment()->sample_count() && m_animation->dynamic_segment()->translation_bone_count() > 0; sample_index++) {
        std::vector<Vector3> sample;
        for(uint32_t bone = 0; bone < m_animation->dynamic_segment()->translation_bone_count(); bone++) {
            for(uint32_t i = 0; i < 3; i++){
                factors.a_min[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_min[i];
                factors.a_scaled_extent[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_scaled_extent[i];
            }
            Vector3 dequantized = unpack_translation(bitpacked_data[sample_index][bone], factors);
            
            Vector3Short quantized_init;
            quantized_init.x = dequantization_info[bone].v_init.x;
            quantized_init.y = dequantization_info[bone].v_init.y;
            quantized_init.z = dequantization_info[bone].v_init.z;
            Vector3 initial_pos = unpack_translation(quantized_init, init_factors);

            sample.push_back(dequantized + initial_pos);
        }
        if(sample.size() == 0) {
            continue;
        }
        m_dynamic_translation.push_back(sample);
    }

    std::vector<std::span<Vector3Short>> rotation_data = m_animation->dynamic_segment()->rotation_data();
    dequantization_info = m_animation->dynamic_segment()->rotation_dequantization_info();
    dynamic_factors = m_animation->rotation_factors();

    std::vector<Quaternion> initial_rotations;
    for(uint32_t bone = 0; bone < m_animation->dynamic_segment()->rotation_bone_count(); bone++) {
        initial_rotations.push_back(unpack_initial_rotation(dequantization_info[bone].v_init));
    }

    for(uint32_t sample_index = 0; sample_index < m_animation->dynamic_segment()->sample_count() && m_animation->dynamic_segment()->rotation_bone_count() > 0; sample_index++) {
        std::vector<Quaternion> sample;
        for(uint32_t bone = 0; bone < m_animation->dynamic_segment()->rotation_bone_count(); bone++) {
            for(uint32_t i = 0; i < 3; i++){
                factors.a_min[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_min[i];
                factors.a_scaled_extent[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_scaled_extent[i];
            }
            Quaternion dequantized = unpack_rotation(rotation_data[sample_index][bone], factors);

            sample.push_back(initial_rotations[bone] * dequantized);
        }
        if(sample.size() == 0) {
            continue;
        }
        m_dynamic_rotation.push_back(sample);
    }
}

void MorphemeLoader::dequantize_root_segment() {
    std::span<uint32_t> bitpacked_data = m_animation->root_segment()->translation_data();
    std::span<Vector3Short> rotation_data = m_animation->root_segment()->rotation_data();
    DequantizationFactors translation_factors = m_animation->root_segment()->translation_factors();
    std::optional<DequantizationFactors> rotation_factors = m_animation->root_segment()->rotation_factors();
    for(uint32_t sample_index = 0; sample_index < m_animation->root_segment()->sample_count(); sample_index++) {
        if(sample_index < bitpacked_data.size()) {
            m_root_translation.push_back(unpack_translation(bitpacked_data[sample_index], translation_factors));
        }

        if(sample_index < rotation_data.size()) {
            m_root_rotation.push_back(unpack_rotation(rotation_data[sample_index], *rotation_factors));
        }
    }
}

std::vector<Vector3> MorphemeLoader::static_translation() const {
    return m_static_translation;
}

std::vector<Quaternion> MorphemeLoader::static_rotation() const {
    return m_static_rotation;
}

std::vector<std::vector<Vector3>> MorphemeLoader::dynamic_translation() const {
    return m_dynamic_translation;
}

std::vector<std::vector<Quaternion>> MorphemeLoader::dynamic_rotation() const {
    return m_dynamic_rotation;
}


std::vector<Vector3> MorphemeLoader::root_translation() const {
    return m_root_translation;
}

std::vector<Quaternion> MorphemeLoader::root_rotation() const {
    return m_root_rotation;
}
