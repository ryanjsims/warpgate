#include <std/mem.pat>

//These values are not correct for old MRN files

enum PacketType : u32 {
    skeleton = 0x01,
    some_other_data = 0x02,
    name2 = 0x03,
    data = 0x07,
    animation_data = 0x0A, // not sure but it has some names in it that sound like it
    name = 0x0C,
    file_names = 0x0E, // same as above - has some file name data in it alongside other stuff.
    vehicle_names = 0x0F,
    indexed_data = 0x10, // Data that has an index rather than namehash in the header
};

struct header {
    u32 magic[2];
    PacketType type;
    u32 index_or_namehash;
    u32 namehashes[4];
    u32 datalength;
    // Header alignment appears to be either 4 or 16 bytes
    u32 header_alignment;
    // Pad the header to align the data
    u8 pad[$%header_alignment != 0 ? (header_alignment - ($ % header_alignment)) : 0];
};

u8 magic @0x00;
// Older version of MRNs used magic of 0x18 with 4 byte alignment
// Newer x64 MRNs use magic of 0x1A with 16 byte alignment
u8 packet_alignment = magic == 0x18 ? 4 : 16;

struct quaternion {
    float x, y, z, w;
};

struct vector4 {
    float x, y, z, w;
};

struct BoneHierarchyEntry {
    s32 unknown, parent_index, start_index, chain_length;
};

struct bone_name {
    char name[];
} [[inline]];

struct orientation_header {
    u32 header_size;
    u32 unknown1;
    padding[8];
    u32 total_length;
    u32 unknown2;
    u32 unknown3;
    bool unknown_bool;
    padding[3];
    u32 unknown4;
    padding[4];
    u64 arr_len_bytes;
    u64 unknown5;
    s32 unknown_array[arr_len_bytes / 4];
};

struct orientation_data {
    orientation_header header;
    padding[12];
    vector4 offsets[parent.bone_count];
    padding[while(std::mem::read_unsigned($, 1) == 0xCD)];
    quaternion rotations[parent.bone_count];
};

struct skeleton_data {
    quaternion rotation;
    vector4 position;
    u32 chain_count;
    padding[4];
    u32 unknown_array1[11];
    padding[4];
    u32 unknown_array2[3];
    padding[4];
    u32 unknown_array3[4];
    padding[8];
    BoneHierarchyEntry chains[chain_count];
    u32 bone_count;
    padding[4];
    u32 unknown_array4[3];
    u32 indices[bone_count];
    u32 unknown_array5[bone_count + 5];
    bone_name bone_names[bone_count];
    padding[while(std::mem::read_unsigned($, 1) == 0xCD)];
    u32 magic;
    padding[4];
    orientation_data orientations;
};

struct packet {
    header head;
    if(head.type == PacketType::skeleton) {
        skeleton_data skeleton;
        u8 data[head.datalength - sizeof(skeleton)];
    } else {
        u8 data[head.datalength];
    }
    // Packets are always aligned to 4 (.mrn) or 16 (x64.mrn) bytes
    u8 align[$%packet_alignment != 0 ? packet_alignment - $ % packet_alignment : 0];
};

packet p[while($ < sizeof($))] @0x00;
