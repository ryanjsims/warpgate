#pragma once

#include "utils/vulkan/camera.hpp"
#include "utils/gtk/asset_type.hpp"
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

    class Texture {
    public:
        Texture(gli::texture tex, Semantic semantic);
        ~Texture();
    
        void bind();
        void unbind();
    private:
        GLuint m_texture;
        Parameter::WarpgateSemantic m_semantic;
        GLenum m_target;

        int32_t get_unit();
    };

    class Shader {
    public:
        Shader(const std::filesystem::path vertex, const std::filesystem::path fragment);
        ~Shader();

        void use() const;
        bool good() const;
        bool bad() const;


        void set_model(const glm::mat4 &model);
        void set_matrices(const Uniform& ubo);
        void set_planes(const GridUniform& planes);

    private:
        GLuint m_program;
        GLuint m_ubo_matrices;
        GLuint m_ubo_planes;
        bool m_good;

        GLuint create_shader(int type, const char *src);
    };

    class Mesh {
    public:
        Mesh(
            std::shared_ptr<const warpgate::Mesh> mesh,
            std::shared_ptr<const warpgate::Material> material,
            nlohmann::json layout,
            std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
            std::shared_ptr<synthium::Manager> manager
        );
        ~Mesh();

        std::pair<std::filesystem::path, std::filesystem::path> get_shader_paths() const;
        uint32_t get_material_hash() const;
        std::vector<uint32_t> get_texture_hashes() const;

        void bind() const;
        void unbind() const;
        uint32_t index_count() const;
        uint32_t index_size() const;

    private:
        struct Layout {
            uint32_t vs_index;
            GLuint pointer;
            GLint count;
            GLenum type;
            GLboolean normalized;
            GLsizei stride;
            void* offset;
        };

        GLuint vao;
        std::vector<GLuint> vertex_streams;
        std::vector<Layout> vertex_layouts;
        GLuint indices;
        uint32_t m_index_count, m_index_size, material_hash;
        std::vector<uint32_t> m_texture_hashes;
        std::string material_name;
    };

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
        GLuint m_grid_fbo = 0;
        GLuint m_grid_tex = 0;
        GLuint m_grid_depth_tex = 0;
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