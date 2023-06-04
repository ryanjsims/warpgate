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

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = vec4(0.3, 0.3, 0.3, 1.0);
}