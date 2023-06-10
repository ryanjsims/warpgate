#pragma once
#include <glm/fwd.hpp>

namespace warpgate::utils {
    double sign(glm::dquat quaternion);
    float sign(glm::quat quaternion);
}