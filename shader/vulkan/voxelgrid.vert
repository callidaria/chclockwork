#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

// engine: ibo
layout(location = 10) in vec3 offset;

layout(location = 0) out vec2 UV;

layout(set = 0,binding = 0) uniform ObjectTransformation
{
	mat4 view;
	mat4 proj;
} ot;

layout(set = 1,binding = 71) uniform VoxelInfo
{
	mat4 model;
} vi;


void main()
{
	gl_Position = ot.proj*ot.view*vi.model*vec4(position+offset,1.);
	UV = uv;
}
