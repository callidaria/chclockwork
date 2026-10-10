#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;
in vec4 bone_index;
in vec4 bone_weight;

out vec3 Position;
out vec2 UV;
out mat3 TBN;

// projection
uniform mat4 view;
uniform mat4 proj;

// preset
uniform mat4 model;
uniform float texel = 1.;

// animation
uniform mat4 joint_transform[64];


void main()
{
	// animation transform
	mat4 anim_transform = mat4(.0);
	for (int i=0;i<4;i++) anim_transform += joint_transform[int(bone_index[i])]*bone_weight[i];
	vec4 world_position = model*anim_transform*vec4(position,1.);

	// pass
	Position = world_position.xyz;
	UV = uv*texel;
	gl_Position = proj*view*world_position;

	// normal transformation setup
	vec3 Tangent = normalize((model*anim_transform*vec4(tangent,0)).xyz);
	vec3 Normal = normalize((model*anim_transform*vec4(normal,0)).xyz);
	Tangent = normalize(Tangent-dot(Tangent,Normal)*Normal);
	vec3 Bitangent = cross(Normal,Tangent);
	TBN = mat3(Tangent,Bitangent,Normal);
}
