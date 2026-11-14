#version 330 core


in vec2 position;
in vec2 edge_coordinates;

out vec2 EdgeCoordinates;


void main()
{
	gl_Position = vec4(position,.0,1.);
	EdgeCoordinates = edge_coordinates;
}
