#version 330 core


in vec3 Position;
in vec2 EdgeCoordinates;
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
	// extract colour & position
	gbuffer_colour = vec4(texture(colour_map,EdgeCoordinates).rgb,1.);
	gbuffer_position = vec4(Position,1.);

	// translate normals
	vec3 normals = texture(normal_map,EdgeCoordinates).rgb*2.0-1.0;
	gbuffer_normals = vec4(normalize(TBN*normals),1.);

	// extract surface materials
	gbuffer_materials = vec4(texture(material_map,EdgeCoordinates).rgb,1.);
	gbuffer_emission = vec4(texture(emission_map,EdgeCoordinates).rgb,1.);
}
// FIXME alpha values are completely unused here, this should be abused!
