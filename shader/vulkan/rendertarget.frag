#version 450 core


layout(location = 0) in vec2 EdgeCoordinates;

layout(location = 0) out vec4 pixelColour;

layout(set = 0,binding = 4) uniform sampler2D fpass_colour;
layout(set = 0,binding = 5) uniform sampler2D fpass_depth;
layout(set = 0,binding = 6) uniform sampler2D gpass_colour;
layout(set = 0,binding = 7) uniform sampler2D gpass_position;
layout(set = 0,binding = 8) uniform sampler2D gpass_normal;
layout(set = 0,binding = 9) uniform sampler2D gpass_material;
layout(set = 0,binding = 10) uniform sampler2D gpass_emission;
layout(set = 0,binding = 11) uniform sampler2D gpass_depth;


void main()
{
	pixelColour = texture(gpass_colour,EdgeCoordinates);
}
