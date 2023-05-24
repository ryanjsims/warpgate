#pragma once

#include "utils/gtk/mesh.hpp"
#include "utils/gtk/texture.hpp"
#include "utils/gtk/shader.hpp"

#include <dme_loader.h>
#include <synthium/manager.h>

#include <memory>
#include <vector>
#include <unordered_map>

#include <glibmm/ustring.h>

namespace warpgate::gtk {
    class Model {
    public:
        Model(
            std::string name,
            std::shared_ptr<warpgate::DME> dme,
            std::unordered_map<uint32_t, std::shared_ptr<Shader>> &shaders,
            std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
            std::shared_ptr<synthium::Manager> manager
        );
        ~Model();

        void draw(
            std::unordered_map<uint32_t, std::shared_ptr<Shader>> &shaders,
            std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
            const Uniform &ubo,
            const GridUniform &planes
        ) const;
        Glib::ustring uname() const;
        std::string name() const;
        std::shared_ptr<const Mesh> mesh(uint32_t index) const;
        size_t mesh_count() const;
    private:
        Glib::ustring m_uname;
        std::string m_name;
        std::vector<std::shared_ptr<Mesh>> m_meshes;
        glm::mat4 m_model;
    };
};