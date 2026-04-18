#version 450 core


layout(location = 0) in vec2 EdgeCoordinates;

layout(location = 0) out vec4 pixelColour;

layout(set = 0,binding = 4) uniform sampler2D tex;


void main()
{
	pixelColour = texture(tex,EdgeCoordinates);
}
