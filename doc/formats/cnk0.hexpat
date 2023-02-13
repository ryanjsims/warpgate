#pragma array_limit 131072
#pragma pattern_limit 1048576

struct ChunkHeader {
    char magic[4];
    u32 version;
};

struct Layer {
    u32 unk1, unk2;
};

struct Vertex {
    s16 x, y, height_far, height_near;
    u32 color1, color2;
};

struct RenderBatch {
    u32 index_offset, index_count, vertex_offset, vertex_count;
};

struct OptimizedDraw {
    u32 data[80];
    //u8 data[320];
};

struct TileOccluderInfo {
    u8 data[64];
};

struct Unknown {
    s16 short_;
    u8 byte1, byte2;
};

struct Vector3 {
    float x, y, z;
};

struct Flora {
    u32 layer_count;
    Layer layers[layer_count];
};

struct Eco {
    u32 id, flora_count;
    Flora floras[flora_count];
};

struct Tile {
    s32 x, y, unk1, unk2;
    u32 eco_count;
    Eco ecos[eco_count];
    u32 index, image_id;
    if(image_id != 0) {
        u32 image_length;
        u8 image_data[image_length];
    }
    u32 layer_length;
    u8 layer_textures[layer_length];
};

struct CNK0 {
    ChunkHeader header;
    u32 tile_count;
    Tile tiles[tile_count];
    
    u32 unk1, unk_array_len;
    Unknown unk_array[unk_array_len];
    
    u32 index_count;
    u16 indices[index_count];
    
    u32 vertex_count;
    Vertex vertices[vertex_count];
    
    u32 render_batch_count;
    RenderBatch render_batches[render_batch_count];
    
    u32 optimized_draw_count;
    OptimizedDraw optimized_draws[optimized_draw_count];
    
    u32 unk_shorts_count;
    u16 unk_shorts[unk_shorts_count];
    
    u32 unk_vectors_count;
    Vector3 unk_vectors[unk_vectors_count];
    
    u32 tile_occluder_info_count;
    TileOccluderInfo tile_occluder_infos[15];
};

CNK0 chunk @0x00;
