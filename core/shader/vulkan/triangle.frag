#version 450 core


layout(location = 0) in vec3 Colour;

layout(location = 0) out vec4 pixelColour;


void main()
{
	pixelColour = vec4(Colour,1.);
}
