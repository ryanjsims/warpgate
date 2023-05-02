#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 blendweight;
layout (location = 2) in uvec4 blendindices;

layout (location = 3) in vec4 tangent;
layout (location = 4) in vec4 binormal;
layout (location = 5) in vec2 inTexcoord0;
layout (location = 6) in vec2 inTexcoord1;
layout (location = 7) in vec2 inTexcoord2;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 invProjectionMatrix;
	mat4 invViewMatrix;
} ubo;

layout (location = 0) out vec2 outTexcoord0;
layout (location = 1) out vec2 outTexcoord1;
layout (location = 2) out vec2 outTexcoord2;


out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outTexcoord0 = inTexcoord0;
	outTexcoord1 = inTexcoord1;
	outTexcoord2 = inTexcoord2;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
