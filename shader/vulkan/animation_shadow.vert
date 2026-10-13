#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 bone_index;
layout(location = 5) in vec4 bone_weight;

layout(set = 0,binding = 42) uniform OrthographicShadow
{
	mat4 view;
	mat4 proj;
	vec3 source;
} os;

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
	mat4 anim_transform = mat4(.0);
	//for (int i=0;i<4;i++) anim_transform += joint.transform[int(bone_index[i])]*bone_weight[i];
	vec4 world_position = trafo.model*anim_transform*vec4(position,1.);
	gl_Position = os.proj*os.view*world_position;
}
