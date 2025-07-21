#version 330 core


in vec2 UV;

layout(location = 0) out vec4 pixelColour;

uniform sampler2D tex;


void main()
{
	pixelColour = texture(tex,UV);
}
