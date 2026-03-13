#version 330 core


in vec2 position;
in vec2 edge_coordinates;

// engine: ibo
in vec3 offset;
in vec2 scale;
in vec2 bearing;
in vec4 colour;
in vec2 atlas_position;
in vec2 atlas_dimension;

out vec2 EdgeCoordinates;
out vec4 Colour;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	vec2 Position = position+.5;
	Position = Position*scale+offset.xy+bearing;
	gl_Position = proj*view*vec4(Position.x,Position.y-scale.y,offset.z,1.);

	// pass
	EdgeCoordinates = atlas_position+atlas_dimension*edge_coordinates;
	Colour = colour;
}
