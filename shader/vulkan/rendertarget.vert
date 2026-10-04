#version 450 core


layout(location = 0) in vec2 position;
layout(location = 1) in vec2 edge_coordinates;

// TODO multi-perspective support
// TODO sprite depth ordering of multi-perspective visualization

layout(location = 0) out vec2 EdgeCoordinates;


void main()
{
	gl_Position = vec4(position,.9,1.);

	// pass
	EdgeCoordinates = edge_coordinates;
}
