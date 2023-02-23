#include <std/mem.pat>
#include <std/ptr.pat>

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
    // Maybe hashes? No real clue. These are identical across all 'indexed_data' packets within one file.
    // Other packets in the file have these values zeroed out.
    u32 unknown[4];
    u32 datalength;
    // Header alignment appears to be either 4 or 16 bytes
    u32 header_alignment;
    if($ % header_alignment != 0) {
        // Pad the header to align the data
        padding[header_alignment - ($ % header_alignment)];
    }
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
    if($ % 16 != 0) {
        padding[16 - ($ % 16)];
    }
    vector4 offsets[parent.bone_count];
    if(sizeof(offsets) % 64 != 0) {
        padding[64 - (sizeof(offsets) % 64)];
    }
    quaternion rotations[parent.bone_count];
    if(sizeof(rotations) % 64 != 0) {
        padding[64 - (sizeof(rotations) % 64)];
    }
};

struct skeleton_data {
    quaternion rotation;
    vector4 position;
    u32 chain_count;
    padding[4];
    u8* unknown_ptr : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u32* ptr_bone_count : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u32 unknown_array1[2];
    u8* unknown_ptr2 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u8* unknown_ptr3 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
	u32 unknown_int;
    padding[4];
    u8* ptr_end_data[96] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u32 unknown_int2;
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

struct u16_array {
    u16 length;
    u16 values[length];
};

 fn relative_to_parents_parent(u128 offset) {
    return addressof(parent.parent);
 };

// Pointer that checks if the offset is valid before dereferencing
// Also needed so that an array of pointers can be made with imhex
struct u16_array_ptr {
    if(std::mem::read_unsigned($, 8) != 0x00) {
        u16_array *ptr: u64 [[pointer_base("relative_to_parents_parent")]];
    } else {
        u64 null;
    }
};

struct float6_array_ptr {
    if(std::mem::read_unsigned($, 8) != 0x00) {
        float *ptr[6]: u64 [[pointer_base("relative_to_parents_parent")]];
    } else {
        u64 null;
    }
} [[inline]];

struct deq_factors {
    float factors[6];
};

struct translation_deq_factors {
    deq_factors array[parent.translation_anim_deq_count];
};

struct rotation_deq_factors {
    deq_factors array[parent.rotation_anim_deq_count];
};

struct scale_deq_factors {
    deq_factors array[parent.scale_anim_deq_count];
};

struct indexed_data_first_segment {
    u32 translation_bone_count;
    u32 rotation_bone_count;
    u32 scale_bone_count;
    deq_factors translation_factors;
    deq_factors rotation_factors;
    if(scale_bone_count > 0) {
    	deq_factors scale_factors;
    }
    padding[4];
    u64 unknown;
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	u8 *translation_data[6 * translation_bone_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_translation_data;
    }
    
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	u8 *rotation_data[6 * rotation_bone_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_rotation_data;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
        u8 *scale_data[6 * scale_bone_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_scale_data;
    }
};

struct rotation_bone_data {
    u8 data[6];
};

struct rotation_frame_data {
    rotation_bone_data data[parent.rotation_bone_count];
};

struct indexed_data_second_segment {
    u32 frames; // Not 100%
    u32 translation_bone_count;
    u32 rotation_bone_count;
    u32 scale_bone_count;
    if(std::mem::read_unsigned($, 8) != 0x00) {
	    u8 *translation_data[frames * translation_bone_count * 4] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
	    u8 *translation_factors : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_translation_data;
        u64 null_translation_factors;
    }
    
    if(std::mem::read_unsigned($, 8) != 0x00) {
	    rotation_frame_data *rotation_data[frames * rotation_bone_count * 6] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
	    u8 *rotation_factors : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_rotation_data;
        u64 null_rotation_factors;
    }
    
    if(std::mem::read_unsigned($, 8) != 0x00) {
        u8 *scale_data[frames * scale_bone_count * 4] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
        u8 *scale_factors : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_scale_data;
        u64 null_scale_factors;
    }
};

struct indexed_data {
    be u32 crc32hash; // CRC32 of the rest of the data
    u32 version;   // Maybe?
    padding[8];    // Looks to be just 0 in the files I've seen so far
    u32 unknown1, unknown2;
    float unknown_float1, unknown_float2;
    u32 unknown3, unknown4;
    u16_array_ptr static_translation_bone_indices;
    u16_array_ptr static_rotation_bone_indices;
    u16_array_ptr static_scale_bone_indices;
    u16_array_ptr dynamic_translation_bone_indices;
    u16_array_ptr dynamic_rotation_bone_indices;
    u16_array_ptr dynamic_scale_bone_indices;
    float translation_start_pos_factors[6];
    padding[8];
    u32 translation_anim_deq_count, rotation_anim_deq_count, scale_anim_deq_count;
    padding[4];
    if(std::mem::read_unsigned($, 8) != 0x00) {
        translation_deq_factors *translation_factors: u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_tr;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
        rotation_deq_factors *rotation_factors: u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_rot;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
        scale_deq_factors *scale_factors: u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_scl;
    }
	
	if(std::mem::read_unsigned($, 8) != 0x00) {
    	indexed_data_first_segment *static_data : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_static_data;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	indexed_data_second_segment *dynamic_data : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_dynamic_data;
    }
    u8 *end_data[96] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    padding[8];
};

struct packet {
    header head;
    if(head.type == PacketType::skeleton) {
        skeleton_data skeleton;
        u8 data[head.datalength - sizeof(skeleton)];
    } else if(head.type == PacketType::indexed_data) {
        indexed_data indexed;
        u8 data[head.datalength - sizeof(indexed)];
    } else {
        u8 data[head.datalength];
    }
    // Packets are always aligned to 4 (.mrn) or 16 (x64.mrn) bytes
    u8 align[$%packet_alignment != 0 ? packet_alignment - $ % packet_alignment : 0];
};

packet p[while($ < sizeof($))] @0x00;
