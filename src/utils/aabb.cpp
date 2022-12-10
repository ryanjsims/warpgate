#include "utils/aabb.h"
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>

using namespace warpgate::utils;

AABB::AABB(glm::dvec4 min, glm::dvec4 max) {
    if(min.x > max.x || min.y > max.y || min.z > max.z) {
        throw std::invalid_argument("min > max on at least one axis");
    }
    midpoint_ = (min + max) * 0.5;
    half = max - midpoint_;
}

AABB::AABB(
    double minx, double miny, double minz, 
    double maxx, double maxy, double maxz
) {
    glm::dvec4 min(minx, miny, minz, 1.0), max(maxx, maxy, maxz, 1.0);
    if(min.x > max.x || min.y > max.y || min.z > max.z) {
        throw std::invalid_argument("min > max on at least one axis");
    }
    midpoint_ = (min + max) * 0.5;
    half = max - midpoint_;
}

std::array<glm::dvec4, 8> AABB::corners() const {
    glm::dvec4 min = midpoint_ - half, max = midpoint_ + half;
    return {
        min,
        {min.x, max.y, min.z, 1.0},
        {max.x, max.y, min.z, 1.0},
        {max.x, min.y, min.z, 1.0},
        {max.x, min.y, max.z, 1.0},
        {min.x, min.y, max.z, 1.0},
        {min.x, max.y, max.z, 1.0},
        max
    };
}

void AABB::translate(glm::dvec3 translation) {
    midpoint_ = midpoint_ + glm::dvec4{translation, 0.0};
}

void AABB::rotate(glm::dquat rotation) {
    for(glm::dvec4 corner : corners()) {
        glm::dvec4 rotated = rotation * (corner - midpoint_);
        if(rotated.x > half.x) {
            half.x = rotated.x;
        }
        if(rotated.y > half.y) {
            half.y = rotated.y;
        }
        if(rotated.z > half.z) {
            half.z = rotated.z;
        }
    }
}

bool AABB::overlaps(const AABB &other) const {
    double dx = other.midpoint_.x - this->midpoint_.x;
    double px = (other.half.x + half.x) - std::fabs(dx);
    if(px <= 0) {
        return false;
    }

    double dy = other.midpoint_.y - this->midpoint_.y;
    double py = (other.half.y + half.y) - std::fabs(dy);
    if(py <= 0) {
        return false;
    }

    double dz = other.midpoint_.z - this->midpoint_.z;
    double pz = (other.half.z + half.z) - std::fabs(dz);
    if(pz <= 0) {
        return false;
    }

    return true;
}

AABB AABB::operator+(const glm::dvec3 translation) {
    glm::dvec4 min = midpoint_ - half, max = midpoint_ + half;
    return AABB(min + glm::dvec4{translation, 0.0}, max + glm::dvec4{translation, 0.0});
}

AABB AABB::operator-(const glm::dvec3 translation) {
    glm::dvec4 min = midpoint_ - half, max = midpoint_ + half;
    return AABB(min - glm::dvec4{translation, 0.0}, max - glm::dvec4{translation, 0.0});
}

AABB AABB::operator*(const glm::dquat rotation) {
    glm::dvec4 new_half = half;
    for(glm::dvec4 corner : corners()) {
        glm::dvec4 rotated = rotation * (corner - midpoint_);
        if(rotated.x > new_half.x) {
            new_half.x = rotated.x;
        }
        if(rotated.y > new_half.y) {
            new_half.y = rotated.y;
        }
        if(rotated.z > new_half.z) {
            new_half.z = rotated.z;
        }
    }
    glm::dvec4 min = midpoint_ - new_half;
    glm::dvec4 max = midpoint_ + new_half;
    return AABB(min, max);
}

AABB AABB::operator*(const glm::dvec3 scale) {
    glm::dvec4 new_half = half;
    new_half.x *= scale.x;
    new_half.y *= scale.y;
    new_half.z *= scale.z;
    glm::dvec4 min = midpoint_ - new_half;
    glm::dvec4 max = midpoint_ + new_half;
    return AABB(min, max);
}