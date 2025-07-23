#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;
in vec4 bone_index;
in vec4 bone_weight;

out vec2 UV;

// projection
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

// animation
uniform mat4 joint_transform[64];


void main()
{
	// animation transform
	mat4 anim_transform = mat4(.0);
	for (int i=0;i<4;i++) anim_transform += joint_transform[int(bone_index[i])]*bone_weight[i];
	vec4 Position = model*anim_transform*vec4(position,1.);

	// pass
	UV = uv;
	gl_Position = proj*view*Position;
}
