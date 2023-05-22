#include "utils/gtk/model_renderer.hpp"

#include <spdlog/spdlog.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/gesturezoom.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollerscroll.h>

#include <utils/adr.h>
#include <utils/materials_3.h>

#include <glm/gtc/type_ptr.hpp>

using namespace warpgate::gtk;

ModelRenderer::ModelRenderer() : m_camera(glm::vec3{2.0f, 2.0f, -2.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f}) {   
    utils::materials3::init_materials();
    
    m_renderer.set_expand(true);
    m_renderer.set_size_request(640, 480);
    m_renderer.set_auto_render(false);
    m_renderer.set_required_version(4, 5);

    m_renderer.signal_realize().connect(sigc::mem_fun(*this, &ModelRenderer::realize));
    m_renderer.signal_unrealize().connect(sigc::mem_fun(*this, &ModelRenderer::unrealize), false);
    m_renderer.signal_render().connect(sigc::mem_fun(*this, &ModelRenderer::render), false);
    m_renderer.signal_resize().connect(sigc::mem_fun(*this, &ModelRenderer::on_resize));
    m_renderer.add_tick_callback(sigc::mem_fun(*this, &ModelRenderer::on_tick));

    auto middle_click = Gtk::GestureClick::create();
    middle_click->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    middle_click->set_button(2);
    middle_click->signal_pressed().connect(sigc::mem_fun(*this, &ModelRenderer::on_mmb_pressed), false);
    middle_click->signal_released().connect(sigc::mem_fun(*this, &ModelRenderer::on_mmb_released), false);
    m_renderer.add_controller(middle_click);

    auto drag = Gtk::GestureDrag::create();
    drag->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    drag->set_button(2);
    drag->set_touch_only(false);
    drag->signal_drag_begin().connect(sigc::mem_fun(*this, &ModelRenderer::on_drag_begin));
    drag->signal_drag_update().connect(sigc::mem_fun(*this, &ModelRenderer::on_drag_update));
    drag->signal_drag_end().connect(sigc::mem_fun(*this, &ModelRenderer::on_drag_end));
    m_renderer.add_controller(drag);

    auto zoom = Gtk::GestureZoom::create();
    zoom->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    zoom->signal_scale_changed().connect(sigc::mem_fun(*this, &ModelRenderer::on_scale_changed));
    m_renderer.add_controller(zoom);

    auto scroll = Gtk::EventControllerScroll::create();
    scroll->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    scroll->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
    scroll->signal_scroll().connect(sigc::mem_fun(*this, &ModelRenderer::on_scroll), false);
    m_renderer.add_controller(scroll);

    auto key = Gtk::EventControllerKey::create();
    key->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    key->signal_key_pressed().connect(sigc::mem_fun(*this, &ModelRenderer::on_key_pressed), false);
    key->signal_key_released().connect(sigc::mem_fun(*this, &ModelRenderer::on_key_released));
    key->signal_modifiers().connect(sigc::mem_fun(*this, &ModelRenderer::on_modifers), false);
    m_renderer.add_controller(key);

    m_renderer.set_focusable();
}

Gtk::GLArea &ModelRenderer::get_area() {
    return m_renderer;
}

void ModelRenderer::realize() {
    m_renderer.make_current();
    try {
        m_renderer.throw_if_error();
        create_grid();
        std::filesystem::path resources = "./resources";
        m_programs[0] = std::make_shared<Shader>(resources / "shaders" / "grid.vert", resources / "shaders" / "grid.frag");
        m_programs[0]->set_matrices(m_matrices);
        m_programs[0]->set_planes(m_planes);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } catch(const Gdk::GLError &gle) {
        spdlog::error("ModelRenderer::realize:");
        spdlog::error("{} - {} - {}", g_quark_to_string(gle.domain()), gle.code(), gle.what());
    }
}

void ModelRenderer::unrealize() {
    m_renderer.make_current();
    try {
        m_renderer.throw_if_error();
        glDeleteBuffers(1, &m_buffer);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteRenderbuffers(1, &m_msaa_rbo);
        glDeleteTextures(1, &m_msaa_tex);
        glDeleteFramebuffers(1, & m_msaa_fbo);
    } catch(const Gdk::GLError &gle) {
        spdlog::error("ModelRenderer::unrealize:");
        spdlog::error("{} - {} - {}", g_quark_to_string(gle.domain()), gle.code(), gle.what());
    }
}

bool ModelRenderer::render(const std::shared_ptr<Gdk::GLContext> &context) {
    try {
        //spdlog::info("Render called");
        m_renderer.throw_if_error();
        glBindFramebuffer(GL_FRAMEBUFFER, m_msaa_fbo);
        glClearColor(0.051f, 0.051f, 0.051f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        m_matrices.viewMatrix = m_camera.get_view();
        m_matrices.invViewMatrix = glm::inverse(m_matrices.viewMatrix);
        m_programs[0]->set_matrices(m_matrices);
        m_programs[0]->set_planes(m_planes);

        draw_grid();

        for(auto it = m_models.begin(); it != m_models.end(); it++) {
            it->second->draw(m_programs, m_textures, m_matrices);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaa_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gtk_fbo);
        glBlitFramebuffer(0, 0, viewport.width, viewport.height, 0, 0, viewport.width, viewport.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glFlush();
        return true;
    } catch(const Gdk::GLError &gle) {
        spdlog::error("ModelRenderer::render:");
        spdlog::error("{} - {} - {}", g_quark_to_string(gle.domain()), gle.code(), gle.what());
        return false;
    }
}

void ModelRenderer::on_resize(int width, int height) {
    viewport.width = width;
    viewport.height = height;
    calculateMatrices(width, height);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &gtk_fbo);

    if(m_msaa_rbo != 0) {
        glDeleteRenderbuffers(1, &m_msaa_rbo);
    }
    if(m_msaa_tex != 0) {
        glDeleteTextures(1, &m_msaa_tex);
    }
    if(m_msaa_fbo != 0) {
        glDeleteFramebuffers(1, &m_msaa_fbo);
    }
    glGenFramebuffers(1, &m_msaa_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaa_fbo);
    glGenTextures(1, &m_msaa_tex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_tex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 16, GL_RGBA, width, height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_msaa_tex, 0);
    
    glGenRenderbuffers(1, &m_msaa_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaa_rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaa_rbo);
    int fbo_status;
    if((fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Failed to create MSAA frame buffer (code {})", fbo_status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, gtk_fbo);
}

bool ModelRenderer::on_tick(const std::shared_ptr<Gdk::FrameClock> &frameClock) {
    //m_renderer.queue_draw();
    return true;
}

void ModelRenderer::on_drag_begin(double start_x, double start_y) {
    spdlog::trace("on_drag_begin");
    if((int)(modifiers_state & Gdk::ModifierType::SHIFT_MASK)) {
        m_panning = true;
    } else {
        m_rotating = true;
    }
}

void ModelRenderer::on_drag_update(double offset_x, double offset_y) {
    spdlog::trace("on_drag_update {:.2f} {:.2f}", offset_x, offset_y);
    if(!(int)(modifiers_state & Gdk::ModifierType::BUTTON2_MASK)) {
        // Button 2 (MMB) not held, do nothing
        return;
    }
    if(m_rotating) {
        m_camera.rotate_about_target(offset_x / viewport.width * 360.0f, offset_y / viewport.height * 360.0f);
    } else if(m_panning) {
        m_camera.pan(
            (
                m_camera.right() * -1.0f * ((float)offset_x / viewport.width) 
                + m_camera.up() * 1.0f * ((float)offset_y / viewport.height)
            ) * m_camera.radius()
        );
    }
    m_renderer.queue_render();
}

void ModelRenderer::on_drag_end(double offset_x, double offset_y) {
    spdlog::trace("on_drag_end");
    if(m_rotating) {
        m_camera.commit_rotation();
        m_rotating = false;
    } else if(m_panning) {
        m_camera.commit_pan();
        m_panning = false;
    }
    m_renderer.queue_render();
}

void ModelRenderer::on_scale_changed(double scale) {
    spdlog::info("Scale = {:.2f}", scale);
}

bool ModelRenderer::on_scroll(double delta_x, double delta_y) {
    spdlog::trace("Scroll = {:.2f}, {:.2f}", delta_x, delta_y);
    m_camera.translate(m_camera.forward() * (float)delta_y * 0.1f * m_camera.radius());
    m_camera.commit_translate();
    m_renderer.queue_render();
    return false;
}

bool ModelRenderer::on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state) {
    // todo: Add keyboard controls here...

    // Allow propagation
    return false;
}

void ModelRenderer::on_key_released(guint keyval, guint keycode, Gdk::ModifierType state) {
    // todo: Add keyboard controls here...
}

bool ModelRenderer::on_modifers(Gdk::ModifierType state) {
    Gdk::ModifierType bits = (Gdk::ModifierType::BUTTON1_MASK 
        | Gdk::ModifierType::BUTTON2_MASK 
        | Gdk::ModifierType::BUTTON3_MASK 
        | Gdk::ModifierType::BUTTON4_MASK 
        | Gdk::ModifierType::BUTTON5_MASK);
    state &= ~bits;
    spdlog::trace("Modifiers changed to {:08x}", (int)((modifiers_state & bits) | state));
    modifiers_state = state | (modifiers_state & bits);
    return false;
}

void ModelRenderer::on_mmb_pressed(int n_buttons, double x, double y) {
    Gdk::ModifierType state = Gdk::ModifierType::BUTTON2_MASK;
    spdlog::trace("Modifiers changed to {:08x}", (int)(modifiers_state | state));
    modifiers_state |= state;
}

void ModelRenderer::on_mmb_released(int n_buttons, double x, double y) {
    Gdk::ModifierType state = ~Gdk::ModifierType::BUTTON2_MASK;
    spdlog::trace("Modifiers changed to {:08x}", (int)(modifiers_state & state));
    modifiers_state &= state;
}

void ModelRenderer::create_grid() {
    std::vector<float> vertices = {
        1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        1, 0, 3
    };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.size(), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Texture::Texture(gli::texture tex, Semantic semantic)
    : m_semantic(Parameter::texture_common_semantic(semantic))
{
    gli::gl GL(gli::gl::PROFILE_KTX);
    gli::gl::format const format = GL.translate(tex.format(), tex.swizzles());
    gli::gl::target const target = GL.translate(tex.target());
    m_target = target;
    glGenTextures(1, &m_texture);
    glBindTexture(target, m_texture);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    switch(target) {
    case GL_TEXTURE_1D:
        glTexImage1D(target, 0, format.Internal, tex.extent().x, 0, format.External, format.Type, tex.data());
        break;
    case GL_TEXTURE_2D:
        glTexImage2D(target, 0, format.Internal, tex.extent().x, tex.extent().y, 0, format.External, format.Type, tex.data());
        break;
    case GL_TEXTURE_3D:
        glTexImage3D(target, 0, format.Internal, tex.extent().x, tex.extent().y, tex.extent().z, 0, format.External, format.Type, tex.data());
        break;
    case GL_TEXTURE_1D_ARRAY:
        glTexImage2D(target, 0, format.Internal, tex.extent().x, tex.layers(), 0, format.External, format.Type, tex.data());
        break;
    case GL_TEXTURE_2D_ARRAY:
        glTexImage3D(target, 0, format.Internal, tex.extent().x, tex.extent().y, tex.layers(), 0, format.External, format.Type, tex.data());
        break;
    case GL_TEXTURE_CUBE_MAP:
        for(uint32_t i = 0; i < tex.faces(); i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format.Internal, tex.extent().x, tex.extent().y, 0, format.External, format.Type, ((gli::texture_cube)tex)[i].data());
        }
        break;
    }
    glGenerateMipmap(target);
    glBindTexture(target, 0);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_texture);
}

void Texture::bind() {
    int32_t unit = get_unit();
    if(unit < 0) {
        spdlog::warn("Attempted to bind texture of unknown semantic");
        return;
    }
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, m_texture);
}

int32_t Texture::get_unit() {
    switch(m_semantic) {
    case Parameter::WarpgateSemantic::DIFFUSE:
        return 0;
    case Parameter::WarpgateSemantic::NORMAL:
        return 1;
    case Parameter::WarpgateSemantic::SPECULAR:
        return 2;
    case Parameter::WarpgateSemantic::DETAILCUBE:
        return 3;
    case Parameter::WarpgateSemantic::DETAILMASK:
        return 4;
    default:
        return -1;
    }
}

GLboolean get_normalized(std::string type, std::string usage) {
    if(type == "Float1") {
        return GL_FALSE;
    } else if(type == "Float2") {
        return GL_FALSE;
    } else if(type == "Float3") {
        return GL_FALSE;
    } else if(type == "Float4") {
        return GL_FALSE;
    } else if(type == "D3dcolor") {
        if(usage == "BlendIndices") {
            return GL_FALSE;
        } else {
            return GL_TRUE;
        }
    } else if(type == "ubyte4n") {
        return GL_TRUE;
    } else if(type == "Float16_2" || type == "float16_2") {
        return GL_FALSE;
    } else if(type == "Short2") {
        return GL_TRUE;
    } else if(type == "Short4") {
        return GL_TRUE;
    }

    return GL_FALSE;
}

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

// void Shader::init_uniforms() {
//     glGenBuffers(1, &m_ubo_matrices);
//     glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
//     glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), &m_matrices, GL_STATIC_DRAW);
//     glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_matrices);

//     glGenBuffers(1, &m_ubo_planes);
//     glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_planes);
//     glBufferData(GL_UNIFORM_BUFFER, sizeof(GridUniform), &m_planes, GL_STATIC_DRAW);
//     glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_ubo_planes);
//     glBindBuffer(GL_UNIFORM_BUFFER, 0);
// }

// void ModelRenderer::update_uniforms() {
//     glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
//     glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), &m_matrices, GL_STATIC_DRAW);
    
//     glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_planes);
//     glBufferData(GL_UNIFORM_BUFFER, sizeof(GridUniform), &m_planes, GL_STATIC_DRAW);
//     glBindBuffer(GL_UNIFORM_BUFFER, 0);
// }

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

Mesh::Mesh(
    std::shared_ptr<const warpgate::Mesh> mesh,
    std::shared_ptr<const warpgate::Material> material,
    nlohmann::json layout,
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
    std::shared_ptr<synthium::Manager> manager

) : m_index_count(mesh->index_count())
  , m_index_size(mesh->index_size() & 0xFF)
  , material_hash(layout["hash"])
  , material_name(layout["name"])
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::span<uint8_t> index_data = mesh->index_data();

    glGenBuffers(1, &indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_data.size(), index_data.data(), GL_STATIC_DRAW);

    vertex_streams.resize(mesh->vertex_stream_count());
    glGenBuffers(mesh->vertex_stream_count(), vertex_streams.data());
    for(uint32_t vs_index = 0; vs_index < mesh->vertex_stream_count(); vs_index++) {
        glBindBuffer(GL_ARRAY_BUFFER, vertex_streams[vs_index]);
        std::span<uint8_t> vs_data = mesh->vertex_stream(vs_index);
        glBufferData(GL_ARRAY_BUFFER, vs_data.size(), vs_data.data(), GL_STATIC_DRAW);
    }

    std::unordered_map<uint32_t, uint32_t> stream_strides;
        
    uint32_t pointer = 0;
    for(nlohmann::json entry : layout["entries"]) {
        uint32_t vs_index = entry["stream"];
        if(stream_strides.find(vs_index) == stream_strides.end()) {
            stream_strides[vs_index] = 0;
        }
        int32_t stride = mesh->bytes_per_vertex(vs_index);
        if(stream_strides[vs_index] >= stride) {
            continue;
        }

        uint32_t entry_size = utils::materials3::sizes[entry["type"]];
        int32_t entry_count = utils::materials3::types[entry["type"]];
        GLenum entry_type = utils::materials3::component_types[entry["type"]];
        if(entry_type == GL_SHORT && entry["usage"] == "Texcoord") {
            entry_type = GL_UNSIGNED_SHORT;
        }
        if(entry_type == GL_UNSIGNED_BYTE && (
            entry["usage"] == "Binormal" ||
            entry["usage"] == "Normal" ||
            entry["usage"] == "Tangent" ||
            entry["usage"] == "Position"
        )) {
            entry_type = GL_BYTE;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vertex_streams[vs_index]);
        glEnableVertexAttribArray(pointer);
        GLboolean normalized = get_normalized(entry["type"], entry["usage"]);
        void* offset = (void*)stream_strides[vs_index];
        glVertexAttribPointer(pointer, entry_count, entry_type, normalized, stride, offset);
        vertex_layouts.push_back(Layout{vs_index, pointer, entry_count, entry_type, normalized, stride, offset});
        
        pointer++;
        stream_strides[vs_index] += entry_size;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    for(uint32_t param = 0; param < material->param_count(); param++) {
        const warpgate::Parameter &parameter = material->parameter(param);
        Parameter::D3DXParamType type = parameter.type();
        switch(type) {
        case warpgate::Parameter::D3DXParamType::TEXTURE1D:
        case warpgate::Parameter::D3DXParamType::TEXTURE2D:
        case warpgate::Parameter::D3DXParamType::TEXTURE3D:
        case warpgate::Parameter::D3DXParamType::TEXTURE:
        case warpgate::Parameter::D3DXParamType::TEXTURECUBE:
            break;
        default:
            continue;
        }


        Parameter::WarpgateSemantic semantic = Parameter::texture_common_semantic(parameter.semantic_hash());
        switch(semantic) {
        case Parameter::WarpgateSemantic::DIFFUSE:
        case Parameter::WarpgateSemantic::NORMAL:
        case Parameter::WarpgateSemantic::SPECULAR:
        case Parameter::WarpgateSemantic::DETAILCUBE:
        case Parameter::WarpgateSemantic::DETAILMASK:
            break;
        default:
            continue;
        }

        uint32_t texture_hash = parameter.get<uint32_t>(parameter.data_offset());
        m_texture_hashes.push_back(texture_hash);
        if(textures.find(texture_hash) != textures.end()) {
            continue;
        }

        std::optional<std::string> texture_name = material->texture(parameter.semantic_hash());
        if(!texture_name.has_value()) {
            continue;
        }

        std::shared_ptr<synthium::Asset2> asset = manager->get(*texture_name);
        std::vector<uint8_t> asset_data = asset->get_data();
        gli::texture texture = gli::load_dds((char*)asset_data.data(), asset_data.size());
        textures[texture_hash] = std::make_shared<Texture>(texture, parameter.semantic_hash());
    }
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &indices);
    glDeleteBuffers(vertex_streams.size(), vertex_streams.data());
    glDeleteVertexArrays(1, &vao);
}

void Mesh::bind() const {
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    for(uint32_t i = 0; i < vertex_layouts.size(); i++) {
        const Layout& layout = vertex_layouts[i];
        glBindBuffer(GL_ARRAY_BUFFER, vertex_streams[layout.vs_index]);
        glEnableVertexAttribArray(layout.pointer);
        glVertexAttribPointer(layout.pointer, layout.count, layout.type, layout.normalized, layout.stride, layout.offset);
    }
}

std::vector<uint32_t> Mesh::get_texture_hashes() const {
    return m_texture_hashes;
}

uint32_t Mesh::get_material_hash() const {
    return material_hash;
}

uint32_t Mesh::index_count() const {
    return m_index_count;
}

uint32_t Mesh::index_size() const {
    return m_index_size;
}

std::pair<std::filesystem::path, std::filesystem::path> Mesh::get_shader_paths() const {
    std::filesystem::path resources = "./resources";
    std::filesystem::path vertex, fragment;
    vertex = fragment = resources / "shaders" / material_name;
    vertex.replace_extension("vert");
    fragment.replace_extension("frag");
    return std::make_pair(vertex, fragment);
}

Model::Model(
    std::string name,
    std::shared_ptr<warpgate::DME> dme,
    std::unordered_map<uint32_t, std::shared_ptr<Shader>> &shaders,
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
    std::shared_ptr<synthium::Manager> manager
) {
    m_name = name;
    m_uname.assign(name.c_str());
    m_model = glm::identity<glm::mat4>();
    std::shared_ptr<const warpgate::DMAT> dmat = dme->dmat();
    for(uint32_t mesh_index = 0; mesh_index < dme->mesh_count(); mesh_index++) {
        uint32_t material_definition = dmat->material(mesh_index)->definition();
        std::string layout_name = utils::materials3::materials["materialDefinitions"][std::to_string(material_definition)]["drawStyles"][0]["inputLayout"];
        
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(
            dme->mesh(mesh_index),
            dmat->material(mesh_index),
            utils::materials3::materials["inputLayouts"][layout_name],
            textures,
            manager
        );
        
        if(shaders.find(mesh->get_material_hash()) == shaders.end()) {
            auto[vertex, fragment] = mesh->get_shader_paths();
            std::shared_ptr<Shader> shader = std::make_shared<Shader>(vertex, fragment);
            if(shader->bad()) {
                continue;
            }
            shaders[mesh->get_material_hash()] = shader;
        }
        m_meshes.push_back(mesh);
    }
}

Model::~Model() {
    m_meshes.clear();
}

void Model::draw(
    std::unordered_map<uint32_t, std::shared_ptr<Shader>> &shaders,
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
    const Uniform &ubo
) const {
    for(uint32_t i = 0; i < m_meshes.size(); i++) {
        if(shaders.find(m_meshes[i]->get_material_hash()) == shaders.end()) {
            continue;
        }
        shaders[m_meshes[i]->get_material_hash()]->use();
        shaders[m_meshes[i]->get_material_hash()]->set_matrices(ubo);
        shaders[m_meshes[i]->get_material_hash()]->set_model(m_model);

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

void ModelRenderer::load_model(std::string name, AssetType type, std::shared_ptr<synthium::Manager> manager) {
    if(m_models.find(name) != m_models.end()) {
        spdlog::warn("{} already loaded", name);
        return;
    }
    std::shared_ptr<synthium::Asset2> asset = manager->get(name), asset2;
    std::shared_ptr<warpgate::DME> dme;
    std::vector<uint8_t> data, data2;
    switch(type) {
    case AssetType::ACTOR_RUNTIME:{
        data = asset->get_data();
        utils::ADR adr(data);
        std::optional<std::string> model = adr.base_model();
        if(!model.has_value()) {
            spdlog::error("'{}' has no associated DME file!", name);
            return;
        }
        std::optional<std::string> palette = adr.base_palette();

        asset = manager->get(*model);
        data = asset->get_data();
        dme = std::make_shared<warpgate::DME>(data, *model);

        if(palette.has_value() && (*palette).size() > 0) {
            std::filesystem::path model_path = *model;
            std::filesystem::path palette_path = *palette;
            if(utils::lowercase(model_path.stem().string()) == utils::lowercase(palette_path.stem().string())) {
                // Palette is not different than what would already be included in the dme.
                break;
            }
            asset2 = manager->get(*palette);
            data2 = asset->get_data();
            std::shared_ptr<warpgate::DMAT> dmat = std::make_shared<warpgate::DMAT>(data2);
            dme->set_dmat(dmat);
        }
        break;
    }
    case AssetType::MODEL:
        data = asset->get_data();
        dme = std::make_shared<warpgate::DME>(data, name);
        break;
    case AssetType::ANIMATION:
        // Todo...
    default:
        spdlog::warn("No method known to load asset '{}' as a model", name);
        return;
    }
     
    std::shared_ptr<Model> model = std::make_shared<Model>(name, dme, m_programs, m_textures, manager);
    m_models[name] = model;
    m_renderer.queue_render();
}

void ModelRenderer::destroy_model(std::string name) {
    if(m_models.find(name) == m_models.end()) {
        spdlog::warn("{} already destroyed", name);
        return;
    }
    m_models.erase(name);
}

void ModelRenderer::calculateMatrices(int width, int height) {
    m_planes.near_plane = 0.01f;
    m_planes.far_plane = 100.0f;

    m_matrices.projectionMatrix = glm::perspective(glm::radians(74.0f), (float)width / (float)height, m_planes.near_plane, m_planes.far_plane);
    m_matrices.viewMatrix = m_camera.get_view();
    m_matrices.modelMatrix = glm::identity<glm::mat4>();
    m_matrices.invProjectionMatrix = glm::inverse(m_matrices.projectionMatrix);
    m_matrices.invViewMatrix = glm::inverse(m_matrices.viewMatrix);
}

void ModelRenderer::draw_grid() {
    m_programs[0]->use();
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}