#version 330 core


in vec2 EdgeCoordinates;
in vec4 Colour;

layout(location = 0) out vec4 pixelColour;

uniform sampler2D tex;


void main()
{
	pixelColour = Colour*texture(tex,EdgeCoordinates).r;
}
