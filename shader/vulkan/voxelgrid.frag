#version 450 core


layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 pixelColour;

layout(set = 1,binding = 0) uniform sampler2D tex;

/*
#layout(push_constant) uniform PushConstants
#{
#	mat4 model;
#} pc;
*/


void main()
{
	pixelColour = texture(tex,UV);
}
