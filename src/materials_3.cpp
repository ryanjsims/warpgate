#include "utils/materials_3.h"

std::vector<std::string> utils::materials3::detailcube_faces = {"+x", "-x", "+y", "-y", "+z", "-z"};

std::unordered_map<std::string, std::string> utils::materials3::usages = {
    {"Position", "POSITION"},
    {"Normal", "NORMAL"},
    {"Binormal", "BINORMAL"},
    {"Tangent", "TANGENT"},
    {"BlendWeight", "WEIGHTS_0"},
    {"BlendIndices", "JOINTS_0"},
    {"Texcoord", "TEXCOORD_"},
    {"Color", "COLOR_"},
};

std::unordered_map<std::string, int> utils::materials3::sizes = {
    {"Float3", 12},
    {"D3dcolor", 4},
    {"Float2", 8},
    {"Float4", 16},
    {"ubyte4n", 4},
    {"Float16_2", 4},
    {"float16_2", 4},
    {"Short2", 4},
    {"Float1", 4},
    {"Short4", 8}
};

std::unordered_map<std::string, int> utils::materials3::component_types = {
    {"Float3", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"D3dcolor", TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE},
    {"Float2", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"Float4", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"ubyte4n", TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE},
    {"Float16_2", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"float16_2", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"Short2", TINYGLTF_COMPONENT_TYPE_SHORT},
    {"Float1", TINYGLTF_COMPONENT_TYPE_FLOAT},
    {"Short4", TINYGLTF_COMPONENT_TYPE_SHORT}
};

std::unordered_map<std::string, int> utils::materials3::types = {
    {"Float3", TINYGLTF_TYPE_VEC3},
    {"D3dcolor", TINYGLTF_TYPE_VEC4},
    {"Float2", TINYGLTF_TYPE_VEC2},
    {"Float4", TINYGLTF_TYPE_VEC4},
    {"ubyte4n", TINYGLTF_TYPE_VEC4},
    {"Float16_2", TINYGLTF_TYPE_VEC2},
    {"float16_2", TINYGLTF_TYPE_VEC2},
    {"Short2", TINYGLTF_TYPE_VEC2},
    {"Float1", TINYGLTF_TYPE_SCALAR},
    {"Short4", TINYGLTF_TYPE_VEC4}
};

void utils::materials3::init_materials() {
    std::ifstream materials_file(MATERIALS_JSON_LOCATION);
    if(materials_file.fail()) {
        spdlog::error("Could not open materials.json: looking at location {}", MATERIALS_JSON_LOCATION);
        std::exit(1);
    }
    materials = nlohmann::json::parse(materials_file);
}

nlohmann::json utils::materials3::get_input_layout(uint32_t material_definition) {
    std::string input_layout_name = materials
        .at("materialDefinitions")
        .at(std::to_string(material_definition))
        .at("drawStyles")
        .at(0)
        .at("inputLayout")
        .get<std::string>();
    
    return materials.at("inputLayouts").at(input_layout_name);
}

nlohmann::json utils::materials3::materials;