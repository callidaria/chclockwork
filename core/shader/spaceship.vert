#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;

// engine: ibo
in vec3 offset;
in float scale;
in vec2 tex_offset;
in vec2 tex_dimension;

out vec2 UV;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	gl_Position = proj*view*vec4(position*scale+offset,1.);
	UV = uv;
}
