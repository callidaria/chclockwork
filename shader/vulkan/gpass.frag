#version 450 core


layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 gbuffer_colour;
layout(location = 1) out vec4 gbuffer_position;
layout(location = 2) out vec4 gbuffer_normals;
layout(location = 3) out vec4 gbuffer_materials;
layout(location = 4) out vec4 gbuffer_emission;

layout(set = 1,binding = 0) uniform sampler2D tex[64];

layout(push_constant) uniform PushConstants
{
	mat4 model;
	float texel;
	uint colour_map;
	uint normal_map;
	uint material_map;
	uint emission_map;
} pc;


void main()
{
	// extract colour & position
	gbuffer_colour = vec4(texture(tex[pc.colour_map],UV).rgb,1.);
	gbuffer_position = vec4(Position,1.);

	// translate normals
	vec3 normals = texture(tex[pc.normal_map],UV).rgb*2.0-1.0;
	gbuffer_normals = vec4(normalize(TBN*normals),1.);

	// extract surface materials
	gbuffer_materials = vec4(texture(tex[pc.material_map],UV).rgb,1.);
	gbuffer_emission = vec4(texture(tex[pc.emission_map],UV).rgb,1.);
}
// FIXME alpha values are completely unused here, this should be abused!
