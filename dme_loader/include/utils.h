#pragma once
#include <string>
#include "vertexstream.h"

namespace utils {
    std::string uppercase(const std::string input);
    std::string relabel_texture(std::string texture_name, std::string label);

    void normalize(float vector[3]);
    void load_vector(std::string vector_type, uint32_t vertex_offset, uint32_t entry_offset, VertexStream &vertices, float vector[3]);
}