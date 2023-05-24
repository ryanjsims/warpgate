#pragma once

#include "utils/gtk/structs.hpp"
#include <epoxy/gl.h>

#include <filesystem>

namespace warpgate::gtk {
    class Shader {
    public:
        Shader(const std::filesystem::path vertex, const std::filesystem::path fragment);
        ~Shader();

        void use() const;
        bool good() const;
        bool bad() const;


        void set_model(const glm::mat4 &model);
        void set_matrices(const Uniform& ubo);
        void set_planes(const GridUniform& planes);

    private:
        GLuint m_program;
        GLuint m_ubo_matrices;
        GLuint m_ubo_planes;
        bool m_good;

        GLuint create_shader(int type, const char *src);
    };
};