#pragma once

#include "utils/vulkan/camera.hpp"
#include <dme_loader.h>
#include <gli/gli.hpp>
#include <glm/gtx/quaternion.hpp>
#include <half.hpp>
#include <json.hpp>

#include <span>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <exception>
#include <filesystem>
#include <chrono>

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/glarea.h>
#include <epoxy/gl.h>

namespace warpgate::gtk {
    class ModelRenderer {
    public:
        ModelRenderer();

        Gtk::GLArea &get_area();
    
    protected:
        struct Uniform {
            glm::mat4 projectionMatrix;
            glm::mat4 modelMatrix;
            glm::mat4 viewMatrix;
            glm::mat4 invProjectionMatrix;
            glm::mat4 invViewMatrix;
            uint32_t faction = 0;
            uint32_t pad[3] = {0, 0, 0};
        };

        struct GridUniform {
            float near_plane, far_plane;
            uint32_t pad[2] = {0, 0};
        };

        struct Rect {
            int x, y, width, height;
        };

        Gdk::ModifierType modifiers_state {0};
        Rect viewport = {};

        warpgate::vulkan::Camera m_camera;
        bool m_panning = false;
        bool m_rotating = false;
        nlohmann::json m_materials;
        Uniform m_matrices {};
        GridUniform m_planes {};

        Gtk::GLArea m_renderer;

        std::unordered_map<uint32_t, GLuint> m_programs;

        GLint gtk_fbo = 0;
        GLuint m_msaa_fbo = 0;
        GLuint m_msaa_tex = 0;
        GLuint m_msaa_rbo = 0;
        GLuint m_vao = 0;
        GLuint m_ebo = 0;
        GLuint m_buffer = 0;
        GLuint m_program = 0;
        GLuint m_ubo_matrices = 0;
        GLuint m_ubo_planes = 0;

        void realize();
        void unrealize();
        bool render(const std::shared_ptr<Gdk::GLContext> &context);
        void on_resize(int width, int height);
        bool on_tick(const std::shared_ptr<Gdk::FrameClock> &frameClock);
        void on_drag_begin(double start_x, double start_y);
        void on_drag_update(double offset_x, double offset_y);
        void on_drag_end(double offset_x, double offset_y);
        void on_scale_changed(double scale);
        bool on_scroll(double offset_x, double offset_y);
        bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
        void on_key_released(guint keyval, guint keycode, Gdk::ModifierType state);
        bool on_modifers(Gdk::ModifierType state);
        void on_mmb_pressed(int n_buttons, double x, double y);
        void on_mmb_released(int n_buttons, double x, double y);

        void init_buffers();
        GLuint create_shader_program(const std::filesystem::path vertex, const std::filesystem::path fragment);
        void init_uniforms();
        void update_uniforms();
        void calculateMatrices(int width, int height);
        void draw_grid();
    };
};