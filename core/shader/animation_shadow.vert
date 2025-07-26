#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;
in vec4 bone_index;
in vec4 bone_weight;

out vec3 Normal;
out vec2 UV;
out vec3 Tangent;

// projection
uniform mat4 view;
uniform mat4 proj;

// preset
uniform mat4 model;

// animation
uniform mat4 joint_transform[64];


void main()
{
	// position calculation
	mat4 anim_transform = mat4(.0);
	for (int i=0;i<4;i++) anim_transform += joint_transform[int(bone_index[i])]*bone_weight[i];
	gl_Position = proj*view*model*anim_transform*vec4(position,1.);

	// optimizer satisfaction pass to keep upload structure
	Normal = normal;
	UV = uv;
	Tangent = tangent;
}
