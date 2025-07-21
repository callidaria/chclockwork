#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;

// engine: ibo
in vec3 offset;
in float scale;
in vec3 colour;
in vec2 material;
// TODO which is faster, encode rotation into euler, use quaternion upload or simply upload transformation matrix
// TODO how to map from sprite atlas with multibuffer

out vec3 Position;
out vec3 Normal;
out vec3 Colour;
out vec2 Material;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	Position = position*scale+offset;
	Normal = normal;
	Colour = colour;
	Material = material;
	gl_Position = proj*view*vec4(Position,1.);
}
