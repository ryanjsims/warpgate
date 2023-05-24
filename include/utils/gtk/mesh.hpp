#pragma once

#include "utils/gtk/texture.hpp"

#include <dme_loader.h>
#include <json.hpp>
#include <synthium/manager.h>

#include <filesystem>
#include <memory>
#include <vector>

#include <epoxy/gl.h>

namespace warpgate::gtk {
    class Mesh {
    public:
        Mesh(
            std::shared_ptr<const warpgate::Mesh> mesh,
            std::shared_ptr<const warpgate::Material> material,
            nlohmann::json layout,
            std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
            std::shared_ptr<synthium::Manager> manager
        );
        ~Mesh();

        std::pair<std::filesystem::path, std::filesystem::path> get_shader_paths() const;
        uint32_t get_material_hash() const;
        std::vector<uint32_t> get_texture_hashes() const;

        void bind() const;
        void unbind() const;
        uint32_t index_count() const;
        uint32_t index_size() const;

    private:
        struct Layout {
            uint32_t vs_index;
            GLuint pointer;
            GLint count;
            GLenum type;
            GLboolean normalized;
            GLsizei stride;
            void* offset;
        };

        GLuint vao;
        std::vector<GLuint> vertex_streams;
        std::vector<Layout> vertex_layouts;
        GLuint indices;
        uint32_t m_index_count, m_index_size, material_hash;
        std::vector<uint32_t> m_texture_hashes;
        std::string material_name;
    };
};