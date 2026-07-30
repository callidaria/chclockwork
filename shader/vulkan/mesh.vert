#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

/*
// aengine: ibo
alayout(location = 10) in vec3 offset;
alayout(location = 11) in vec2 atlas_position;
alayout(location = 12) in vec2 atlas_dimension;
*/

layout(location = 0) out vec2 UV;
layout(location = 1) out vec3 Normal;
layout(location = 2) out mat3 TBN;

layout(set = 0,binding = 0) uniform ObjectTransformation
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ot;


void main()
{
	gl_Position = ot.proj*ot.view*ot.model*vec4(position/*+offset*/,1.);

	// pass
	UV = /*atlas_position+atlas_dimension**/uv;
	Normal = normal;

	// gram-schmidt reorthogonalization
	vec3 Tangent = normalize(tangent-dot(tangent,normal)*normal);
	TBN = mat3(Tangent,cross(normal,Tangent),normal);
}
