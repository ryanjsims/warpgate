#pragma once

#include "utils/gtk/shader.hpp"

#include <epoxy/gl.h>
#include <glm/matrix.hpp>

#include <memory>
#include <vector>

namespace warpgate::gtk {
    class Bone {
    public:
        Bone(glm::mat4 matrix);
        ~Bone();

        void draw();

        glm::mat4 get_bind_matrix();
        void set_bind_matrix(glm::mat4 matrix);

        static void init(GLuint matrices_uniform);
        static GLuint vao;
        static GLuint vertices;
        static GLuint indices;
        static std::shared_ptr<Shader> program;
    private:
        std::vector<std::shared_ptr<Bone>> m_children;
        glm::mat4 m_bind_matrix;
    };
}