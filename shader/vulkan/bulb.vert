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

layout(location = 0) out vec3 Position;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec3 Colour;
layout(location = 3) out vec2 Material;

layout(set = 0,binding = 0) uniform ObjectTransformation
{
	mat4 view;
	mat4 proj;
} ot;


void main()
{
	Position = position*scale+offset;
	Normal = normal;
	Colour = colour;
	Material = material;
	gl_Position = ot.proj*ot.view*vec4(Position,1.);
}
