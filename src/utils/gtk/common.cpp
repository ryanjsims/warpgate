#include "utils/gtk/common.hpp"

#include <spdlog/spdlog.h>

GLenum glCheckError(std::string name) {
    GLenum error = glGetError();
    if(error != GL_NO_ERROR) {
        switch(error) {
        case GL_INVALID_ENUM:
            spdlog::error("{}: Invalid enum (code {})", name, error);
            break;
        case GL_INVALID_VALUE:
            spdlog::error("{}: Invalid value (code {})", name, error);
            break;
        case GL_INVALID_OPERATION:
            spdlog::error("{}: Invalid operation (code {})", name, error);
            break;
        case GL_STACK_OVERFLOW:
            spdlog::error("{}: Stack overflow (code {})", name, error);
            break;
        case GL_STACK_UNDERFLOW:
            spdlog::error("{}: Stack underflow (code {})", name, error);
            break;
        case GL_OUT_OF_MEMORY:
            spdlog::error("{}: Out of memory (code {})", name, error);
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            spdlog::error("{}: Invalid framebuffer operation (code {})", name, error);
            break;
        }
    }
    return error;
}