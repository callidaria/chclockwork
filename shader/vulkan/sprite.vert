#version 450 core


layout(location = 0) in vec2 position;
layout(location = 1) in vec2 edge_coordinates;

// engine: ibo
layout(location = 2) in vec3 offset;
layout(location = 3) in vec2 scale;
layout(location = 4) in float rotation;
layout(location = 5) in float alpha;
layout(location = 6) in vec2 atlas_position;
layout(location = 7) in vec2 atlas_dimension;

layout(location = 0) out vec2 EdgeCoordinates;
layout(location = 1) out float Alpha;

layout(set = 0,binding = 3) uniform SpriteTransformation
{
	mat4 view;
	mat4 proj;
} csys;
// TODO change


void main()
{
	// sprite rotation
	float rd_rotation = radians(rotation);
	float rotation_sin = sin(rd_rotation);
	float rotation_cos = cos(rd_rotation);
	vec2 Position = mat2(rotation_cos,-rotation_sin,rotation_sin,rotation_cos)*position;
	gl_Position = csys.proj*csys.view*vec4(Position*scale+offset.xy,offset.z,1.);

	// pass
	EdgeCoordinates = atlas_position+atlas_dimension*edge_coordinates;
	Alpha = alpha;
}
