#version 450 core


layout(location = 0) in vec3 Position;

layout(location = 0) out vec4 result_colour;

layout(set = 2,binding = 0) uniform sampler2D stcm;

const vec2 atan_inv = vec2(.1591,.3183);  // (1/2PI,1/PI) norm angular into uv range


vec2 acquire_uv_coordinate(vec3 pos);

void main()
{
	result_colour = vec4(texture(stcm,acquire_uv_coordinate(normalize(Position))).rgb,1.);
}

vec2 acquire_uv_coordinate(vec3 pos)
{
	return vec2(atan(pos.y,pos.x),asin(pos.z))*atan_inv+.5;
}
