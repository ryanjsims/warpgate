#pragma once

#include "utils/gtk/bone.hpp"
#include <dme_loader.h>
#include <skeleton_data.h>

#include <memory>
#include <vector>

namespace warpgate::gtk {
    class Skeleton {
    public:
        Skeleton(std::shared_ptr<DME> dme);
        Skeleton(std::shared_ptr<mrn::SkeletonData> skeleton_data);
        ~Skeleton();

        void draw();
    private:
        void reorient(std::shared_ptr<Bone> root);
        void update_global_transform(std::shared_ptr<Bone> root);
        std::vector<std::shared_ptr<Bone>> m_roots;
        std::vector<std::shared_ptr<Bone>> m_skeleton;
    };
}