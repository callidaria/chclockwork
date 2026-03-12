#version 330 core


in vec3 Position;
in vec3 Normal;
in vec3 Colour;
in vec2 Material;

layout(location = 0) out vec4 gbuffer_colour;
layout(location = 1) out vec4 gbuffer_position;
layout(location = 2) out vec4 gbuffer_normals;
layout(location = 3) out vec4 gbuffer_materials;
layout(location = 4) out vec4 gbuffer_emission;


void main()
{
	gbuffer_colour = vec4(0,0,0,1);
	gbuffer_position = vec4(Position,1);
	gbuffer_normals = vec4(Normal,1);
	gbuffer_materials = vec4(0,.4,1,1);
	gbuffer_emission = vec4(Colour,1);
}
