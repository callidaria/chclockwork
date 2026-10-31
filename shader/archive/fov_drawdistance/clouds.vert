#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;

out vec3 Position;
out vec2 UV;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;


void main()
{
	vec4 world_position = model*vec4(position,1.f);
	gl_Position = proj*view*world_position;

	// pass
	Position = world_position.xyz;
	UV = uv;
	Normal = (model*vec4(normal,1.f)).xyz;
}
