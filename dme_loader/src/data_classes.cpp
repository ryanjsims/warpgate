#include "data_classes.h"

DME::DME(uint8_t *data, size_t count): buf_(data, count) {
    parse_dmat();
    parse_meshes();
}

DME::DME(std::span<uint8_t> subspan): buf_(subspan) {
    parse_dmat();
    parse_meshes();
}

void DME::parse_dmat() {
    dmat_ = std::make_shared<DMAT>(buf_.subspan(dmat_offset(), dmat_length()));
}

void DME::parse_meshes() {
    uint32_t mesh_count = this->mesh_count();
    meshes_size = 0;
    for(uint32_t i = 0; i < mesh_count; i++) {
        spdlog::info("Loading mesh: {}", i);
        auto mesh = std::make_shared<Mesh>(buf_.subspan(meshes_offset() + 4 + meshes_size));
        meshes_size += mesh->size();
        meshes.push_back(mesh);
        spdlog::info("Loaded mesh {}", i);
    }
}

DME::ref<uint32_t> DME::magic() const { return get<uint32_t>(0); }

DME::ref<uint32_t> DME::version() const { return get<uint32_t>(4); }

uint32_t DME::dmat_offset() const { return 12; }

DME::ref<uint32_t> DME::dmat_length() const { return get<uint32_t>(8); }

std::shared_ptr<const DMAT> DME::dmat() const {
    return dmat_;
}

uint32_t DME::aabb_offset() const {
    return dmat_offset() + dmat_length();
}

DME::ref<AABB> DME::aabb() const {
    return get<AABB>(aabb_offset());
}

uint32_t DME::meshes_offset() const {
    return aabb_offset() + sizeof(AABB);
}

DME::ref<uint32_t> DME::mesh_count() const {
    return get<uint32_t>(meshes_offset());
}

std::shared_ptr<const Mesh> DME::mesh(uint32_t index) const {
    return meshes.at(index);
}

uint32_t DME::drawcall_offset() const {
    return meshes_offset() + meshes_size;
}

DME::ref<uint32_t> DME::drawcall_count() const {
    return get<uint32_t>(drawcall_offset());
}

std::span<DrawCall> DME::drawcalls() const {
    std::span<uint8_t> data = buf_.subspan(drawcall_offset() + 4, drawcall_count() * sizeof(DrawCall));
    return std::span<DrawCall>(reinterpret_cast<DrawCall*>(data.data()), drawcall_count());
}

uint32_t DME::bonemap_offset() const {
    return drawcall_offset() + 4 + drawcall_count() * sizeof(DrawCall);
}

DME::ref<uint32_t> DME::bme_count() const {
    return get<uint32_t>(bonemap_offset());
}

std::span<BoneMapEntry> DME::bone_map() const {
    std::span<uint8_t> data = buf_.subspan(bonemap_offset() + 4, bme_count() * sizeof(BoneMapEntry));
    return std::span<BoneMapEntry>(reinterpret_cast<BoneMapEntry*>(data.data()), bme_count());
}

DME::ref<uint32_t> DME::bone_count() const {
    return get<uint32_t>(bonemap_offset() + 4 + bme_count() * sizeof(BoneMapEntry));
}

DMAT::DMAT(std::span<uint8_t> subspan): buf_(subspan) {
    parse_materials();
    parse_filenames();
}

DMAT::ref<uint32_t> DMAT::magic() const {
    return get<uint32_t>(0);
}

DMAT::ref<uint32_t> DMAT::version() const {
    return get<uint32_t>(4);
}

DMAT::ref<uint32_t> DMAT::filenames_length() const {
    return get<uint32_t>(8);
}

std::span<uint8_t> DMAT::texturename_data() const {
    return buf_.subspan(12, filenames_length());
}

const std::vector<std::string> DMAT::textures() const {
    return texture_names;
}

uint32_t DMAT::material_offset() const {
    return 12 + filenames_length();
}

DMAT::ref<uint32_t> DMAT::material_count() const {
    return get<uint32_t>(material_offset());
}

std::shared_ptr<const Material> DMAT::material(uint32_t index) const {
    return materials.at(index);
}

void DMAT::parse_filenames() {
    std::span<uint8_t> filenames = texturename_data();
    for(
        std::string filename = std::string((char*)filenames.data()); 
        !filenames.empty(); 
        filenames = filenames.subspan(filename.size() + 1), filename = std::string((char*)filenames.data())
    ) {
        texture_names.push_back(filename);
    }
}

void DMAT::parse_materials() {
    materials_size = 0;
    uint32_t material_count = this->material_count();
    spdlog::info("Parsing {} material{}", material_count, material_count != 1 ? "s" : "");
    for(size_t i = 0; i < material_count; i++) {
        std::shared_ptr<Material> material = std::make_shared<Material>(buf_.subspan(material_offset() + 4 + materials_size));
        materials.push_back(material);
        materials_size += material->size();
    }
}

Material::Material(std::span<uint8_t> subspan): buf_(subspan) {
    buf_ = buf_.first(length() + 8);
}

Material::ref<uint32_t> Material::namehash() const {
    return get<uint32_t>(0);
}

Material::ref<uint32_t> Material::length() const {
    return get<uint32_t>(4);
}

Material::ref<uint32_t> Material::definition() const {
    return get<uint32_t>(8);
}

Material::ref<uint32_t> Material::param_count() const {
    return get<uint32_t>(12);
}

std::span<uint8_t> Material::param_data() const {
    return buf_.subspan(16, length() - 8);
}


Mesh::Mesh(std::span<uint8_t> subspan): buf_(subspan) {
    vertex_data_size = 0;
    uint32_t vertex_stream_count = this->vertex_stream_count();
    uint32_t vertex_count = this->vertex_count();
    spdlog::info("Loading mesh with vertex_stream_count = {} and vertex count = {}", vertex_stream_count, vertex_count);
    for(uint32_t i = 0; i < vertex_stream_count; i++) {
        spdlog::info("Loading vertex stream: {}...", i);
        vertex_stream_offsets.push_back(32 + 4 * i + vertex_data_size);
        vertex_data_size += bytes_per_vertex(i) * vertex_count;
        spdlog::info("Loaded vertex stream {}", i);
    }
    buf_ = buf_.first(index_offset() + index_count() * (index_size() & 0xFF));
}

Mesh::ref<uint32_t> Mesh::draw_offset() const {
    return get<uint32_t>(0);
}

Mesh::ref<uint32_t> Mesh::draw_count() const {
    return get<uint32_t>(4);
}

Mesh::ref<uint32_t> Mesh::bone_count() const {
    return get<uint32_t>(8);
}

Mesh::ref<uint32_t> Mesh::unknown() const {
    return get<uint32_t>(12);
}

Mesh::ref<uint32_t> Mesh::vertex_stream_count() const {
    return get<uint32_t>(16);
}

Mesh::ref<uint32_t> Mesh::index_size() const {
    return get<uint32_t>(20);
}

Mesh::ref<uint32_t> Mesh::index_count() const {
    return get<uint32_t>(24);
}

Mesh::ref<uint32_t> Mesh::vertex_count() const {
    return get<uint32_t>(28);
}

Mesh::ref<uint32_t> Mesh::bytes_per_vertex(uint32_t vertex_stream_index) const {
    return get<uint32_t>(vertex_stream_offsets[vertex_stream_index]);
}

std::span<uint8_t> Mesh::vertex_stream(uint32_t vertex_stream_index) const {
    return buf_.subspan(vertex_stream_offsets[vertex_stream_index] + 4, bytes_per_vertex(vertex_stream_index) * vertex_count());
}

size_t Mesh::index_offset() const {
    return 32 + 4 * vertex_stream_count() + vertex_data_size;
}

std::span<uint8_t> Mesh::index_data() const {
    return buf_.subspan(index_offset(), index_count() * (index_size() & 0xFF));
}
