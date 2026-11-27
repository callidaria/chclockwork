#version 450 core


layout(location = 0) in vec3 Position;

layout(location = 0) out vec4 result_colour;

layout(set = 2,binding = 0) uniform samplerCube skybox;


void main()
{
	result_colour = texture(skybox,Position);
}
