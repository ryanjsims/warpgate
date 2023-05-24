#pragma once

#include "utils/vulkan/camera.hpp"
#include "utils/gtk/asset_type.hpp"
#include "utils/gtk/common.hpp"
#include "utils/gtk/mesh.hpp"
#include "utils/gtk/model.hpp"
#include "utils/gtk/texture.hpp"
#include "utils/gtk/shader.hpp"
#include "utils/gtk/structs.hpp"
#include <dme_loader.h>
#include <gli/gli.hpp>
#include <glm/gtx/quaternion.hpp>
#include <half.hpp>
#include <json.hpp>
#include <synthium/manager.h>

#include <span>
#include <string>
#include <fstream>
#include <vector>
#include <tuple>
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
        void load_model(std::string name, AssetType type, std::shared_ptr<synthium::Manager> manager);
        void destroy_model(std::string name);
    
    protected:
        Gdk::ModifierType modifiers_state {0};
        Rect viewport = {};

        warpgate::vulkan::Camera m_camera;
        bool m_panning = false;
        bool m_rotating = false;
        Uniform m_matrices {};
        GridUniform m_planes {};

        Gtk::GLArea m_renderer;

        std::unordered_map<uint32_t, std::shared_ptr<Shader>> m_programs;
        std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_textures;
        std::unordered_map<uint32_t, std::vector<std::shared_ptr<Mesh>>> m_meshes_by_material;
        std::map<std::string, std::shared_ptr<Model>> m_models;

        GLint gtk_fbo = 0;
        GLuint m_msaa_fbo = 0;
        GLuint m_msaa_tex = 0;
        GLuint m_msaa_depth_tex = 0;
        GLuint m_vao = 0;
        GLuint m_ebo = 0;
        GLuint m_buffer = 0;

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

        /**
         * generate_msaa_framebuffer(uint32_t samples)
         * 
         * Generates a framebuffer, texture, depth texture triplet with <samples>x multisampling
         * 
         * @returns std::tuple of framebuffer, texture, and depth texture names
         */
        std::tuple<GLuint, GLuint, GLuint> generate_msaa_framebuffer(uint32_t samples, uint32_t width, uint32_t height);

        void create_grid();

        //GLuint create_shader_program(const std::filesystem::path vertex, const std::filesystem::path fragment);
        //void init_uniforms();
        //void update_uniforms();
        void calculateMatrices(int width, int height);
        void draw_grid();
    };
};