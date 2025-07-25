#version 330 core


in vec3 Position;
in vec2 UV;
in mat3 TBN;

layout(location = 0) out vec4 gbuffer_colour;
layout(location = 1) out vec4 gbuffer_position;
layout(location = 2) out vec4 gbuffer_normals;
layout(location = 3) out vec4 gbuffer_materials;
layout(location = 4) out vec4 gbuffer_emission;

uniform sampler2D colour_map;
uniform sampler2D normal_map;
uniform sampler2D material_map;
uniform sampler2D emission_map;


void main()
{
	// FIXME unused shader, this is passed into gpass.frag
}
