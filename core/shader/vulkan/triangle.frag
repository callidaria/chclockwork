#version 450 core


layout(location = 0) in vec3 Colour;
layout(location = 1) in vec2 TexCoords;

layout(location = 0) out vec4 pixelColour;

layout(binding = 1) uniform sampler2D tex;


void main()
{
	pixelColour = texture(tex,TexCoords);
}
