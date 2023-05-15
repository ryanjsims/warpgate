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
  uint faction;
} ubo;

const vec3 primaries[4] = vec3[](
  vec3(0.58, 0.58, 0.58),
  vec3(0.247, 0.169, 0.396),
  vec3(0.192, 0.251, 0.435),
  vec3(0.396, 0.125, 0.125)
);

const vec3 secondaries[4] = vec3[](
  vec3(0.788, 0.788, 0.788),
  vec3(0.235, 0.584, 0.568),
  vec3(0.580, 0.486, 0.105),
  vec3(0.514, 0.169, 0.149)
);

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D specularSampler;
layout(binding = 3) uniform sampler2D normalSampler;
//layout(binding = 4) uniform sampler2D detailMaskSampler;
//layout(binding = 5) uniform samplerCube detailCubeSampler;

layout (location = 0) in vec2 inTexcoord0;
layout (location = 1) in vec2 inTexcoord1;
layout (location = 2) in vec2 inTexcoord2;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  vec4 normalSample = texture(normalSampler, inTexcoord0);
  float primary = float(normalSample.b < 0.25);
  float secondary = float(normalSample.r < 0.25);
  vec4 mixed_primary = mix(texture(diffuseSampler, inTexcoord0), vec4(primaries[ubo.faction], 1.0), primary);
  outFragColor = mix(mixed_primary, vec4(secondaries[ubo.faction], 1.0), secondary);
}
