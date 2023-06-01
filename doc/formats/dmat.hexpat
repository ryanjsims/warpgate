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

DMAT dmat @0x00;