#include "utils/sign.h"

#include <cmath>
#include <glm/gtx/quaternion.hpp>

using namespace warpgate;

double utils::sign(glm::dquat quaternion) {
    return (std::signbit(quaternion.x) ? -1.0 : 1.0)
            * (std::signbit(quaternion.y) ? -1.0 : 1.0)
            * (std::signbit(quaternion.z) ? -1.0 : 1.0)
            * (std::signbit(quaternion.w) ? -1.0 : 1.0);
}

float utils::sign(glm::quat quaternion) {
    return (   std::signbit(quaternion.x) ? -1.0f : 1.0f)
            * (std::signbit(quaternion.y) ? -1.0f : 1.0f)
            * (std::signbit(quaternion.z) ? -1.0f : 1.0f)
            * (std::signbit(quaternion.w) ? -1.0f : 1.0f);
}