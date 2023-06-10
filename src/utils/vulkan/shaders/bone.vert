#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 invProjectionMatrix;
	mat4 invViewMatrix;
	uint faction;
} ubo;

uniform float _length;

out gl_PerVertex 
{
    vec4 gl_Position;
};

layout (location = 0) out vec3 viewSpacePos;
layout (location = 1) out vec3 facing;

void main() 
{
    facing = normalize(vec3(inPos.xz, inPos.y * 0.1)).xzy;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(_length * inPos.xyz, 1.0);

    viewSpacePos = (ubo.viewMatrix * ubo.modelMatrix * vec4(_length * inPos, 1.0)).xyz;
}