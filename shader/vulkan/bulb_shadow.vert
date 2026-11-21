#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

// engine: ibo
layout(location = 4) in vec3 offset;
layout(location = 5) in float scale;
layout(location = 6) in vec3 colour;
layout(location = 7) in vec2 material;

layout(set = 0,binding = 42) uniform OrthographicShadow
{
	mat4 view;
	mat4 proj;
	vec3 source;
} os;


void main()
{
	vec3 Position = position*scale+offset;
	gl_Position = os.proj*os.view*vec4(Position,1.);
}
