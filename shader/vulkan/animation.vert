#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 bone_index;
layout(location = 5) in vec4 bone_weight;

layout(location = 0) out vec3 Position;
layout(location = 1) out vec2 UV;
layout(location = 2) out mat3 TBN;

layout(set = 0,binding = 0) uniform ObjectTransformation
{
	mat4 view;
	mat4 proj;
} ot;

layout(set = 1,binding = 70) uniform ObjectInfo
{
	mat4 model;
	float texel;
} trafo;

layout(set = 1,binding = 128) uniform JointTransform
{
	mat4 transform[64];
} joint;


void main()
{
	// animation transform
	mat4 anim_transform = mat4(.0);
	//for (int i=0;i<4;i++) anim_transform += joint.transform[int(bone_index[i])]*bone_weight[i];
	vec4 world_position = trafo.model*anim_transform*vec4(position,1.);

	// pass
	Position = world_position.xyz;
	UV = uv*trafo.texel;
	gl_Position = ot.proj*ot.view*world_position;

	// normal transformation setup
	vec3 Tangent = normalize((trafo.model*anim_transform*vec4(tangent,0)).xyz);
	vec3 Normal = normalize((trafo.model*anim_transform*vec4(normal,0)).xyz);
	Tangent = normalize(Tangent-dot(Tangent,Normal)*Normal);
	vec3 Bitangent = cross(Normal,Tangent);
	TBN = mat3(Tangent,Bitangent,Normal);
}
