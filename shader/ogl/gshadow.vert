#version 330 core


in vec3 position;
in vec2 edge_coordinates;
in vec3 normals;
in vec3 tangent;

uniform mat4 view;
uniform mat4 proj;

uniform mat4 model;


void main()
{
	gl_Position = proj*view*model*vec4(position,1.);
}
