#include "utils/gtk/bone.hpp"
#include "utils/common.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace warpgate::gtk;

GLuint Bone::vao = 0;
GLuint Bone::indices = 0;
GLuint Bone::vertices = 0;
std::shared_ptr<Shader> Bone::program = nullptr;

const std::array<float, 18> Bone::bone_vertices = {
     0.0f, 0.0f,  0.0f,
     0.1f, 0.1f,  0.1f,
    -0.1f, 0.1f,  0.1f,
    -0.1f, 0.1f, -0.1f,
     0.1f, 0.1f, -0.1f,
     0.0f, 1.0f,  0.0f,
};

const std::array<uint16_t, 24> bone_indices = {
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 1,
    1, 5, 2,
    2, 5, 3,
    3, 5, 4,
    4, 5, 1,
};

void Bone::init(GLuint matrices_uniform) {
    if(Bone::vao == 0) {
        glGenVertexArrays(1, & Bone::vao);
    }
    glBindVertexArray(Bone::vao);
    if(Bone::vertices == 0) {
        glGenBuffers(1, &Bone::vertices);
        glBindBuffer(GL_ARRAY_BUFFER, Bone::vertices);
        glBufferData(GL_ARRAY_BUFFER, bone_vertices.size() * sizeof(float), bone_vertices.data(), GL_STATIC_DRAW);
    }
    if(Bone::indices == 0) {
        glGenBuffers(1, &Bone::indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Bone::indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bone_indices.size() * sizeof(uint16_t), bone_indices.data(), GL_STATIC_DRAW);
    }
    if(Bone::program == nullptr) {
        std::optional<std::filesystem::path> exe_location = executable_location();
        if(!exe_location.has_value()) {
            return;
        }
        std::filesystem::path shaders = exe_location->parent_path() / "share" / "shaders";
        Bone::program = std::make_shared<Shader>(shaders / "bone.vert", shaders / "bone.frag", matrices_uniform, 0);
    }
    glBindVertexArray(0);
}

Bone::Bone(std::string name, glm::mat4 inverse_bind_matrix)
    : m_name(name)
    , m_inverse_bind_matrix(inverse_bind_matrix)
    , m_global_transform_matrix(glm::identity<glm::mat4>())
    , m_length(1.0f)
    , m_offset(0.0f, 1.0f, 0.0f)
{
    glm::mat4 bind_matrix = glm::inverse(m_inverse_bind_matrix);
    m_translation = {bind_matrix[3].x, bind_matrix[3].y, bind_matrix[3].z};
    bind_matrix[3].x = 0;
    bind_matrix[3].y = 0;
    bind_matrix[3].z = 0;
    bind_matrix[0] /= glm::length(bind_matrix[0]);
    bind_matrix[1] /= glm::length(bind_matrix[1]);
    bind_matrix[2] /= glm::length(bind_matrix[2]);
    m_rotation = glm::quat(bind_matrix);
}

Bone::~Bone() {
    m_children.clear();
}

void Bone::add_child(std::shared_ptr<Bone> child) {
    m_children.push_back(child);
}

void Bone::set_parent(std::shared_ptr<Bone> parent) {
    m_parent = parent;
}

std::shared_ptr<Bone> Bone::parent() {
    return m_parent;
}

std::vector<std::shared_ptr<Bone>> Bone::children() {
    return m_children;
}

std::string Bone::name() const {
    return m_name;
}

glm::vec3 Bone::get_position() const {
    return m_translation;
}

glm::vec3 Bone::get_direction() const {
    return glm::normalize(m_offset);
}

glm::quat Bone::get_rotation() const {
    return m_rotation;
}

void Bone::update_tail() {
    if(m_children.size() == 0) {
        spdlog::info("Bone {} has no children to check tail from.", m_name);
        if(m_parent != nullptr) {
            m_parent->update_tail();
            m_length = m_parent->m_length;
            m_offset = m_parent->m_offset;
            spdlog::info("Instead, it will use its parent's scale and offset: {:.2f}\n", m_length);
        } else {
            spdlog::info("It doesn't have a parent, either?\n");
            m_length = 1.0f;
            m_offset = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        return;
    }
    glm::vec3 tail_pos(0.0f);
    for(uint32_t i = 0; i < m_children.size(); i++) {
        glm::vec3 child_pos = m_children[i]->get_position();
        tail_pos += child_pos;
    }
    tail_pos /= m_children.size();
    m_offset = tail_pos - get_position();
    m_length = glm::length(m_offset);
}

void Bone::draw_individual() {
    glBindVertexArray(Bone::vao);
    glBindBuffer(GL_ARRAY_BUFFER, Bone::vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Bone::indices);
    Bone::program->use();
    int lengthLocation = glGetUniformLocation(Bone::program->get_program(), "_length");
    glUniform1f(lengthLocation, m_length);
    Bone::program->set_model(m_global_transform_matrix);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 0);
}

void Bone::draw() {
    if(m_parent == nullptr && m_children.size() == 0) {
        return;
    }
    int lengthLocation = glGetUniformLocation(Bone::program->get_program(), "_length");
    glUniform1f(lengthLocation, m_length);
    glm::mat4 model = glm::identity<glm::mat4>();
    model = glm::toMat4(m_rotation) * model;
    model = glm::translate(glm::identity<glm::mat4>(), m_translation) * model;

    glm::mat4 pointing_model = m_global_transform_matrix * glm::inverse(model) * glm::translate(glm::identity<glm::mat4>(), m_translation) * glm::toMat4(m_pointing_rotation);

    Bone::program->set_model(m_global_transform_matrix);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 0);
}

glm::mat4 Bone::get_inverse_bind_matrix() {
    return m_inverse_bind_matrix;
}

void Bone::set_inverse_bind_matrix(glm::mat4 matrix) {
    m_inverse_bind_matrix = matrix;
}
