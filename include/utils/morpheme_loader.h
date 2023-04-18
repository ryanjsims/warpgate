#pragma once

#include <memory>
#include <span>
#include <vector>

#include "nsa_file.h"
#include "utils.h"

namespace warpgate::utils {
    class MorphemeLoader {
        MorphemeLoader(std::shared_ptr<mrn::NSAFile> animation);

        std::vector<Vector3> static_translation() const;
        std::vector<Quaternion> static_rotation() const;

        std::vector<std::vector<Vector3>> dynamic_translation() const;
        std::vector<std::vector<Quaternion>> dynamic_rotation() const;

        std::vector<Vector3> root_translation() const;
        std::vector<Quaternion> root_rotation() const;
    
    private:
        void dequantize_static_segment();
        void dequantize_dynamic_segment();
        void dequantize_root_segment();

        std::shared_ptr<mrn::NSAFile> m_animation;
        std::vector<Vector3> m_static_translation;
        std::vector<Quaternion> m_static_rotation;
        std::vector<std::vector<Vector3>> m_dynamic_translation;
        std::vector<std::vector<Quaternion>> m_dynamic_rotation;
        std::vector<Vector3> m_root_translation;
        std::vector<Quaternion> m_root_rotation;
    };
}