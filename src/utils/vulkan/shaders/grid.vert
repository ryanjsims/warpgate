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

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout(location = 1) out vec3 nearPoint;
layout(location = 2) out vec3 farPoint;


vec3 UnprojectPoint(float x, float y, float z) {
    vec4 unprojectedPoint =  ubo.invViewMatrix * ubo.invProjectionMatrix * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}


void main() 
{
	nearPoint = UnprojectPoint(inPos.x, inPos.y, 0.0);
	farPoint = UnprojectPoint(inPos.x, inPos.y, 1.0);
	gl_Position = vec4(inPos.xyz, 1.0);
}
