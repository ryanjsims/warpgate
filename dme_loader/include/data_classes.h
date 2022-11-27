#pragma once
#include <span>
#include <fstream>
#include <exception>
#include <vector>
#include <memory>
#include <cstring>
#include <spdlog/spdlog.h>

struct Bone;

struct PackedMat4 {
    float data[4][3];
};

struct BoneMapEntry {
    uint16_t bone_index, global_index;
};

struct DrawCall {
    uint32_t unk0, bone_start, bone_count, delta, unk1, vertex_offset, vertex_count, index_offset, index_count;
};

struct VertexStream {
    mutable std::span<uint8_t> buf_;

    VertexStream(std::span<uint8_t> span): buf_(span) {}

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("DME: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }
};

struct Mesh {
    mutable std::span<uint8_t> buf_;

    Mesh(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Mesh: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }

    static size_t check_size(std::span<uint8_t> subspan) {
        Mesh mesh(subspan);
        size_t vertex_data_length = 0;
        for(uint32_t i = 0; i < mesh.vertex_stream_count(); i++) {
            vertex_data_length += mesh.vertex_count() * mesh.bytes_per_vertex(i);
        }
        return 8 * sizeof(uint32_t) + mesh.vertex_stream_count() * sizeof(uint32_t) + vertex_data_length;
    }

    ref<uint32_t> draw_offset() const;
    ref<uint32_t> draw_count() const;
    ref<uint32_t> bone_count() const;
    ref<uint32_t> unknown() const;
    ref<uint32_t> vertex_stream_count() const;
    ref<uint32_t> index_size() const;
    ref<uint32_t> index_count() const;
    ref<uint32_t> vertex_count() const;
    ref<uint32_t> bytes_per_vertex(uint32_t vertex_stream_index) const;
    std::span<uint8_t> vertex_stream(uint32_t vertex_stream_index) const;
    std::span<uint8_t> index_data() const;

private:
    std::vector<size_t> vertex_stream_offsets;
    size_t vertex_data_size;
    size_t index_offset() const;
};

struct AABB {
    struct {
        float x, y, z;
    } min, max;
};

struct Material {
    mutable std::span<uint8_t> buf_;

    Material(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Material: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }
    
    ref<uint32_t> namehash() const;
    ref<uint32_t> length() const;
    ref<uint32_t> definition() const;
    ref<uint32_t> param_count() const;
    std::span<uint8_t> param_data() const;
};

struct DMAT {
    mutable std::span<uint8_t> buf_;

    DMAT(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("DMAT: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }

    ref<uint32_t> magic() const;
    ref<uint32_t> version() const;
    ref<uint32_t> filenames_length() const;
    std::span<uint8_t> texturename_data() const;
    const std::vector<std::string> textures() const;

    ref<uint32_t> material_count() const;
    std::shared_ptr<const Material> material(uint32_t index) const;

private:
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::string> texture_names;
    size_t materials_size;
    uint32_t material_offset() const;
    void parse_filenames();
    void parse_materials();
};

struct DME {
    mutable std::span<uint8_t> buf_;

    DME(uint8_t *data, size_t count);
    DME(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("DME: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }

    ref<uint32_t> magic() const;
    ref<uint32_t> version() const;

    ref<uint32_t> dmat_length() const;
    std::shared_ptr<const DMAT> dmat() const;

    ref<AABB> aabb() const;

    ref<uint32_t> mesh_count() const;
    std::shared_ptr<const Mesh> mesh(uint32_t index) const;

    ref<uint32_t> drawcall_count() const;
    std::span<DrawCall> drawcalls() const;

    ref<uint32_t> bme_count() const;
    std::span<BoneMapEntry> bone_map() const;

    ref<uint32_t> bone_count() const;


private:
    std::shared_ptr<DMAT> dmat_ = nullptr;
    std::vector<std::shared_ptr<Mesh>> meshes;
    size_t meshes_size;

    uint32_t aabb_offset() const;
    uint32_t bonemap_offset() const;
    uint32_t dmat_offset() const;
    uint32_t drawcall_offset() const;
    uint32_t meshes_offset() const;
    void parse_meshes();
    void parse_dmat();
};