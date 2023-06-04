#pragma once

#include "utils/gtk/bone.hpp"

#include <memory>

namespace warpgate::gtk {
    class Skeleton {
    public:
        Skeleton();
        ~Skeleton();
    private:
        std::shared_ptr<Bone> m_root;
    };
}