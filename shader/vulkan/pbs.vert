#version 450 core


layout(location = 0) in vec2 position;
layout(location = 1) in vec2 edge_coordinates;

layout(location = 0) out vec2 EdgeCoordinates;


void main()
{
	gl_Position = vec4(position,0,1.);
	EdgeCoordinates = edge_coordinates;
}
