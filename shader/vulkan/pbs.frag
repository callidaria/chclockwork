#version 450 core


layout(location = 0) in vec2 EdgeCoordinates;

layout(location = 0) out vec4 pixelColour;

layout(input_attachment_index = 0,set = 0,binding = 4) uniform subpassInput spColour;


void main()
{
	pixelColour = vec4(subpassLoad(spColour).rgb,1.);
}
