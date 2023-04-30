#pragma pattern_limit 2097152

struct Offsets {
    u32 ecos, floras, invis_walls, objects, lights, unknown;
};

struct ChunkInfo {
    u32 tile_count;
    s32 start_x, start_y;
    u32 count_x, count_y;
};

struct Color4 {
    u8 r, g, b, a;
};

struct Color4ARGB {
    u8 a, r, g, b;
};

struct EcoTint {
    Color4 color_rgba;
    u32 strength;
};

struct fMinMax {
    float min, max;
};

struct Float2 {
    float x, y;
};

struct Float4 {
    float x, y, z, w;
};

struct Rotation {
    float y, x, z, w;
};

struct FloatMapEntry {
    u32 key;
    float value;
};

struct InvisWall {
    // No layout info known :(
};

enum LightType: u32 {
    Point = 1,
    Spot
};

struct PerTileInfo {
    u32 quad_count;
    float width, height;
    u32 vertex_count;
};

struct UIntMapEntry {
    u32 key, value;
};

struct Vector4MapEntry {
    u32 key;
    Float4 value;
};

struct ZoneHeader {
    char magic_[4];
    u32 version;
    Offsets offsets;
    PerTileInfo per_tile;
    ChunkInfo chunk_info;
};

struct TextureInfo {
    char name[];
    char cnx_name[];
    char sbny_name[];
    u32 detail_repeat;
    float blend_strength;
    fMinMax specular, smoothness;
    char physics_mat_name[];
};

struct EcoLayer {
    float density;
    fMinMax scale;
    float slope_peak, slope_extent;
    fMinMax elevation;
    u8 min_alpha;
    char flora_name[];
    u32 tint_count;
    EcoTint tints[tint_count];
};

struct FloraInfo {
    u32 layer_count;
    EcoLayer layers[layer_count];
};

struct Eco {
    u32 index;
    TextureInfo textureInfo;
    FloraInfo floraInfo;
};

struct Flora {
    char name[];
    char texture[];
    char model[];
    bool unk_bool;
    float unk_float0, unk_float1;
};

struct Instance {
    Float4 translation;
    Rotation rotation;
    Float4 scale;
    u32 unk_int;
    u8 unk_byte1, unk_byte2;
    float unk_float;
};

struct RuntimeObject {
    char actorFile[];
    float renderDistance;
    u32 instance_count;
    Instance instances[instance_count];
};

struct Light {
    char name[];
    char color_name[];
    LightType type;
    bool unk_bool;
    Float4 translation;
    Rotation rotation;
    Float2 unk_floats;
    Color4ARGB color;
    u8 unk_data1[5];
    Float4 unk_vector;
    u8 unk_data2[5];
};

struct Zone {
    ZoneHeader header;
    u32 eco_count;
    Eco ecos[eco_count];
    
    u32 flora_count;
    Flora floras[flora_count];
    
    u32 invis_walls_count;
    InvisWall invis_walls[invis_walls_count];
    
    u32 objects_count;
    RuntimeObject objects[objects_count];
    
    u32 lights_count;
    Light lights[lights_count];
    
    u32 unknowns_count;
    u8 unknown_data[unknowns_count];
};

Zone zone @0x00;
