#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

// engine: ibo
layout(location = 10) in vec3 offset;

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
	gl_Position = ot.proj*ot.view*ot.model*vec4(position+offset,1.);

	// pass
	UV = uv;
	Normal = normal;

	// gram-schmidt reorthogonalization
	vec3 Tangent = normalize(tangent-dot(tangent,normal)*normal);
	TBN = mat3(Tangent,cross(normal,Tangent),normal);
}
