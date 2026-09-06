#version 450 core


layout(location = 0) in vec2 EdgeCoordinates;
layout(location = 1) in vec4 Colour;

layout(location = 0) out vec4 pixelColour;

layout(binding = 3) uniform sampler2D tex;


void main()
{
	pixelColour = Colour*texture(tex,EdgeCoordinates).r;
}
