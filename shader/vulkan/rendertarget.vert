#version 450 core


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 edge_coordinates;

// TODO multi-perspective support

layout(location = 0) out vec2 EdgeCoordinates;


void main()
{
	gl_Position = vec4(position,1.);

	// pass
	EdgeCoordinates = edge_coordinates;
}
