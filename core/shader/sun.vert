#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;
// FIXME most of this can be removed for performance gains

out vec2 UV;

uniform vec3 offset = vec3(0);
uniform float scale = 1.f;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	vec3 Position = position+offset;
	gl_Position = proj*view*vec4(Position*scale,1.);
	UV = uv;
}
