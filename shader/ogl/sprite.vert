#version 330 core


in vec2 position;
in vec2 edge_coordinates;

// engine: ibo
in vec3 offset;
in vec2 scale;
in float rotation;
in float alpha;
in vec2 tex_position;
in vec2 tex_dimension;

out vec2 EdgeCoordinates;
out float Alpha;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	// sprite rotation
	float rd_rotation = radians(rotation);
	float rotation_sin = sin(rd_rotation);
	float rotation_cos = cos(rd_rotation);
	vec2 Position = mat2(rotation_cos,-rotation_sin,rotation_sin,rotation_cos)*position;
	gl_Position = proj*view*vec4(Position*scale+offset.xy,offset.z,1.);

	// pass
	EdgeCoordinates = tex_position+tex_dimension*edge_coordinates;
	Alpha = alpha;
}
