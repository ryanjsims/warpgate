#version 450

layout (binding = 0) uniform UBO 
{
  mat4 projectionMatrix;
  mat4 modelMatrix;
  mat4 viewMatrix;
  mat4 invProjectionMatrix;
	mat4 invViewMatrix;
  uint faction;
} ubo;

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D specularSampler;
layout(binding = 3) uniform sampler2D normalSampler;
layout(binding = 4) uniform sampler2D detailMaskSampler;
layout(binding = 5) uniform samplerCube detailCubeSampler;

layout (location = 0) in vec4 inTexcoord0;
layout (location = 1) in vec4 inTexcoord1;


layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = texture(diffuseSampler, inTexcoord0.st);
}
