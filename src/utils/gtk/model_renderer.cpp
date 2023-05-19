#include "utils/gtk/model_renderer.hpp"

#include <spdlog/spdlog.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/gesturezoom.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollerscroll.h>

using namespace warpgate::gtk;

ModelRenderer::ModelRenderer() : m_camera(glm::vec3{2.0f, 2.0f, -2.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f}) {
    set_title("Warpgate");
    set_default_size(1280, 960);
    
    
    set_child(m_box);
    
    m_renderer.set_expand(true);
    m_renderer.set_size_request(640, 480);
    m_renderer.set_auto_render(true);
    m_renderer.set_required_version(4, 5);

    m_box.append(m_renderer);

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
    key->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    key->signal_key_pressed().connect(sigc::mem_fun(*this, &ModelRenderer::on_key_pressed), false);
    key->signal_key_released().connect(sigc::mem_fun(*this, &ModelRenderer::on_key_released));
    key->signal_modifiers().connect(sigc::mem_fun(*this, &ModelRenderer::on_modifers), false);
    m_renderer.add_controller(key);

    m_renderer.set_focusable();
}

ModelRenderer::~ModelRenderer() {

}

void ModelRenderer::realize() {
    m_renderer.make_current();
    try {
        m_renderer.throw_if_error();
        init_buffers();
        init_uniforms();
        std::filesystem::path resources = "./resources";
        m_program = create_shader_program(resources / "shaders" / "grid.vert", resources / "shaders" / "grid.frag");
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
        glDeleteBuffers(1, &m_ubo_matrices);
        glDeleteBuffers(1, &m_ubo_planes);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteRenderbuffers(1, &m_msaa_rbo);
        glDeleteTextures(1, &m_msaa_tex);
        glDeleteFramebuffers(1, & m_msaa_fbo);
        glDeleteProgram(m_program);
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
        update_uniforms();

        draw_grid();

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
    m_renderer.queue_draw();
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
}

void ModelRenderer::on_scale_changed(double scale) {
    spdlog::info("Scale = {:.2f}", scale);
}

bool ModelRenderer::on_scroll(double delta_x, double delta_y) {
    spdlog::trace("Scroll = {:.2f}, {:.2f}", delta_x, delta_y);
    m_camera.translate(m_camera.forward() * (float)delta_y * 0.1f * m_camera.radius());
    m_camera.commit_translate();
    return false;
}

bool ModelRenderer::on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state) {
    // todo: Add keyboard controls here...

    // Allow propagation
    return false;
}

void ModelRenderer::on_key_released(guint keyval, guint keycode, Gdk::ModifierType state) {
    // todo: Add keyboard controls here...

    // Allow propagation
    //return false;
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

void ModelRenderer::init_buffers() {
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

    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static GLuint create_shader(int type, const char *src) {
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

GLuint ModelRenderer::create_shader_program(const std::filesystem::path vertex, const std::filesystem::path fragment) {
    std::ifstream vertex_file(vertex, std::ios::ate);
    if(!vertex_file) {
        spdlog::error("Failed to load vertex shader {}", vertex.string());
        return 0;
    }
    size_t length = vertex_file.tellg();
    vertex_file.seekg(0);
    std::unique_ptr<uint8_t[]> data;
    try {
        data = std::make_unique<uint8_t[]>(length + 1);
    } catch(std::bad_alloc) {
        spdlog::error("init_shaders: Failed to allocate memory for vertex shader!");
        std::exit(1);
    }
    //memset(data.get(), 0, length + 1);
    data[length] = 0;
    vertex_file.read((char*)data.get(), length);
    vertex_file.close();

    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, (const char*)data.get());

    if(vertex_shader == 0) {
        return 0;
    }

    std::ifstream fragment_file(fragment, std::ios::ate);
    if(!fragment_file) {
        spdlog::error("Failed to load fragment shader {}", fragment.string());
        return 0;
    }
    length = fragment_file.tellg();
    fragment_file.seekg(0);
    try {
        data = std::make_unique<uint8_t[]>(length + 1);
    } catch(std::bad_alloc) {
        spdlog::error("init_shaders: Failed to allocate memory for fragment shader!");
        std::exit(1);
    }
    //memset(data.get(), 0, length + 1);
    data[length] = 0;
    fragment_file.read((char*)data.get(), length);
    fragment_file.close();

    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, (const char*)data.get());

    if(fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    GLuint to_return = glCreateProgram();
    glAttachShader(to_return, vertex_shader);
    glAttachShader(to_return, fragment_shader);

    glLinkProgram(to_return);

    int status;
    glGetProgramiv(to_return, GL_LINK_STATUS, &status);
    if(!status) {
        int log_len;
        glGetProgramiv(to_return, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_message(log_len + 1, ' ');
        glGetProgramInfoLog(to_return, log_len, nullptr, log_message.data());

        spdlog::error("Failed to link shader program:");
        spdlog::error(log_message);

        glDeleteProgram(to_return);
        return 0;
    }
    glDetachShader(to_return, vertex_shader);
    glDetachShader(to_return, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return to_return;
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

void ModelRenderer::init_uniforms() {
    glGenBuffers(1, &m_ubo_matrices);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), &m_matrices, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_matrices);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &m_ubo_planes);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_planes);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GridUniform), &m_planes, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_ubo_planes);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ModelRenderer::update_uniforms() {
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_matrices);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), &m_matrices, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_planes);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GridUniform), &m_planes, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ModelRenderer::draw_grid() {
    glUseProgram(m_program);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
}