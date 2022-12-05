#pragma once
#include <string>
#include "vertexstream.h"

namespace utils {
    std::string uppercase(const std::string input);

    void normalize(float vector[3]);
    void load_vector(std::string vector_type, uint32_t vertex_offset, uint32_t entry_offset, VertexStream &vertices, float vector[3]);
}