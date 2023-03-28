#pragma pattern_limit 16777216

#include <std/mem.pat>
#include <std/ptr.pat>
#include <std/io.pat>
//These values are not correct for old MRN files

enum PacketType : u32 {
    skeleton = 0x01,
    some_other_data = 0x02, // Maybe a network object? Header hashes are interesting
    name2 = 0x03,
    data = 0x07,
    network_data = 0x0A, // not sure but it has some names in it that sound like it
    name = 0x0C,
    file_names = 0x0E, // same as above - has some file name data in it alongside other stuff.
    skeleton_names = 0x0F,
    nsa_data = 0x10, // Data that has an index rather than namehash in the header
};

struct header {
    u32 magic[2];
    PacketType type;
    u32 index_or_namehash;
    // Maybe hashes? No real clue. These are identical across all 'nsa_data' packets within one file.
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
};

struct string {
    char value[];
};

fn relative_to_parents_parent(u128 offset) {
    return addressof(parent.parent);
};

struct orientation_transform_locations {
	u8* translation_data : u64 [[pointer_base("relative_to_parents_parent")]];
    u8* rotation_data : u64 [[pointer_base("relative_to_parents_parent")]];
};

struct orientation_subheader {
	u32 datalength;
    u32 alignment;
    u32 element_count;
    bool full;
    padding[3];
    u32 element_type; //maybe
    padding[4];
    u8* unknown_ptr1 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    orientation_transform_locations* transforms : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u8* unknown_ptr3 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    padding[while($ < std::mem::read_unsigned(addressof(this) + 32, 8) + addressof(this) + 16)];
    
};

struct orientation_header {
    u32 header_size;
    u32 unknown1;
    padding[8];
    orientation_subheader subheader;
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

struct expanded_string_table {
	u32 string_count;
    u32 string_data_size;
    u32* string_ids[string_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u32* string_offsets[string_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u8* unknown_data[string_count * 4] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    string* strings[string_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    padding[string_count * sizeof(u32) + string_data_size];
    padding[while(std::mem::read_unsigned($, 1) == 0xCD)];
};

struct string_table {
    u32 string_count;
    u32 string_data_size;
    u32* string_offsets[string_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    string* strings[string_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    padding[string_count * sizeof(u32) + string_data_size];
    padding[while(std::mem::read_unsigned($, 1) == 0xCD)];
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
    u8* unknown_ptr4 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u8* unknown_ptr5 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    padding[8];
    BoneHierarchyEntry chains[chain_count];
    u32 bone_count;
    padding[4];
    u32 unknown_array4[2];
    u32 indices[bone_count];
    string_table bone_names_table;
    padding[while(std::mem::read_unsigned($, 1) == 0xCD)];
    u32 magic;
    padding[4];
    orientation_data orientations;
};

struct u16_array {
    u16 length;
    u16 values[length];
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

// For arrays of pointers
struct checked_ptr {
    if(std::mem::read_unsigned($, 8) != 0x00) {
        u8 *ptr : u64 [[pointer_base("relative_to_parents_parent")]];
    } else {
        u64 null;
    }
} [[inline]];

struct dequantization_factors {
    float q_min[3];
    float q_scaled_extent[3];
};

struct dequantization_info {
    u8 init[3];
    u8 factor_index[3];
};

struct translation_deq_factors {
    dequantization_factors array[parent.translation_anim_deq_count];
};

struct rotation_deq_factors {
    dequantization_factors array[parent.rotation_anim_deq_count];
};

struct scale_deq_factors {
    dequantization_factors array[parent.scale_anim_deq_count];
};

struct vec3_short {
    u16 x, y, z;
};

struct nsa_data_first_segment {
    u32 translation_bone_count;
    u32 rotation_bone_count;
    u32 scale_bone_count;
    dequantization_factors translation_factors;
    dequantization_factors rotation_factors;
    if(scale_bone_count > 0) {
        dequantization_factors scale_factors;
    }
    padding[4];
    u64 unknown;
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	vec3_short *translation_data[translation_bone_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_translation_data;
    }
    
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	vec3_short *rotation_data[rotation_bone_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_rotation_data;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
        vec3_short *scale_data[scale_bone_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_scale_data;
    }
};

bitfield translation_bone_data {
    x : 11;
    y : 11;
    z : 10;
} [[left_to_right]];

struct translation_sample_data {
    translation_bone_data data[parent.translation_bone_count];
};

struct rotation_bone_data {
    u16 data[3];
};
s32 i = -1;
u32 sample_padding;

struct rotation_sample_data {
    rotation_bone_data data[parent.rotation_bone_count];
    padding[sample_padding];
};

// There are deq_info sets for every bone, but the number of sets is always a multiple of 4
// This code simply gets the first multiple of 4 that is greater than or equal to the bone count
fn deq_info_sets(u32 bone_count) {
    u32 multiple = bone_count / 4;
    if(bone_count % 4 != 0) {
        multiple = multiple + 1;
    }
    return multiple * 4;
};

struct nsa_data_second_segment {
    u32 samples;
    u32 translation_bone_count;
    u32 rotation_bone_count;
    u32 scale_bone_count;
    if(std::mem::read_unsigned($, 8) != 0x00) {
	    translation_sample_data *translation_data[samples] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
	    dequantization_info *translation_deq_info[deq_info_sets(translation_bone_count)] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_translation_data;
        u64 null_translation_deq_info;
    }
    
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	sample_padding = ((std::mem::read_unsigned($+8, 8) - std::mem::read_unsigned($, 8)) % (samples * rotation_bone_count * 2)) / samples;
    	//std::print("{:3d} sample_padding = {} samples={:2d} rotation_bone_count={:2d}", i, sample_padding, samples, rotation_bone_count);
	    rotation_sample_data *rotation_data[samples] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
	    dequantization_info *rotation_deq_info[deq_info_sets(rotation_bone_count)] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_rotation_data;
        u64 null_rotation_deq_info;
    }
    
    if(std::mem::read_unsigned($, 8) != 0x00) {
        u8 *scale_data[frames * scale_bone_count * 4] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
        dequantization_info *scale_deq_info : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_scale_data;
        u64 null_scale_deq_info;
    }
};

struct end_data {
	u32 version;
	u32 unk_array[3];
	u32 size;
	u32 alignment;
	float frame_rate;
	u32 samples;
	dequantization_factors factors;
	quaternion rotation;
	checked_ptr translation_data;
	if(std::mem::read_unsigned($, 8) != 0x00) {
		translation_bone_data *rotation_data[samples] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
	} else {
		u64 null_rotation_data;
	}
	checked_ptr scale_data;
};

struct nsa_data {
    be u32 crc32hash; // CRC32 of the rest of the data
    u32 version;   // Maybe?
    padding[8];    // Looks to be just 0 in the files I've seen so far
    u32 static_length, alignment;
    float duration_seconds, frame_rate;
    u32 total_bones, total_animated_bones;
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
    	nsa_data_first_segment *static_data : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_static_data;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	nsa_data_second_segment *dynamic_data : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
        u64 null_dynamic_data;
    }
    if(std::mem::read_unsigned($, 8) != 0x00) {
    	end_data *end_data_ptr : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    } else {
    	u64 null_end_data;
    }
    padding[8];
};

struct string_table_ptr {
    string_table* table : u64 [[pointer_base("relative_to_parents_parent")]];
} [[inline]];

struct file_data {
    string_table* filenames : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    string_table* filetypes : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    string_table* source_filenames : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    string_table* animation_names : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    // Turns out imhex does not allow you to dereference pointers to get values.
    // The count here should be filenames->string_count but unfortunately that is not available in imhex
    be u32* crc32hashes[std::mem::read_unsigned(addressof(this) + 40, 4)] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
};

struct expanded_string_table_ptr {
	expanded_string_table* table : u64 [[pointer_base("relative_to_parents_parent")]];
} [[inline]];

struct network_data_offset_table {
	u32 chunk_count;
	u32 chunk_size;
	checked_ptr* ptrs[chunk_count] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
};

struct network_data {
    u8 unknown1[32];
    checked_ptr ptrs[6];
    u8 unknown2[120];
    checked_ptr ptrs2[5];
    expanded_string_table_ptr ex_string_tables[2];
    string_table* string_table1 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    checked_ptr ptrs3[1];
    string_table* string_table2 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    string_table* string_table3 : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    checked_ptr ptrs4[2];
    network_data_offset_table* data_chunk_table : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    u32 unknown_int;
    padding[4];
    checked_ptr ptrs5[1];
    u32 unknown_int2;
    padding[4];
    checked_ptr* ptr_ptrs[unknown_int2] : u64 [[pointer_base("std::ptr::relative_to_parent")]];
    checked_ptr ptrs6[1];
    u32 some_data_length;
    u32 unknown_int3;
    u16 unknown_short;
    padding[2];
    u32 unknown_int4;
    checked_ptr ptrs7[4];
};

struct skeleton_names_data {
	expanded_string_table_ptr names_table;
};

struct packet {
	i = i + 1;
    header head;
    if(head.type == PacketType::skeleton) {
        skeleton_data skeleton;
        u8 data[head.datalength - sizeof(skeleton)];
    } else if(head.type == PacketType::nsa_data) {
        nsa_data nsa;
        padding[head.datalength - sizeof(nsa)];
    } else if(head.type == PacketType::file_names) {
        file_data files;
        padding[head.datalength - sizeof(files)];
    } else if(head.type == PacketType::network_data) {
        network_data network;
        u8 data[head.datalength - sizeof(network)];
    } else if(head.type == PacketType::skeleton_names) {
    	skeleton_names_data skeletons;
    	padding[head.datalength - sizeof(skeletons)];
    } else {
        u8 data[head.datalength];
    }
    // Packets are always aligned to 4 (.mrn) or 16 (x64.mrn) bytes
    u8 align[$%packet_alignment != 0 ? packet_alignment - $ % packet_alignment : 0];
};

packet p[while($ < sizeof($))] @0x00;
