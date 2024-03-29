#include "semantic.hexpat"

struct vec3 {
    float x, y, z;
};

struct string {
    char data[];
} [[inline]];

enum D3DXParamClass : u32 {
    SCALAR,
    VECTOR,
    MATRIX_ROWS,
    MATRIX_COLS,
    OBJECT,
    STRUCT,
    FORCE_DWORD = 0x7fffffff
};

enum D3DXParamType : u32 {
    VOID,
    BOOL,
    INT,
    FLOAT,
    STRING,
    TEXTURE,
    TEXTURE1D,
    TEXTURE2D,
    TEXTURE3D,
    TEXTURECUBE,
    SAMPLER,
    SAMPLER1D,
    SAMPLER2D,
    SAMPLER3D,
    SAMPLERCUBE,
    PIXELSHADER,
    VERTEXSHADER,
    PIXELFRAGMENT,
    VERTEXFRAGMENT,
    UNSUPPORTED,
    FORCE_DWORD = 0x7fffffff
};

union SemanticHash {
    Semantic semantic;
    s32 hash;
};

struct Parameter {
    Semantic semantic;
    D3DXParamClass class;
    D3DXParamType type;
    u32 length;
    match (class, type) {
        (D3DXParamClass::SCALAR, D3DXParamType::INT): s32 value;
        (D3DXParamClass::VECTOR, D3DXParamType::INT): s32 value[length / 4];
        (D3DXParamClass::SCALAR, D3DXParamType::FLOAT): float value;
        (D3DXParamClass::VECTOR, D3DXParamType::FLOAT): float value[length / 4];
        (_, D3DXParamType::STRING): char value[length];
        (D3DXParamClass::SCALAR, D3DXParamType::BOOL): bool value;
        (D3DXParamClass::OBJECT, D3DXParamType::TEXTURE): u32 namehash;
        (_, _): u8 data[length];
    }
};

struct Material {
    u32 namehash, length, definition, param_count;
    Parameter parameters[param_count];
};

u32 start_offset;
struct DMAT {
    char magic[4];
    u32 version;
    u32 filenames_length;
    start_offset = $;
    string filenames[while($ - start_offset < filenames_length)];
    u32 material_count;
    Material materials[material_count];
};

struct AABB {
    vec3 min, max;
};

struct VertexStream {
    u32 vertex_size;
    u8 vertex_data[parent.vertex_count * vertex_size];
};

struct Mesh {
    u32 draw_offset, draw_count, bone_count, unknown, vertexstream_count, index_size, index_count, vertex_count;
    VertexStream vertex_streams[vertexstream_count];
    if((index_size & 0xFF) == 2) {
        u16 indices[index_count];
    } else if((index_size & 0xFF) == 4) {
        u32 indices[index_count];
    } else {
        u8 index_data[index_count * (index_size & 0xFF)];
    }
};

struct DrawCall {
    u32 unk0, bone_start, bone_count, delta, unk1, vertex_offset, vertex_count, index_offset, index_count;
};

struct BoneMapEntry {
    u16 bone_index, global_index;
};

struct PackedMat4 {
    vec3 rows[4];
};

struct Bone {
    PackedMat4 inverse_bind_matrix;
    AABB bounding_box;
    u32 namehash;
};

struct DME {
    char magic[4];
    u32 version;
    u32 dmat_length;
    DMAT material;
    AABB bounds;
    u32 mesh_count;
    Mesh meshes[mesh_count];
    u32 drawcall_count;
    DrawCall drawcalls[drawcall_count];
    u32 bonemapentry_count;
    BoneMapEntry boneMapEntries[bonemapentry_count];
    u32 bone_count;
    Bone bones[bone_count];
};

DME dme @ 0x00;