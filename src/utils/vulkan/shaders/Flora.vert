#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in float inColor0;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;
layout (location = 4) in vec2 inTexcoord0;
layout (location = 5) in vec4 inColor1;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 invProjectionMatrix;
	mat4 invViewMatrix;
	uint faction;
} ubo;

layout (location = 0) out vec2 outTexcoord0;


out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outTexcoord0 = inTexcoord0;

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
