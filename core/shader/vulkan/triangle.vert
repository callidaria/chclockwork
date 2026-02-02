#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec2 texCoords;

layout(location = 0) out vec3 Colour;
layout(location = 1) out vec2 TexCoords;

layout(binding = 0) uniform ObjectTransformation {
	mat4 model;
	mat4 view;
	mat4 proj;
} ot;


void main()
{
	gl_Position = ot.proj*ot.view*ot.model*vec4(position,1.);
	Colour = colour;
	TexCoords = texCoords;
}
