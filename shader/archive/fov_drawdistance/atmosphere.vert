#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;

out vec3 Position;

uniform mat4 view;
uniform mat4 proj;

const float Scale = 1.1;


void main()
{
	Position = position*Scale;
	gl_Position = proj*view*vec4(Position,1.f);
}
