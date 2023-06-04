#include "utils/gtk/bone.hpp"
#include "utils/common.h"

#include <array>

using namespace warpgate::gtk;

GLuint Bone::vao = 0;
GLuint Bone::indices = 0;
GLuint Bone::vertices = 0;
std::shared_ptr<Shader> Bone::program = nullptr;

const std::array<float, 18> bone_vertices = {
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

Bone::Bone(glm::mat4 bind_matrix) : m_bind_matrix(bind_matrix) {
    
}

Bone::~Bone() {

}

void Bone::draw() {
    glBindVertexArray(Bone::vao);
    glBindBuffer(GL_ARRAY_BUFFER, Bone::vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Bone::indices);
    Bone::program->use();
    Bone::program->set_model(m_bind_matrix);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 0);
}

glm::mat4 Bone::get_bind_matrix() {
    return m_bind_matrix;
}

void Bone::set_bind_matrix(glm::mat4 matrix) {
    m_bind_matrix = matrix;
}