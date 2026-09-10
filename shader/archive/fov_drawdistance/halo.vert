#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;
// FIXME wayy to much info

// engine: ibo
in vec3 offset;
in float scale;
in vec2 tex_offset;
in vec2 tex_dimension;

out vec3 Position;
out vec2 UV;
out vec3 Normal;

uniform mat4 view;
uniform mat4 proj;


void main()
{
	Position = position*scale+offset;
	gl_Position = proj*view*vec4(Position,1.);

	// pass
	Position = normalize(Position);
	Normal = normal;

	// calculate precision texture uv
	float eps = .001;
	UV = tex_offset+tex_dimension*clamp(uv,eps,1.-eps);
}
