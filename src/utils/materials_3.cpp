#include "utils/materials_3.h"

namespace logger = spdlog;

#ifdef WIN32
#include <windows.h>

std::optional<std::filesystem::path> executable_location() {
    char location[512];
    DWORD rc = GetModuleFileNameA(nullptr, location, 512);
    DWORD error = GetLastError();
    if(rc == 512 && error == ERROR_INSUFFICIENT_BUFFER || rc == 0) {
        logger::error("Failed to get executable location: error code {}", error);
        return {};
    }
    return std::filesystem::path(location);
}
#else
std::optional<std::filesystem::path> executable_location() {
    return std::filesystem::canonical("/proc/self/exe");
}
#endif

using namespace warpgate;

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
    {"Float1", 4},
    {"Float2", 8},
    {"Float3", 12},
    {"Float4", 16},
    {"D3dcolor", 4},
    {"ubyte4n", 4},
    {"Float16_2", 4},
    {"float16_2", 4},
    {"Short2", 4},
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
    std::filesystem::path material_location = MATERIALS_JSON_LOCATION;
    #if MATERIALS_JSON_PORTABLE
        material_location = (*executable_location()).parent_path() / material_location;
    #endif
    spdlog::debug("Loading materials.json from {}", material_location.string());
    std::ifstream materials_file(material_location);
    if(materials_file.fail()) {
        spdlog::error("Could not open materials.json: looking at location {}", material_location.string());
        std::exit(1);
    }
    materials = nlohmann::json::parse(materials_file);
}

std::optional<nlohmann::json> utils::materials3::get_input_layout(uint32_t material_definition) {
    if(!materials.at("materialDefinitions").contains(std::to_string(material_definition))) {
        return {};
    }
    
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