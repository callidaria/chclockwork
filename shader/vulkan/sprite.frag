#version 450 core


layout(location = 0) in vec2 EdgeCoordinates;
layout(location = 1) in float Alpha;

layout(location = 0) out vec4 pixelColour;

layout(binding = 2) uniform sampler2D tex;


void main()
{
	pixelColour = texture(tex,EdgeCoordinates);
	pixelColour.a *= Alpha;
}
