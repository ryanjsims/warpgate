#pragma once

#include <glm/matrix.hpp>

namespace warpgate::gtk {
    struct Uniform {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 invProjectionMatrix;
        glm::mat4 invViewMatrix;
        uint32_t faction = 0;
        uint32_t pad[3] = {0, 0, 0};
    };

    struct GridUniform {
        float near_plane, far_plane;
        uint32_t pad[2] = {0, 0};
    };

    struct Rect {
        int x, y, width, height;
    };
};