#version 450 core


layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 pixelColour;

layout(set = 2,binding = 0) uniform sampler2D tex;


void main()
{
	pixelColour = texture(tex,UV);
}
