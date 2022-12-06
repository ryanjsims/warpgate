#include "utils.h"
#include <algorithm>
#include <cmath>

using namespace warpgate;

std::string utils::uppercase(const std::string input) {
    std::string temp = input;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](const char value) -> char {
        if(value >= 'a' && value <= 'z') {
            return value - (char)('a' - 'A');
        }
        return value;
    });
    return temp;
}

void utils::normalize(float vector[3]) {
    float length = std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
    if(std::fabsf(length) > 0) {
        vector[0] /= length;
        vector[1] /= length;
        vector[2] /= length;
    }
}

void utils::load_vector(std::string vector_type, uint32_t vertex_offset, uint32_t entry_offset, VertexStream &vertices, float vector[3]) {
    if(vector_type == "ubyte4n") {
        vector[0] = ((float)vertices.get<uint8_t>(vertex_offset + entry_offset) / 255.0f * 2) - 1;
        vector[1] = ((float)vertices.get<uint8_t>(vertex_offset + entry_offset + 1) / 255.0f * 2) - 1;
        vector[2] = ((float)vertices.get<uint8_t>(vertex_offset + entry_offset + 2) / 255.0f * 2) - 1;
    } else {
        vector[0] = vertices.get<float>(vertex_offset + entry_offset);
        vector[1] = vertices.get<float>(vertex_offset + entry_offset + 4);
        vector[2] = vertices.get<float>(vertex_offset + entry_offset + 8);
    }
}