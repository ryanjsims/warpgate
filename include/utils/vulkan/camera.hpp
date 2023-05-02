#pragma once

#include "glm/vec4.hpp"
#include "glm/matrix.hpp"
#include "glm/trigonometric.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace warpgate::vulkan {
    class Camera {
    public:
        Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up) : m_position(pos), m_target(target), m_up(up) {
            update_view();
            
            glm::vec3 offset = m_position - m_target;
            float radius = glm::length(glm::vec2{offset.x, offset.z});
            m_yaw = glm::asin(offset.z / radius);
            radius = glm::length(offset);
            m_pitch = glm::asin(offset.y / radius);
        }

        glm::mat4 get_view() {
            return m_view;
        }

        glm::vec3 get_pos() {
            return m_position;
        }

        void rotate_about_target(float dyaw, float dpitch) {
            m_yaw_temp = m_yaw + glm::radians(dyaw);
            m_pitch_temp = m_pitch + glm::radians(dpitch);
            
            glm::vec3 offset = m_position - m_target;
            float radius = glm::length(offset);
            offset.x = glm::cos(m_yaw_temp) * glm::cos(m_pitch_temp) * radius;
            offset.y = glm::sin(m_pitch_temp) * radius;
            offset.z = glm::sin(m_yaw_temp) * glm::cos(m_pitch_temp) * radius;

            m_position = offset + m_target;
            update_view();
        }

        void commit_rotation() {
            m_yaw = m_yaw_temp;
            m_pitch = m_pitch_temp;
        }

        void pan(glm::vec3 translation) {
            m_target += translation;
            m_position += translation;

            update_view();

            m_target_temp = m_target;
            m_target -= translation;

            m_position_temp = m_position;
            m_position -= translation;
        }

        void commit_pan() {
            m_target = m_target_temp;
            m_position = m_position_temp;
        }

        void translate(glm::vec3 translation) {
            m_position += translation;

            update_view();

            m_position_temp = m_position;
            m_position -= translation;
        }

        void commit_translate() {
            m_position = m_position_temp;
        }

        glm::vec3 right() {
            return m_cameraRight;
        }

        glm::vec3 up() {
            return m_cameraUp;
        }

        glm::vec3 forward() {
            return m_direction;
        }

        float radius() {
            return glm::length(m_position - m_target);
        }

    private:
        glm::vec3 m_position, m_target, m_up, m_cameraRight, m_cameraUp, m_direction, m_target_temp, m_position_temp;
        glm::mat4 m_view;
        float m_fov, m_near, m_far;
        float m_yaw, m_pitch, m_yaw_temp, m_pitch_temp;

        void update_view() {
            glm::vec3 offset = m_position - m_target;
            m_direction = glm::normalize(offset);
            m_cameraRight = glm::normalize(glm::cross(m_up, m_direction));
            m_cameraUp = glm::normalize(glm::cross(m_direction, m_cameraRight));

            m_view = glm::lookAt(m_position, m_target, m_cameraUp);
        }
    };
}