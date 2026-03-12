#version 450 core


layout(location = 0) in vec2 EdgeCoordinates;
layout(location = 1) in vec4 Colour;

layout(location = 0) out vec4 pixelColour;

layout(binding = 2) uniform sampler2D tex;


void main()
{
	pixelColour = Colour*texture(tex,EdgeCoordinates).r;
	pixelColour = vec4(1,0,0,1);  // §§testing colour for vertex validation
}
