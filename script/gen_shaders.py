import json
from pathlib import Path

vertex_base = """#version 450

{}
layout (binding = 0) uniform UBO 
{{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 invProjectionMatrix;
	mat4 invViewMatrix;
	uint faction;
}} ubo;

{}

out gl_PerVertex 
{{
    vec4 gl_Position;   
}};


void main() 
{{
{}
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}}
"""

vertex_inputs = "layout (location = {}) in {} {};\n"
texcoord_format = "inTexcoord{}"
out_texcoord_format = "layout (location = {0}) out {1} outTexcoord{0};\n"
in_texcoord_format = "layout (location = {0}) in {1} inTexcoord{0};\n"
assignment_format = "	outTexcoord{0} = inTexcoord{0};\n"
color_format = "inColor{}"

color_out_texture = "outFragColor = texture(diffuseSampler, inTexcoord0.st);"
color_out_notexture = "outFragColor = vec4(0, 0, 0, 1);"

fragment_base = """#version 450

layout (binding = 0) uniform UBO 
{{
  mat4 projectionMatrix;
  mat4 modelMatrix;
  mat4 viewMatrix;
  mat4 invProjectionMatrix;
	mat4 invViewMatrix;
  uint faction;
}} ubo;

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D specularSampler;
layout(binding = 3) uniform sampler2D normalSampler;
layout(binding = 4) uniform sampler2D detailMaskSampler;
layout(binding = 5) uniform samplerCube detailCubeSampler;

{}

layout (location = 0) out vec4 outFragColor;

void main() 
{{
  {}
}}
"""

type_from_entry_type = {
    "Float3":    "vec3",
    "D3dcolor":  "vec4",
    "Float2":    "vec2",
    "Float4":    "vec4",
    "ubyte4n":   "vec4",
    "Float16_2": "vec2",
    "float16_2": "vec2",
    "Short2":    "vec2",
    "Float1":    "float",
    "Short4":    "vec4"
}

def get_variable_name(usage: str, usageIndex: int):
    if usage == "Position":
        return "inPos"
    elif usage == "Texcoord":
        return texcoord_format.format(usageIndex)
    elif usage == "Color":
        return color_format.format(usageIndex)
    return usage.lower()

with open("../resources/materials.json") as f:
    materials = json.load(f)

output_path = Path("./shaders")
output_path.mkdir(parents=True, exist_ok=True)

for key, inputLayout in materials["inputLayouts"].items():
    key: str
    fragment_input_block = ""
    with open((output_path / key).with_suffix(".vert"), "w") as vertex:
        input_block = ""
        output_block = ""
        assignment_block = ""
        vertex_stream = 0

        for i, entry in enumerate(inputLayout["entries"]):
            if vertex_stream != entry["stream"]:
                vertex_stream = entry["stream"]
                input_block += "\n"
            glsl_type = type_from_entry_type[entry["type"]]
            if entry["usage"] == "BlendIndices":
                glsl_type = "u" + glsl_type
            input_block += vertex_inputs.format(i, glsl_type, get_variable_name(entry["usage"], entry["usageIndex"]))
            if entry["usage"] == "Texcoord":
                output_block += out_texcoord_format.format(entry["usageIndex"], glsl_type)
                fragment_input_block += in_texcoord_format.format(entry["usageIndex"], glsl_type)
                assignment_block += assignment_format.format(entry["usageIndex"])
        vertex.write(vertex_base.format(input_block, output_block, assignment_block))
    
    with open((output_path / key).with_suffix(".frag"), "w") as fragment:
        fragment.write(fragment_base.format(fragment_input_block, color_out_texture if len(fragment_input_block) > 0 else color_out_notexture))