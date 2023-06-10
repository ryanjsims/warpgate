#pragma once

#include "utils/gtk/shader.hpp"

#include <epoxy/gl.h>
#include <glm/matrix.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <vector>
#include <array>

namespace warpgate::gtk {
    class Bone {
    friend class Skeleton;
    public:
        Bone(std::string name, glm::mat4 matrix);
        ~Bone();

        void draw_individual();
        void draw();

        glm::mat4 get_inverse_bind_matrix();
        void set_inverse_bind_matrix(glm::mat4 matrix);

        void add_child(std::shared_ptr<Bone> child);
        void set_parent(std::shared_ptr<Bone> parent);
        std::shared_ptr<Bone> parent();
        std::vector<std::shared_ptr<Bone>> children();

        std::string name() const;

        glm::vec3 get_position() const;
        glm::vec3 get_direction() const;
        glm::quat get_rotation() const;
        void update_tail();

        static void init(GLuint matrices_uniform);
        static GLuint vao;
        static GLuint vertices;
        static GLuint indices;
        static std::shared_ptr<Shader> program;

        static const std::array<float, 18> bone_vertices;

    private:
        std::vector<std::shared_ptr<Bone>> m_children;
        std::shared_ptr<Bone> m_parent = nullptr;
        glm::mat4 m_inverse_bind_matrix;
        glm::mat4 m_global_transform_matrix;
        glm::vec3 m_translation, m_offset;
        glm::quat m_rotation, m_pointing_rotation;
        float m_length;
        std::string m_name;
    };
}