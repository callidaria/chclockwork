#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(set = 0,binding = 0) uniform ObjectTransformation
{
	mat4 view;
	mat4 proj;
} ot;

layout(set = 1,binding = 70) uniform ObjectInfo
{
	mat4 model;
	float texel;
} trafo;


void main()
{
	vec4 world_position = trafo.model*vec4(position,1.);
	gl_Position = ot.proj*ot.view*world_position;
}
