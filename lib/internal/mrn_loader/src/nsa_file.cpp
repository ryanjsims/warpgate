#include "nsa_file.h"
#include "utils.h"

#include <spdlog/spdlog.h>

using namespace warpgate::mrn;

NSAFile::NSAFile(std::span<uint8_t> subspan) : buf_(subspan) {
    if(static_segment_ptr() != 0x00) {
        m_static_segment = std::make_shared<NSAStaticSegment>(buf_.subspan(static_segment_ptr()));
    }
    if(dynamic_segment_ptr() != 0x00) {
        m_dynamic_segment = std::make_shared<NSADynamicSegment>(buf_.subspan(dynamic_segment_ptr()));
    }
    if(root_segment_ptr() != 0x00) {
        m_root_segment = std::make_shared<NSARootSegment>(buf_.subspan(root_segment_ptr()));
    }
}

uint32_t NSAFile::crc32hash() const {
    uint32_t crc32hash_be = get<uint32_t>(0);
    return swap_endianness(crc32hash_be);
}

NSAFile::ref<uint32_t> NSAFile::version() const {
    return get<uint32_t>(4);
}

NSAFile::ref<uint32_t> NSAFile::static_length() const {
    return get<uint32_t>(16);
}

NSAFile::ref<uint32_t> NSAFile::alignment() const {
    return get<uint32_t>(20);
}

NSAFile::ref<float> NSAFile::duration() const {
    return get<float>(24);
}

NSAFile::ref<float> NSAFile::sample_rate() const {
    return get<float>(28);
}

NSAFile::ref<uint32_t> NSAFile::bone_count() const {
    return get<uint32_t>(32);
}

NSAFile::ref<uint32_t> NSAFile::animated_bone_count() const {
    return get<uint32_t>(36);
}

std::span<uint16_t> NSAFile::static_translation_bone_indices() const {
    uint16_t length = get<uint16_t>(static_translation_indices_ptr());
    return std::span<uint16_t>(
        (uint16_t*)(buf_.data() + static_translation_indices_ptr() + sizeof(uint16_t)),
        length
    );
}

std::span<uint16_t> NSAFile::static_rotation_bone_indices() const {
    uint16_t length = get<uint16_t>(static_rotation_indices_ptr());
    return std::span<uint16_t>(
        (uint16_t*)(buf_.data() + static_rotation_indices_ptr() + sizeof(uint16_t)),
        length
    );
}

std::span<uint16_t> NSAFile::static_scale_bone_indices() const {
    uint16_t length = get<uint16_t>(static_scale_indices_ptr());
    return std::span<uint16_t>(
        (uint16_t*)(buf_.data() + static_scale_indices_ptr() + sizeof(uint16_t)),
        length
    );
}


std::span<uint16_t> NSAFile::dynamic_translation_bone_indices() const {
    uint16_t length = get<uint16_t>(dynamic_translation_indices_ptr());
    return std::span<uint16_t>(
        (uint16_t*)(buf_.data() + dynamic_translation_indices_ptr() + sizeof(uint16_t)),
        length
    );
}

std::span<uint16_t> NSAFile::dynamic_rotation_bone_indices() const {
    uint16_t length = get<uint16_t>(dynamic_rotation_indices_ptr());
    return std::span<uint16_t>(
        (uint16_t*)(buf_.data() + dynamic_rotation_indices_ptr() + sizeof(uint16_t)),
        length
    );
}

std::span<uint16_t> NSAFile::dynamic_scale_bone_indices() const {
    uint16_t length = get<uint16_t>(dynamic_scale_indices_ptr());
    return std::span<uint16_t>(
        (uint16_t*)(buf_.data() + dynamic_scale_indices_ptr() + sizeof(uint16_t)),
        length
    );
}

NSAFile::ref<DequantizationFactors> NSAFile::initial_translation_factors() const {
    return get<DequantizationFactors>(88);
}

NSAFile::ref<uint32_t> NSAFile::dynamic_translation_factors_count() const {
    return get<uint32_t>(120);
}

NSAFile::ref<uint32_t> NSAFile::dynamic_rotation_factors_count() const {
    return get<uint32_t>(124);
}

NSAFile::ref<uint32_t> NSAFile::dynamic_scale_factors_count() const {
    return get<uint32_t>(128);
}

std::span<DequantizationFactors> NSAFile::translation_factors() const {
    return std::span<DequantizationFactors>((DequantizationFactors*)(buf_.data() + translation_factors_ptr()), dynamic_translation_factors_count());
}

std::span<DequantizationFactors> NSAFile::rotation_factors() const {
    return std::span<DequantizationFactors>((DequantizationFactors*)(buf_.data() + rotation_factors_ptr()), dynamic_rotation_factors_count());
}

std::span<DequantizationFactors> NSAFile::scale_factors() const {
    return std::span<DequantizationFactors>((DequantizationFactors*)(buf_.data() + scale_factors_ptr()), dynamic_scale_factors_count());
}

std::shared_ptr<NSAStaticSegment> NSAFile::static_segment() const {
    return m_static_segment;
}

std::shared_ptr<NSADynamicSegment> NSAFile::dynamic_segment() const {
    return m_dynamic_segment;
}

std::shared_ptr<NSARootSegment> NSAFile::root_segment() const {
    return m_root_segment;
}

NSAFile::ref<uint64_t> NSAFile::static_translation_indices_ptr() const {
    return get<uint64_t>(40);
}

NSAFile::ref<uint64_t> NSAFile::static_rotation_indices_ptr() const {
    return get<uint64_t>(48);
}

NSAFile::ref<uint64_t> NSAFile::static_scale_indices_ptr() const {
    return get<uint64_t>(56);
}

NSAFile::ref<uint64_t> NSAFile::dynamic_translation_indices_ptr() const {
    return get<uint64_t>(64);
}

NSAFile::ref<uint64_t> NSAFile::dynamic_rotation_indices_ptr() const {
    return get<uint64_t>(72);
}

NSAFile::ref<uint64_t> NSAFile::dynamic_scale_indices_ptr() const {
    return get<uint64_t>(80);
}

NSAFile::ref<uint64_t> NSAFile::translation_factors_ptr() const {
    return get<uint64_t>(136);
}

NSAFile::ref<uint64_t> NSAFile::rotation_factors_ptr() const {
    return get<uint64_t>(144);
}

NSAFile::ref<uint64_t> NSAFile::scale_factors_ptr() const {
    return get<uint64_t>(152);
}

NSAFile::ref<uint64_t> NSAFile::static_segment_ptr() const {
    return get<uint64_t>(160);
}

NSAFile::ref<uint64_t> NSAFile::dynamic_segment_ptr() const {
    return get<uint64_t>(168);
}

NSAFile::ref<uint64_t> NSAFile::root_segment_ptr() const {
    return get<uint64_t>(176);
}

void NSAFile::dequantize() {
    dequantize_static_segment();
    dequantize_dynamic_segment();
    dequantize_root_segment();
}

std::vector<glm::vec3> NSAFile::static_translation() const {
    return m_static_translation;
}

std::vector<glm::quat> NSAFile::static_rotation() const {
    return m_static_rotation;
}

std::vector<std::vector<glm::vec3>> NSAFile::dynamic_translation() const {
    return m_dynamic_translation;
}

std::vector<std::vector<glm::quat>> NSAFile::dynamic_rotation() const {
    return m_dynamic_rotation;
}

std::vector<glm::vec3> NSAFile::root_translation() const {
    return m_root_translation;
}

std::vector<glm::quat> NSAFile::root_rotation() const {
    return m_root_rotation;
}

void NSAFile::dequantize_static_segment() {
    if(m_static_segment == nullptr) {
        return;
    }
    std::span<glm::u16vec3> quantized_data = m_static_segment->translation_data();
    DequantizationFactors factors = m_static_segment->translation_factors();
    for(uint32_t i = 0; i < m_static_segment->translation_bone_count(); i++) {
        glm::vec3 dequantized = unpack_translation(quantized_data[i], factors);
        m_static_translation.push_back(dequantized);
    }

    quantized_data = m_static_segment->rotation_data();
    factors = m_static_segment->rotation_factors();
    for(uint32_t i = 0; i < m_static_segment->rotation_bone_count(); i++) {
        glm::quat dequantized = unpack_rotation(quantized_data[i], factors);
        m_static_rotation.push_back(dequantized);
    }
}

void NSAFile::dequantize_dynamic_segment() {
    if(m_dynamic_segment == nullptr) {
        return;
    }
    std::vector<std::span<uint32_t>> bitpacked_data = m_dynamic_segment->translation_data();
    std::span<DequantizationInfo> dequantization_info = m_dynamic_segment->translation_dequantization_info();
    std::span<DequantizationFactors> dynamic_factors = this->translation_factors();
    DequantizationFactors init_factors = this->initial_translation_factors(), factors = {};
    for(uint32_t sample_index = 0; sample_index < m_dynamic_segment->sample_count() && m_dynamic_segment->translation_bone_count() > 0; sample_index++) {
        std::vector<glm::vec3> sample;
        for(uint32_t bone = 0; bone < m_dynamic_segment->translation_bone_count(); bone++) {
            for(uint32_t i = 0; i < 3; i++){
                factors.a_min[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_min[i];
                factors.a_scaled_extent[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_scaled_extent[i];
            }
            glm::vec3 dequantized = unpack_translation(bitpacked_data[sample_index][bone], factors);
            
            glm::u16vec3 quantized_init;
            quantized_init.x = dequantization_info[bone].v_init.x;
            quantized_init.y = dequantization_info[bone].v_init.y;
            quantized_init.z = dequantization_info[bone].v_init.z;
            glm::vec3 initial_pos = unpack_translation(quantized_init, init_factors);

            sample.push_back(dequantized + initial_pos);
        }
        if(sample.size() == 0) {
            continue;
        }
        m_dynamic_translation.push_back(sample);
    }

    std::vector<std::span<glm::u16vec3>> rotation_data = m_dynamic_segment->rotation_data();
    dequantization_info = m_dynamic_segment->rotation_dequantization_info();
    dynamic_factors = this->rotation_factors();

    std::vector<glm::quat> initial_rotations;
    for(uint32_t bone = 0; bone < m_dynamic_segment->rotation_bone_count(); bone++) {
        initial_rotations.push_back(unpack_initial_rotation(dequantization_info[bone].v_init));
    }

    for(uint32_t sample_index = 0; sample_index < m_dynamic_segment->sample_count() && m_dynamic_segment->rotation_bone_count() > 0; sample_index++) {
        std::vector<glm::quat> sample;
        for(uint32_t bone = 0; bone < m_dynamic_segment->rotation_bone_count(); bone++) {
            for(uint32_t i = 0; i < 3; i++){
                factors.a_min[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_min[i];
                factors.a_scaled_extent[i] = dynamic_factors[dequantization_info[bone].a_factor_index[i]].a_scaled_extent[i];
            }
            glm::quat dequantized = unpack_rotation(rotation_data[sample_index][bone], factors);

            sample.push_back(initial_rotations[bone] * dequantized);
        }
        if(sample.size() == 0) {
            continue;
        }
        m_dynamic_rotation.push_back(sample);
    }
}

void NSAFile::dequantize_root_segment() {
    if(m_root_segment == nullptr) {
        return;
    }
    std::span<uint32_t> bitpacked_data = m_root_segment->translation_data();
    std::span<glm::u16vec3> rotation_data = m_root_segment->rotation_data();
    DequantizationFactors translation_factors = m_root_segment->translation_factors();
    std::optional<DequantizationFactors> rotation_factors = m_root_segment->rotation_factors();
    for(uint32_t sample_index = 0; sample_index < m_root_segment->sample_count(); sample_index++) {
        if(sample_index < bitpacked_data.size()) {
            m_root_translation.push_back(unpack_translation(bitpacked_data[sample_index], translation_factors));
        }

        if(sample_index < rotation_data.size()) {
            m_root_rotation.push_back(unpack_rotation(rotation_data[sample_index], *rotation_factors));
        }
    }
}



NSAStaticSegment::NSAStaticSegment(std::span<uint8_t> subspan) : buf_(subspan) {
    uint64_t count = 0;
    if(scale_data_ptr() != 0 && translation_data_ptr() != 0) {
        count = scale_data_ptr() - translation_data_ptr() + scale_bone_count() * sizeof(glm::u16vec3);
    } else if(rotation_data_ptr() != 0 && translation_data_ptr() != 0) {
        count = rotation_data_ptr() - translation_data_ptr() + rotation_bone_count() * sizeof(glm::u16vec3);
    } else if(scale_data_ptr() != 0 && rotation_data_ptr() != 0) {
        count = scale_data_ptr() - rotation_data_ptr() + scale_bone_count() * sizeof(glm::u16vec3);
    } else if(translation_data_ptr() != 0) {
        count = translation_bone_count() * sizeof(glm::u16vec3);
    } else if(rotation_data_ptr() != 0) {
        count = rotation_bone_count() * sizeof(glm::u16vec3);
    } else if(scale_data_ptr() != 0) {
        count = scale_bone_count() * sizeof(glm::u16vec3);
    }
    count += 16 - count % 16;
    buf_ = buf_.first(96 + count);
}

NSAStaticSegment::ref<uint32_t> NSAStaticSegment::translation_bone_count() const {
    return get<uint32_t>(0);
}

NSAStaticSegment::ref<uint32_t> NSAStaticSegment::rotation_bone_count() const {
    return get<uint32_t>(4);
}

NSAStaticSegment::ref<uint32_t> NSAStaticSegment::scale_bone_count() const {
    if(get<uint32_t>(8) != 0) {
        spdlog::warn("Scaled poses are present in this animation - stuff will probably break!");
        spdlog::warn("Please raise an issue in https://github.com/ryanjsims/warpgate to get support added - an example file will be needed");
    }
    return get<uint32_t>(8);
}

NSAStaticSegment::ref<DequantizationFactors> NSAStaticSegment::translation_factors() const {
    return get<DequantizationFactors>(12);
}

NSAStaticSegment::ref<DequantizationFactors> NSAStaticSegment::rotation_factors() const {
    return get<DequantizationFactors>(36);
}

std::optional<NSAStaticSegment::ref<DequantizationFactors>> NSAStaticSegment::scale_factors() const {
    if(scale_bone_count() == 0) {
        return {};
    }
    spdlog::warn("Scaled poses are present in this animation - stuff will probably break!");
    spdlog::warn("Please raise an issue in https://github.com/ryanjsims/warpgate to get support added - an example file will be needed");
    return get<DequantizationFactors>(60);
}

std::span<glm::u16vec3> NSAStaticSegment::translation_data() const {
    return std::span<glm::u16vec3>((glm::u16vec3*)(buf_.data() + translation_data_ptr()), translation_bone_count());
}

std::span<glm::u16vec3> NSAStaticSegment::rotation_data() const {
    return std::span<glm::u16vec3>((glm::u16vec3*)(buf_.data() + rotation_data_ptr()), rotation_bone_count());
}

std::span<glm::u16vec3> NSAStaticSegment::scale_data() const {
    return std::span<glm::u16vec3>((glm::u16vec3*)(buf_.data() + scale_data_ptr()), scale_bone_count());
}

NSAStaticSegment::ref<uint64_t> NSAStaticSegment::translation_data_ptr() const {
    return get<uint64_t>(72);
}

NSAStaticSegment::ref<uint64_t> NSAStaticSegment::rotation_data_ptr() const {
    return get<uint64_t>(80);
}

NSAStaticSegment::ref<uint64_t> NSAStaticSegment::scale_data_ptr() const {
    return get<uint64_t>(88);
}

uint32_t next_multiple_of_4(uint32_t value) {
    // There are deq_info sets for every bone, but the number of sets is always a multiple of 4
    // This code simply gets the first multiple of 4 that is greater than or equal to the bone count
    return ((uint32_t)std::ceil(((float)value) / 4)) * 4;
}

NSADynamicSegment::NSADynamicSegment(std::span<uint8_t> subspan) : buf_(subspan) {
    uint32_t _sample_count = sample_count();
    uint32_t sample_padding = rotation_bone_count() > 0 ? (((rotation_dequantization_info_ptr() - rotation_data_ptr()) % (_sample_count * rotation_bone_count() * 2)) / _sample_count) : 0;
    uint32_t _translation_bone_count = translation_bone_count();
    uint32_t _rotation_bone_count = rotation_bone_count();
    uint32_t _scale_bone_count = scale_bone_count();
    uint64_t _translation_data_ptr = translation_data_ptr();
    uint64_t _rotation_data_ptr = rotation_data_ptr();
    uint64_t _scale_data_ptr = scale_data_ptr();
    for(uint32_t i = 0; i < _sample_count; i++) {
        std::span<uint32_t> translation = std::span<uint32_t>(
            (uint32_t*)(buf_.data() + _translation_data_ptr 
                + i * sizeof(uint32_t) * _translation_bone_count), 
            _translation_bone_count
        );
        std::span<glm::u16vec3> rotation = std::span<glm::u16vec3>(
            (glm::u16vec3*)(buf_.data() + _rotation_data_ptr 
                + i * (sizeof(glm::u16vec3) * _rotation_bone_count + sample_padding)), 
            _rotation_bone_count
        );
        std::span<uint32_t> scale = std::span<uint32_t>(
            (uint32_t*)(buf_.data() + _scale_data_ptr 
                + i * sizeof(uint32_t) * _scale_bone_count), 
            _scale_bone_count
        );

        if(translation.size() > 0) {
            m_translation_data.push_back(translation);
        }
        
        if(rotation.size() > 0) {
            m_rotation_data.push_back(rotation);
        }
        
        if(scale.size() > 0) {
            m_scale_data.push_back(scale);
        }
    }
    uint64_t length = (sizeof(uint32_t) * _translation_bone_count 
            + sizeof(glm::u16vec3) * _rotation_bone_count + sample_padding
            + sizeof(uint32_t) * _scale_bone_count
        ) * _sample_count
        + (next_multiple_of_4(_translation_bone_count)
            + next_multiple_of_4(_rotation_bone_count)
            + next_multiple_of_4(_scale_bone_count)
        ) * sizeof(DequantizationInfo);
    length += 16 - length % 16; // align to 16 bytes
    buf_ = buf_.first(64 + length);
}

NSADynamicSegment::ref<uint32_t> NSADynamicSegment::sample_count() const {
    return get<uint32_t>(0);
}

NSADynamicSegment::ref<uint32_t> NSADynamicSegment::translation_bone_count() const {
    return get<uint32_t>(4);
}

NSADynamicSegment::ref<uint32_t> NSADynamicSegment::rotation_bone_count() const {
    return get<uint32_t>(8);
}

NSADynamicSegment::ref<uint32_t> NSADynamicSegment::scale_bone_count() const {
    return get<uint32_t>(12);
}

std::vector<std::span<uint32_t>> NSADynamicSegment::translation_data() const {
    return m_translation_data;
}

std::span<DequantizationInfo> NSADynamicSegment::translation_dequantization_info() const {
    uint32_t count = next_multiple_of_4(translation_bone_count());
    return std::span<DequantizationInfo>((DequantizationInfo*)(buf_.data() + translation_dequantization_info_ptr()), count);
}

std::vector<std::span<glm::u16vec3>> NSADynamicSegment::rotation_data() const {
    return m_rotation_data;
}

std::span<DequantizationInfo> NSADynamicSegment::rotation_dequantization_info() const {
    uint32_t count = next_multiple_of_4(rotation_bone_count());
    return std::span<DequantizationInfo>((DequantizationInfo*)(buf_.data() + rotation_dequantization_info_ptr()), count);
}

std::vector<std::span<uint32_t>> NSADynamicSegment::scale_data() const {
    return m_scale_data;
}

std::span<DequantizationInfo> NSADynamicSegment::scale_dequantization_info() const {
    uint32_t count = next_multiple_of_4(scale_bone_count());
    return std::span<DequantizationInfo>((DequantizationInfo*)(buf_.data() + scale_dequantization_info_ptr()), count);
}

NSADynamicSegment::ref<uint64_t> NSADynamicSegment::translation_data_ptr() const {
    return get<uint64_t>(16);
}

NSADynamicSegment::ref<uint64_t> NSADynamicSegment::translation_dequantization_info_ptr() const {
    return get<uint64_t>(24);
}

NSADynamicSegment::ref<uint64_t> NSADynamicSegment::rotation_data_ptr() const {
    return get<uint64_t>(32);
}

NSADynamicSegment::ref<uint64_t> NSADynamicSegment::rotation_dequantization_info_ptr() const {
    return get<uint64_t>(40);
}

NSADynamicSegment::ref<uint64_t> NSADynamicSegment::scale_data_ptr() const {
    return get<uint64_t>(48);
}

NSADynamicSegment::ref<uint64_t> NSADynamicSegment::scale_dequantization_info_ptr() const {
    return get<uint64_t>(56);
}


NSARootSegment::NSARootSegment(std::span<uint8_t> subspan) : buf_(subspan) {
    buf_ = buf_.first(96 + translation_data().size_bytes() + rotation_data().size_bytes());
}

NSARootSegment::ref<uint32_t> NSARootSegment::version() const {
    return get<uint32_t>(0);
}

NSARootSegment::ref<uint32_t> NSARootSegment::data_length() const {
    return get<uint32_t>(16);
}

NSARootSegment::ref<uint32_t> NSARootSegment::alignment() const {
    return get<uint32_t>(20);
}

NSARootSegment::ref<float> NSARootSegment::sample_rate() const {
    return get<float>(24);
}

NSARootSegment::ref<uint32_t> NSARootSegment::sample_count() const {
    return get<uint32_t>(28);
}

NSARootSegment::ref<DequantizationFactors> NSARootSegment::translation_factors() const {
    return get<DequantizationFactors>(32);
}

std::optional<NSARootSegment::ref<DequantizationFactors>> NSARootSegment::rotation_factors() const {
    if(rotation_data_ptr() == 0) {
        return {};
    }
    return get<DequantizationFactors>(56);
}

std::optional<NSARootSegment::ref<glm::quat>> NSARootSegment::constant_rotation() const {
    if(rotation_data_ptr() != 0) {
        return {};
    }
    return get<glm::quat>(56);
}

std::span<uint32_t> NSARootSegment::translation_data() const {
    if(translation_data_ptr() == 0) {
        return {};
    }
    return std::span<uint32_t>((uint32_t*)(buf_.data() + translation_data_ptr()), sample_count());
}

std::span<glm::u16vec3> NSARootSegment::rotation_data() const {
    if(rotation_data_ptr() == 0) {
        return {};
    }
    return std::span<glm::u16vec3>((glm::u16vec3*)(buf_.data() + rotation_data_ptr()), sample_count());
}

NSARootSegment::ref<uint64_t> NSARootSegment::translation_data_ptr() const {
    return get<uint64_t>(80);
}

NSARootSegment::ref<uint64_t> NSARootSegment::rotation_data_ptr() const {
    return get<uint64_t>(88);
}
