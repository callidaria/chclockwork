#version 450 core


layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 Normal;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 pixelColour;

layout(binding = 1) uniform sampler2D tex;


void main()
{
	pixelColour = texture(tex,UV);
}
