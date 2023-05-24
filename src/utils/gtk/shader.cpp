#include "utils/gtk/shader.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

using namespace warpgate::gtk;

Shader::Shader(const std::filesystem::path vertex, const std::filesystem::path fragment) : m_good(false) {
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_file; 
    std::ifstream fragment_file; 
    vertex_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        vertex_file.open(vertex);
        fragment_file.open(fragment);
        std::stringstream vertex_stream, fragment_stream;

        vertex_stream << vertex_file.rdbuf();
        fragment_stream << fragment_file.rdbuf();

        vertex_file.close();
        fragment_file.close();

        vertex_code = vertex_stream.str();
        fragment_code = fragment_stream.str();
    } catch(std::ifstream::failure &e) {
        spdlog::error("Failed to load shader: {}", e.what());
        return;
    }


    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, (const char*)vertex_code.c_str());

    if(vertex_shader == 0) {
        return;
    }

    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, (const char*)fragment_code.c_str());

    if(fragment_shader == 0) {
        glDeleteShader(fragment_shader);
        return;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vertex_shader);
    glAttachShader(m_program, fragment_shader);

    glLinkProgram(m_program);

    int status;
    glGetProgramiv(m_program, GL_LINK_STATUS, &status);
    if(!status) {
        int log_len;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_message(log_len + 1, ' ');
        glGetProgramInfoLog(m_program, log_len, nullptr, log_message.data());

        spdlog::error("Failed to link shader program:");
        spdlog::error("Vertex: {}\n{}\n", vertex.string(), vertex_code);
        spdlog::error("Fragment: {}\n{}\n", fragment.string(), fragment_code);
        spdlog::error(log_message);

        glDeleteProgram(m_program);
        return;
    }
    glDetachShader(m_program, vertex_shader);
    glDetachShader(m_program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    m_good = true;

    glGenBuffers(1, &m_ubo_matrices);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_matrices);

    glGenBuffers(1, &m_ubo_planes);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_planes);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GridUniform), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_ubo_planes);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

Shader::~Shader() {
    
}

bool Shader::good() const {
    return m_good;
}

bool Shader::bad() const {
    return !m_good;
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::set_model(const glm::mat4& model) {
    use();
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Uniform, modelMatrix), sizeof(model), glm::value_ptr(model));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Shader::set_matrices(const Uniform& ubo) {
    use();
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), &ubo, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Shader::set_planes(const GridUniform& planes) {
    use();
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_planes);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GridUniform), &planes, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GLuint Shader::create_shader(int type, const char *src) {
    spdlog::debug("create shader from source:\n{}", src);
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if(!status) {
        int log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_message(log_len + 1, ' ');
        glGetShaderInfoLog(shader, log_len, nullptr, log_message.data());

        spdlog::error("Failed to compile {} shader:", type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        spdlog::error(log_message);

        glDeleteShader(shader);
        return 0;
    }
    return shader;
}