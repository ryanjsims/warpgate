#version 450

layout (location = 0) in vec3 inPos;

layout (location = 1) in vec3 tangent;
layout (location = 2) in vec3 binormal;
layout (location = 3) in vec2 inTexcoord0;

layout (location = 4) in vec4 inTexcoord1;
layout (location = 5) in vec2 inTexcoord2;
layout (location = 6) in vec4 inTexcoord3;
layout (location = 7) in vec4 inTexcoord4;
layout (location = 8) in vec4 inColor0;

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
layout (location = 1) out vec4 outTexcoord1;
layout (location = 2) out vec2 outTexcoord2;
layout (location = 3) out vec4 outTexcoord3;
layout (location = 4) out vec4 outTexcoord4;


out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outTexcoord0 = inTexcoord0;
	outTexcoord1 = inTexcoord1;
	outTexcoord2 = inTexcoord2;
	outTexcoord3 = inTexcoord3;
	outTexcoord4 = inTexcoord4;

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
