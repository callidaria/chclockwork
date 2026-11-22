#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
// TODO reduce it to a more simplisting cube input data structure

layout(location = 0) out vec3 Position;

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
// TODO make it possible to remove those for forward geometry pipeline registrations


void main()
{
	Position = position;
	mat4 ntrans_view = mat4(mat3(ot.view));
	vec4 out_position = ot.proj*ntrans_view*vec4(position,1.);
	gl_Position = out_position.xyww;
}
