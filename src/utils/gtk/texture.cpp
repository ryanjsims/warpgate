#include "utils/gtk/texture.hpp"

#include "utils/gtk/common.hpp"
#include <spdlog/spdlog.h>
#include <synthium/utils.h>

using namespace warpgate::gtk;

Texture::Texture(gli::texture tex, Semantic semantic)
    : m_semantic(Parameter::texture_common_semantic(semantic))
{
    gli::gl GL(gli::gl::PROFILE_GL33);
    gli::gl::format const format = GL.translate(tex.format(), tex.swizzles());
    gli::gl::target const target = GL.translate(tex.target());
    m_target = target;
    glGenTextures(1, &m_texture);
    glBindTexture(target, m_texture);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, tex.levels() - 1);
    glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &format.Swizzles[0]);
    spdlog::info("Texture size: ({}, {}) - {}", tex.extent().x, tex.extent().y, synthium::utils::human_bytes(tex.size()));

    switch(target) {
    case GL_TEXTURE_1D:
        glTexStorage1D(target, tex.levels(), format.Internal, tex.extent().x);
        for(size_t level = 0; level < tex.levels(); level++) {
            glm::tvec3<GLsizei> extent(tex.extent(level));
            if(gli::is_compressed(tex.format())) {
                glCompressedTexSubImage1D(target, level, 0, extent.x, format.Internal, tex.size(level), tex.data(0, 0, level));
            } else {
                glTexSubImage1D(target, level, 0, extent.x, format.Internal, format.Type, tex.data(0, 0, level));
            }
        }
        break;
    case GL_TEXTURE_2D:
        glTexStorage2D(target, tex.levels(), format.Internal, tex.extent().x, tex.extent().y);
        for(size_t level = 0; level < tex.levels(); level++) {
            glm::tvec3<GLsizei> extent(tex.extent(level));
            if(gli::is_compressed(tex.format())) {
                glCompressedTexSubImage2D(target, level, 0, 0, extent.x, extent.y, format.Internal, tex.size(level), tex.data(0, 0, level));
            } else {
                glTexSubImage2D(target, level, 0, 0, extent.x, extent.y, format.Internal, format.Type, tex.data(0, 0, level));
            }
        }
        break;
    case GL_TEXTURE_3D:
        glTexStorage3D(target, tex.levels(), format.Internal, tex.extent().x, tex.extent().y, tex.extent().z);
        for(size_t level = 0; level < tex.levels(); level++) {
            glm::tvec3<GLsizei> extent(tex.extent(level));
            if(gli::is_compressed(tex.format())) {
                glCompressedTexSubImage3D(target, level, 0, 0, 0, extent.x, extent.y, extent.z, format.Internal, tex.size(level), tex.data(0, 0, level));
            } else {
                glTexSubImage3D(target, level, 0, 0, 0, extent.x, extent.y, extent.z, format.Internal, format.Type, tex.data(0, 0, level));
            }
        }
        break;
    case GL_TEXTURE_1D_ARRAY:
        glTexStorage2D(target, tex.levels(), format.Internal, tex.extent().x, tex.layers());
        for(size_t level = 0; level < tex.levels(); level++) {
            glm::tvec3<GLsizei> extent(tex.extent(level));
            if(gli::is_compressed(tex.format())) {
                glCompressedTexSubImage2D(target, level, 0, 0, extent.x, tex.layers(), format.Internal, tex.size(level), tex.data(0, 0, level));
            } else {
                glTexSubImage2D(target, level, 0, 0, extent.x, tex.layers(), format.Internal, format.Type, tex.data(0, 0, level));
            }
        }
        break;
    case GL_TEXTURE_2D_ARRAY:
        glTexStorage3D(target, tex.levels(), format.Internal, tex.extent().x, tex.extent().y, tex.layers());
        for(size_t level = 0; level < tex.levels(); level++) {
            glm::tvec3<GLsizei> extent(tex.extent(level));
            if(gli::is_compressed(tex.format())) {
                glCompressedTexSubImage3D(target, level, 0, 0, 0, extent.x, extent.y, tex.layers(), format.Internal, tex.size(level), tex.data(0, 0, level));
            } else {
                glTexSubImage3D(target, level, 0, 0, 0, extent.x, extent.y, tex.layers(), format.Internal, format.Type, tex.data(0, 0, level));
            }
        }
        break;
    case GL_TEXTURE_CUBE_MAP:{
        gli::texture_cube cube_tex = (gli::texture_cube)tex;
        glTexStorage2D(GL_TEXTURE_CUBE_MAP, cube_tex.levels(), format.Internal, cube_tex.extent().x, cube_tex.extent().y);

        glCheckError("Cubemap storage");
        for(size_t level = 0; level < tex.levels(); level++) {
            glm::tvec3<GLsizei> extent(tex.extent(level));
            if(gli::is_compressed(cube_tex.format())) {
                for(uint32_t i = 0; i < cube_tex.faces(); i++) {
                    glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, 0, 0, cube_tex[i].extent(level).x, cube_tex[i].extent(level).y, format.Internal, cube_tex[i].size(level), cube_tex[i].data(0, 0, level));
                }
            } else {
                for(uint32_t i = 0; i < cube_tex.faces(); i++) {
                    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, 0, 0, cube_tex[i].extent(level).x, cube_tex[i].extent(level).y, format.Internal, format.Type, cube_tex[i].data(0, 0, level));
                }
            }
            glCheckError("Cubemap SubImage " + std::to_string(level));
        }
    }
        break;
    }
    glCheckError("Texture loaded");

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
    glActiveTexture(GL_TEXTURE1 + unit);
    glBindTexture(m_target, m_texture);
}

void Texture::unbind() {
    int32_t unit = get_unit();
    if(unit < 0) {
        spdlog::warn("Attempted to unbind texture of unknown semantic");
        return;
    }
    glActiveTexture(GL_TEXTURE1 + unit);
    glBindTexture(m_target, 0);
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
    case Parameter::WarpgateSemantic::EMISSIVE:
        return 5;
    default:
        return -1;
    }
}