#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(location = 0) out vec3 Position;
layout(location = 1) out vec2 UV;
layout(location = 2) out mat3 TBN;

layout(set = 0,binding = 0) uniform ObjectTransformation
{
	mat4 view;
	mat4 proj;
} ot;

layout(push_constant) uniform PushConstants
{
	mat4 model;
	float texel;
	uint colour_map;
	uint normal_map;
	uint material_map;
	uint emission_map;
} pc;
// TODO allow for standard values (here texel = 1.)


void main()
{
	vec4 world_position = pc.model*vec4(position,1.);
	Position = world_position.xyz;
	gl_Position = ot.proj*ot.view*world_position;

	// calculate texture coordinates
	UV = uv*pc.texel;

	// gram-schmidt orthogonalization
	vec3 Tangent = normalize((pc.model*vec4(tangent,0)).xyz);
	vec3 Normal = normalize((pc.model*vec4(normal,0)).xyz);
	Tangent = normalize(Tangent-dot(Tangent,Normal)*Normal);
	vec3 Bitangent = cross(Normal,Tangent);
	TBN = mat3(Tangent,Bitangent,Normal);
}
