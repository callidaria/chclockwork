#version 450 core


layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 Normal;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 pixelColour;

layout(set = 0,binding = 5) uniform sampler2D tex[2048];
layout(push_constant) uniform PushConstants
{
	uint texIndex;
} pc;


void main()
{
	pixelColour = texture(tex[pc.texIndex],UV);
}
