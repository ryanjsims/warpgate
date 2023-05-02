#version 450

// #define SPECULAR_BINDING 2
// #define NORMAL_BINDING 3
// #define DETAIL_MASK_BINDING 4
// #define DETAIL_CUBE_BINDING 5

layout (binding = 0) uniform UBO 
{
  mat4 projectionMatrix;
  mat4 modelMatrix;
  mat4 viewMatrix;
  mat4 invProjectionMatrix;
	mat4 invViewMatrix;
} ubo;

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D specularSampler;
// layout(binding = 3) uniform sampler2D normalSampler;
// layout(binding = 4) uniform sampler2D detailMaskSampler;
// layout(binding = 5) uniform samplerCube detailCubeSampler;

layout (location = 0) in vec2 inTexcoord0;
layout (location = 1) in vec2 inTexcoord1;
layout (location = 2) in vec2 inTexcoord2;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = texture(diffuseSampler, inTexcoord0);
}
