#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor0;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 invProjectionMatrix;
	mat4 invViewMatrix;
	uint faction;
} ubo;



out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
