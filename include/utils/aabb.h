#pragma once
#include <array>
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>

namespace warpgate::utils {
    struct AABB {
        AABB(glm::dvec4 min, glm::dvec4 max);
        AABB(double minx, double miny, double minz, double maxx, double maxy, double maxz);

        /**
         * Corner order:
         * x y z
         * - - -
         * - + -
         * + + -
         * + - -
         * + - +
         * - - +
         * - + +
         * + + +
        */
        std::array<glm::dvec4, 8> corners() const;
        glm::dvec4 minimum() const {
            return midpoint_ - half;
        }
        glm::dvec4 maximum() const {
            return midpoint_ + half;
        }
        glm::dvec4 midpoint() const {
            return midpoint_;
        }
        void translate(glm::dvec3 translation);
        // Rotates the AABB around its midpoint, expanding the AABB as needed
        void rotate(glm::dquat rotation);
        bool overlaps(const AABB &other) const;
        bool contains(const glm::dvec3 point) const;

        AABB operator+(const glm::dvec3 translation);
        AABB operator-(const glm::dvec3 translation);
        AABB operator*(const glm::dquat rotation);
        AABB operator*(const glm::dvec3 scale);

    private:
        glm::dvec4 midpoint_, half;
    };
}
