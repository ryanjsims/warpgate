#pragma once

#include <gli/gli.hpp>
#include <dme_loader.h>

#include <epoxy/gl.h>

namespace warpgate::gtk {
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
};