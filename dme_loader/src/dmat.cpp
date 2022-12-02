#include "dmat.h"
#include <spdlog/spdlog.h>

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