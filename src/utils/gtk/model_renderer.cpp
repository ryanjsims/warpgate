#include "utils/gtk/model_renderer.hpp"

#include <spdlog/spdlog.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/gesturezoom.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollerscroll.h>

#include <utils/adr.h>
#include <utils/materials_3.h>
#include <synthium/utils.h>

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
        // glFrontFace(GL_CW);
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
        glDeleteTextures(1, &m_msaa_depth_tex);
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
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_matrices.viewMatrix = m_camera.get_view();
        m_matrices.invViewMatrix = glm::inverse(m_matrices.viewMatrix);

        for(auto it = m_models.begin(); it != m_models.end(); it++) {
            it->second->draw(m_programs, m_textures, m_matrices, m_planes);
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_programs[0]->set_matrices(m_matrices);
        m_programs[0]->set_planes(m_planes);
        draw_grid();
        glCheckError("Draw grid");
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaa_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gtk_fbo);
        glBlitFramebuffer(0, 0, viewport.width, viewport.height, 0, 0, viewport.width, viewport.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glCheckError("Blit grid fbo");
        return true;
    } catch(const Gdk::GLError &gle) {
        spdlog::error("ModelRenderer::render:");
        spdlog::error("{} - {} - {}", g_quark_to_string(gle.domain()), gle.code(), gle.what());
        return false;
    }
}

std::tuple<GLuint, GLuint, GLuint> ModelRenderer::generate_msaa_framebuffer(uint32_t samples, uint32_t width, uint32_t height) {
    GLuint msaa_fbo, msaa_tex, msaa_depth_tex;
    glGenFramebuffers(1, &msaa_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
    glGenTextures(1, &msaa_tex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_tex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, width, height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaa_tex, 0);
    
    glGenTextures(1, &msaa_depth_tex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_depth_tex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH_COMPONENT24, width, height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msaa_depth_tex, 0);
    int fbo_status;
    if((fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Failed to create MSAA frame buffer (code {})", fbo_status);
    }

    return std::make_tuple(msaa_fbo, msaa_tex, msaa_depth_tex);
}

void ModelRenderer::on_resize(int width, int height) {
    viewport.width = width;
    viewport.height = height;
    calculateMatrices(width, height);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &gtk_fbo);

    if(m_msaa_depth_tex != 0) {
        glDeleteTextures(1, &m_msaa_depth_tex);
    }
    if(m_msaa_tex != 0) {
        glDeleteTextures(1, &m_msaa_tex);
    }
    if(m_msaa_fbo != 0) {
        glDeleteFramebuffers(1, &m_msaa_fbo);
    }
    
    std::tie(m_msaa_fbo, m_msaa_tex, m_msaa_depth_tex) = generate_msaa_framebuffer(16, viewport.width, viewport.height);

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

void ModelRenderer::load_model(std::string name, AssetType type, std::shared_ptr<synthium::Manager> manager) {
    if(m_models.find(name) != m_models.end()) {
        spdlog::warn("{} already loaded", name);
        return;
    }
    m_renderer.make_current();
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
    m_renderer.make_current();
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