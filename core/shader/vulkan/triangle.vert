#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 colour;

layout(location = 0) out vec3 Colour;

/*
layout(binding = 0) uniform ObjectTransformation {
	mat4 model;
	mat4 view;
	mat4 proj;
} ot;
*/


void main()
{
	//gl_Position = ot.proj*ot.view*ot.model*vec4(position,.0,1.);
	gl_Position = vec4(position,.0,1.);
	Colour = colour;
}
