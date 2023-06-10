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

layout (location = 0) in vec3 viewPos;
layout (location = 1) in vec3 facing;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  vec3 xTangent = dFdx(viewPos);
  vec3 yTangent = dFdy(viewPos);
  vec3 faceNormal = normalize(cross(xTangent, yTangent));

  vec3 viewDir = normalize(-viewPos);
  outFragColor = vec4(max(dot(viewDir, faceNormal), 0.2) * facing, 1.0);
}