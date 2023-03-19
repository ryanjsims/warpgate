# Forgelight File Format Docs

The files in this folder document the structure of various file formats found within Forgelight games.

To use a *.hexpat file, download [ImHex](https://github.com/WerWolv/ImHex), open a file of the type corresponding to the name of the hex pattern file with it, and then click `file -> load pattern... -> /path/to/xxx.hexpat`

## MRN (Morpheme Runtime Network)
An MRN file contains animation data created by a (now defunct) tool/middleware called `morpheme` created by the company NaturalMotion. This tool was used to create "networks" or "state machines" of animations such that each animation had links to other animations it could transition to. For example, a character could have an `idle` animation that could transition to a `run` animation, `walk` animation, `sit` animation, or could remain in the `idle` animation state.

The data contained in the file is structured as a series of "packets" which contain fields indicating their own length, packet type, and (assumed) hashes of the data contained within.

Currently, packet types with some level of analysis and reversing applied include:
* Skeleton data
* Animation motion data
* Namelists for both the skeletons and the animations

Assumed packet types include:
* Animation state data
* Animation network data

There are other packet types that currently have unknown functions

### NSA Animation Data
In the animations' namelist, each animation is named as a file with the extension `.nsa`. These are one of several formats of animation data that Morpheme was able to compile to, and is the only format observed in PS2. Other formats include QSA, DWA, and MBA.

These files contain several different types of structures, as well as pointers that can be null if they do not contain data.

NSA files are structured with a few different segments. First is the static header of the file which specifies the data's crc32 hash, version, duration in seconds, sample rate, total number of bones, and the number of animated bones. Next come several pointers relative to the start of the file which point to the start of arrays of bone indices. These arrays begin with the number of elements in the array. There are two sets of three bone index arrays. The first three are bones that are static/unchanging in translation/rotation/scaling respectively, and the second three are bones that do change in translation/rotation/scaling.

After the bone indices, the dequantization factors are specified. The dequantization factors are arrays of 6 float structures used to undo range reduction and quantization performed by the morpheme compiler when compressing the animation data. The first three floats are the x, y, and z minimum values, and the second three are the x, y, and z scaled extents. The data first contains one of these `DequantizationFactors` structures which contains the factors to dequantize the translation initial offsets. Then there are 3 unsigned integers which represent the length of the translation, rotation, and scale dequantization factor arrays. Then there are pointers to the arrays themselves. If there are no dequantization factors for a channel, that channel's array length will be zero and the pointer to the array will be `null`. Following these pointers are pointers to three different sections of the file, the static transform data section, dynamic transform data section, and the end data section. The dynamic data section's pointer can be `null` if the animation simply represents a static pose.

The static transform section contains one sample for the bones not animated by the animation. At the start of the section are three unsigned integers representing the number of bones the translation, rotation, and scale channels affect. Then comes two sets of dequantization factors for the static translation and rotation data, finally followed by pointers (relative to the beginning of the section) to the actual translation, rotation, and scale data. The data consists of `translation/rotation/scale_bone_count * 3` unsigned shorts.

The dynamic transfrom section is similar to the static section, except it may contain multiple samples. The section starts with an unsigned integer defining the number of samples contained, followed by 3 unsigned integers for the translation, rotation, and scale bone counts. Next come three sets of two pointers each. The first of each set points to the animation data, and the second points to an array of 6 byte `DequantizationInfo` structures. For translation data, the samples are structured as an array of arrays of 4 byte packed x, y, and z values. Each inner array has `translation_bone_count` elements, and there will be `samples` outer arrays. The x, y, and z values are packed as 11-, 11-, and 10-bit unsigned integers in an unsigned 32 bit value. They can be retrieved as `x, y, z = ((value >> 21) & 0x7FF, (value >> 10) & 0x7FF, value & 0x3FF)`. The rotation data has been observed to be stored as either 3 or 4 unsigned shorts per sample, though if there are 4 shorts, the fourth will be 0. Scale data has not been encountered in PS2's animations yet, so I don't have any info about it yet. As for the `DequantizationInfo` structures - there will be one for each bone, padded to a multiple of four (if there is one bone there will be four `DequantizationInfo` structures, five bones eight structures, etc. The last ones will be duplicated as padding). The first three bytes in the `DequantizationInfo` are 8 bit unsigned integers containing initial values/offsets for x, y, and z to use when dequantizing, and the second three bytes are indices into the dequantization factor array for the particular channel. The x, y, and z values can each use a different set of dequantization factors, but will always use the minimums and extents corresponding to their index (x will always use `factor.minimums[0] and factor.extents[0]`, y will use index 1, and z index 2).

The end data section appears to be a second place to store the animation framerate and sample count for when the dynamic data is not present. There also appears to be a 4x4 matrix or something as part of this data, but I am not sure what it is used for.

#### Dequantizing the data
To dequantize a translation sample, we need the x, y and z values, the dequantization factor array for the translation channel, the dequantization factor structure for the translation init data, and the `DequantizationInfo` structure corresponding to the bone. The dequantization algorithm looks like this:

```python
def unpack_pos(packed_pos: Tuple[int, int, int], init_factor_indices: InitFactorIndices, pos_factors: List[DequantizationFactors], init_factors: DequantizationFactors) -> Tuple[float, float, float]:
    x_factors = pos_factors[init_factor_indices.dequantization_factor_indices[0]]
    x_quant_factor = x_factors.q_extent[0]
    x_quant_min = x_factors.q_min[0]

    y_factors = pos_factors[init_factor_indices.dequantization_factor_indices[1]]
    y_quant_factor = y_factors.q_extent[1]
    y_quant_min = y_factors.q_min[1]

    z_factors = pos_factors[init_factor_indices.dequantization_factor_indices[2]]
    z_quant_factor = z_factors.q_extent[2]
    z_quant_min = z_factors.q_min[2]

    init_x = init_factors.q_extent[0] * init_factor_indices.init_values[0] + init_factors.q_min[0]
    init_y = init_factors.q_extent[1] * init_factor_indices.init_values[1] + init_factors.q_min[1]
    init_z = init_factors.q_extent[2] * init_factor_indices.init_values[2] + init_factors.q_min[2]

    dequant_x = x_quant_factor * packed_pos[0] + x_quant_min + init_x
    dequant_y = y_quant_factor * packed_pos[1] + y_quant_min + init_y
    dequant_z = z_quant_factor * packed_pos[2] + z_quant_min + init_z
    return dequant_x, dequant_y, dequant_z
```

I am still working on dequantizing rotations and I'll update this readme when I've got them working.

### Recommended Reading
* [Nicholas Frechette's blog](https://nfrechette.github.io/2016/10/21/anim_compression_toc/) was very helpful when learning about animation compression to reverse the NSA data