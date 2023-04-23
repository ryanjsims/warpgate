#version 450

layout(binding = 1) uniform sampler2D tex0sampler;

layout (location = 0) in vec2 inTexcoord0;
layout (location = 1) in vec2 inTexcoord1;
layout (location = 2) in vec2 inTexcoord2;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = texture(tex0sampler, inTexcoord0);
}
