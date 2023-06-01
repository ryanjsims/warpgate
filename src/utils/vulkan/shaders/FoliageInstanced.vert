#version 450

layout (location = 0) in vec3 inPos;

layout (location = 1) in vec4 tangent;
layout (location = 2) in vec4 binormal;
layout (location = 3) in vec2 inTexcoord0;
layout (location = 4) in vec4 inColor0;

layout (location = 5) in vec4 inTexcoord5;
layout (location = 6) in vec4 inTexcoord6;
layout (location = 7) in vec4 inTexcoord7;

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
layout (location = 5) out vec4 outTexcoord5;
layout (location = 6) out vec4 outTexcoord6;
layout (location = 7) out vec4 outTexcoord7;


out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outTexcoord0 = inTexcoord0;
	outTexcoord5 = inTexcoord5;
	outTexcoord6 = inTexcoord6;
	outTexcoord7 = inTexcoord7;

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
