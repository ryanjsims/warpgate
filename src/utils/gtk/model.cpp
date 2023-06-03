#include "utils/gtk/model.hpp"

#include <utils/materials_3.h>
#include <glm/gtx/string_cast.hpp>

using namespace warpgate::gtk;

Model::Model(
    std::string name,
    std::shared_ptr<warpgate::DME> dme,
    std::unordered_map<uint32_t, std::shared_ptr<Shader>> &shaders,
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
    std::shared_ptr<synthium::Manager> manager,
    GLuint matrices_uniform,
    GLuint planes_uniform
) {
    m_name = name;
    m_uname.assign(name.c_str());
    m_model = glm::identity<glm::mat4>();
    std::shared_ptr<const warpgate::DMAT> dmat = dme->dmat();
    spdlog::debug("Creating model '{}'", m_name);
    for(uint32_t mesh_index = 0; mesh_index < dme->mesh_count(); mesh_index++) {
        uint32_t material_definition = dmat->material(mesh_index)->definition();
        std::string layout_name = utils::materials3::materials["materialDefinitions"][std::to_string(material_definition)]["drawStyles"][0]["inputLayout"];
        
        spdlog::debug("    Creating mesh with layout '{}'", layout_name);
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(
            dme->mesh(mesh_index),
            dmat->material(mesh_index),
            utils::materials3::materials["inputLayouts"][layout_name],
            textures,
            manager
        );
        
        if(shaders.find(mesh->get_material_hash()) == shaders.end()) {
            spdlog::debug("    Creating shader with layout '{}'", layout_name);
            auto[vertex, fragment] = mesh->get_shader_paths();
            std::shared_ptr<Shader> shader = std::make_shared<Shader>(vertex, fragment, matrices_uniform, planes_uniform);
            if(shader->bad()) {
                continue;
            }
            shaders[mesh->get_material_hash()] = shader;
        }
        m_meshes.push_back(mesh);
    }
    spdlog::debug("Done");
}

Model::~Model() {
    spdlog::debug("Destroying model '{}'", m_name);
    m_meshes.clear();
}

void Model::draw(
    std::unordered_map<uint32_t, std::shared_ptr<Shader>> &shaders,
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures
) const {
    if(m_meshes.size() > 0) {
        shaders[m_meshes[0]->get_material_hash()]->set_model(m_model);
    }
    for(uint32_t i = 0; i < m_meshes.size(); i++) {
        if(shaders.find(m_meshes[i]->get_material_hash()) == shaders.end()) {
            continue;
        }
        shaders[m_meshes[i]->get_material_hash()]->use();
        m_meshes[i]->bind();
        std::vector<uint32_t> texture_hashes = m_meshes[i]->get_texture_hashes();
        for(uint32_t j = 0; j < texture_hashes.size(); j++) {
            if(textures.find(texture_hashes[j]) == textures.end()) {
                continue;
            }
            textures[texture_hashes[j]]->bind();
        }

        glDrawElements(GL_TRIANGLES, m_meshes[i]->index_count(), m_meshes[i]->index_size() == 4 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, 0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

Glib::ustring Model::uname() const {
    return m_uname;
}

std::string Model::name() const {
    return m_name;
}

std::shared_ptr<const Mesh> Model::mesh(uint32_t index) const {
    return m_meshes[index];
}

size_t Model::mesh_count() const {
    return m_meshes.size();
}