#include "utils/gtk/mesh.hpp"

#include <utils/common.h>
#include <utils/materials_3.h>

std::unordered_map<std::string, int> component_types = {
    {"Float3", GL_FLOAT},
    {"D3dcolor", GL_UNSIGNED_BYTE},
    {"Float2", GL_FLOAT},
    {"Float4", GL_FLOAT},
    {"ubyte4n", GL_UNSIGNED_BYTE},
    {"Float16_2", GL_HALF_FLOAT},
    {"float16_2", GL_HALF_FLOAT},
    {"Short2", GL_SHORT},
    {"Float1", GL_FLOAT},
    {"Short4", GL_SHORT}
};

using namespace warpgate::gtk;

GLboolean get_normalized(std::string type, std::string usage) {
    if(type == "Float1") {
        return GL_FALSE;
    } else if(type == "Float2") {
        return GL_FALSE;
    } else if(type == "Float3") {
        return GL_FALSE;
    } else if(type == "Float4") {
        return GL_FALSE;
    } else if(type == "D3dcolor") {
        if(usage == "BlendIndices") {
            return GL_FALSE;
        } else {
            return GL_TRUE;
        }
    } else if(type == "ubyte4n") {
        return GL_TRUE;
    } else if(type == "Float16_2" || type == "float16_2") {
        return GL_FALSE;
    } else if(type == "Short2") {
        return GL_TRUE;
    } else if(type == "Short4") {
        return GL_TRUE;
    }

    return GL_FALSE;
}

Mesh::Mesh(
    std::shared_ptr<const warpgate::Mesh> mesh,
    std::shared_ptr<const warpgate::Material> material,
    nlohmann::json layout,
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> &textures,
    std::shared_ptr<synthium::Manager> manager

) : m_index_count(mesh->index_count())
  , m_index_size(mesh->index_size() & 0xFF)
  , material_hash(layout["hash"])
  , material_name(layout["name"])
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::span<uint8_t> index_data = mesh->index_data();

    glGenBuffers(1, &indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_data.size(), index_data.data(), GL_STATIC_DRAW);

    vertex_streams.resize(mesh->vertex_stream_count());
    glGenBuffers(mesh->vertex_stream_count(), vertex_streams.data());
    for(uint32_t vs_index = 0; vs_index < mesh->vertex_stream_count(); vs_index++) {
        glBindBuffer(GL_ARRAY_BUFFER, vertex_streams[vs_index]);
        std::span<uint8_t> vs_data = mesh->vertex_stream(vs_index);
        glBufferData(GL_ARRAY_BUFFER, vs_data.size(), vs_data.data(), GL_STATIC_DRAW);
    }

    std::unordered_map<uint32_t, uint32_t> stream_strides;
        
    uint32_t pointer = 0;
    for(nlohmann::json entry : layout["entries"]) {
        uint32_t vs_index = entry["stream"];
        if(stream_strides.find(vs_index) == stream_strides.end()) {
            stream_strides[vs_index] = 0;
        }
        int32_t stride = mesh->bytes_per_vertex(vs_index);
        if(stream_strides[vs_index] >= stride) {
            continue;
        }

        uint32_t entry_size = utils::materials3::sizes[entry["type"]];
        int32_t entry_count = utils::materials3::types[entry["type"]];
        GLenum entry_type = component_types[entry["type"]];
        if(entry_type == GL_SHORT && entry["usage"] == "Texcoord") {
            entry_type = GL_UNSIGNED_SHORT;
        }
        if(entry_type == GL_UNSIGNED_BYTE && (
            entry["usage"] == "Binormal" ||
            entry["usage"] == "Normal" ||
            entry["usage"] == "Tangent" ||
            entry["usage"] == "Position"
        )) {
            entry_type = GL_BYTE;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vertex_streams[vs_index]);
        glEnableVertexAttribArray(pointer);
        GLboolean normalized = get_normalized(entry["type"], entry["usage"]);
        void* offset = (void*)stream_strides[vs_index];
        glVertexAttribPointer(pointer, entry_count, entry_type, normalized, stride, offset);
        vertex_layouts.push_back(Layout{vs_index, pointer, entry_count, entry_type, normalized, stride, offset});
        
        pointer++;
        stream_strides[vs_index] += entry_size;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    for(uint32_t param = 0; param < material->param_count(); param++) {
        const warpgate::Parameter &parameter = material->parameter(param);
        Parameter::D3DXParamType type = parameter.type();
        switch(type) {
        case warpgate::Parameter::D3DXParamType::TEXTURE1D:
        case warpgate::Parameter::D3DXParamType::TEXTURE2D:
        case warpgate::Parameter::D3DXParamType::TEXTURE3D:
        case warpgate::Parameter::D3DXParamType::TEXTURE:
        case warpgate::Parameter::D3DXParamType::TEXTURECUBE:
            break;
        default:
            continue;
        }


        Parameter::WarpgateSemantic semantic = Parameter::texture_common_semantic(parameter.semantic_hash());
        switch(semantic) {
        case Parameter::WarpgateSemantic::DIFFUSE:
        case Parameter::WarpgateSemantic::NORMAL:
        case Parameter::WarpgateSemantic::SPECULAR:
        case Parameter::WarpgateSemantic::DETAILCUBE:
        case Parameter::WarpgateSemantic::DETAILMASK:
            break;
        default:
            continue;
        }

        uint32_t texture_hash = parameter.get<uint32_t>(parameter.data_offset());
        m_texture_hashes.push_back(texture_hash);
        if(textures.find(texture_hash) != textures.end()) {
            continue;
        }

        std::optional<std::string> texture_name = material->texture(parameter.semantic_hash());
        if(!texture_name.has_value()) {
            continue;
        }

        std::shared_ptr<synthium::Asset2> asset = manager->get(*texture_name);
        std::vector<uint8_t> asset_data = asset->get_data();
        gli::texture texture = gli::load_dds((char*)asset_data.data(), asset_data.size());
        spdlog::info("Texture is {}compressed", gli::is_compressed(texture.format()) ? "" : "not ");
        textures[texture_hash] = std::make_shared<Texture>(texture, parameter.semantic_hash());
    }
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &indices);
    glDeleteBuffers(vertex_streams.size(), vertex_streams.data());
    glDeleteVertexArrays(1, &vao);
}

void Mesh::bind() const {
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    for(uint32_t i = 0; i < vertex_layouts.size(); i++) {
        const Layout& layout = vertex_layouts[i];
        glBindBuffer(GL_ARRAY_BUFFER, vertex_streams[layout.vs_index]);
        glEnableVertexAttribArray(layout.pointer);
        glVertexAttribPointer(layout.pointer, layout.count, layout.type, layout.normalized, layout.stride, layout.offset);
    }
}

void Mesh::unbind() const {
    for(uint32_t i = 0; i < vertex_layouts.size(); i++) {
        const Layout& layout = vertex_layouts[i];
        glDisableVertexAttribArray(layout.pointer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

std::vector<uint32_t> Mesh::get_texture_hashes() const {
    return m_texture_hashes;
}

uint32_t Mesh::get_material_hash() const {
    return material_hash;
}

uint32_t Mesh::index_count() const {
    return m_index_count;
}

uint32_t Mesh::index_size() const {
    return m_index_size;
}

std::pair<std::filesystem::path, std::filesystem::path> Mesh::get_shader_paths() const {
    std::optional<std::filesystem::path> exe_path = executable_location();
    if(!exe_path.has_value()) {
        spdlog::error("Could not find base directory to load shaders!");
        exe_path = std::filesystem::path("./warpgate.exe");
    }
    std::filesystem::path base_dir = exe_path->parent_path();
    std::filesystem::path resources = base_dir / "share";
    std::filesystem::path vertex, fragment;
    vertex = fragment = resources / "shaders" / material_name;
    vertex.replace_extension("vert");
    fragment.replace_extension("frag");
    return std::make_pair(vertex, fragment);
}