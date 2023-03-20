struct quaternion {
	float x, y, z, w;
};

struct vector {
	float x, y, z;
};

struct translation_bone_data {
	vector samples[31];
};

struct translation_data {
	translation_bone_data bones[13];
};

struct rotation_bone_data {
	quaternion samples[31];
};

struct rotation_data {
	rotation_bone_data bones[14];
};

struct animation {
	float timeline[31];
	translation_data t;
	rotation_data r;
};

animation anim @0x00;
