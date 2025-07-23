#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;
in vec4 bone_index;
in vec4 bone_weight;

out vec2 UV;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	UV = uv;
	gl_Position = proj*view*vec4(position,1.);
}
