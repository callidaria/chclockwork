#version 330 core


in vec3 position;
in vec2 uv;
in vec3 normal;
in vec3 tangent;

// engine: ibo
in vec3 offset;
in float scale;
in vec2 tex_offset;
in vec2 tex_dimension;

out vec3 Position;
out vec2 UV;
out vec3 Normal;
out mat3 TBN;
// TODO remove normal pass overuse
// TODO actually replace this by a particle gpass shader serving the deferred pipeline

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
	UV = tex_offset+tex_dimension*clamp(vec2(1.-uv.x,uv.y),eps,1.-eps);

	// gram-schmidt reothogonalization
	vec3 Tangent = normalize(tangent-dot(tangent,normal)*normal);
	TBN = mat3(Tangent,cross(normal,Tangent),normal);
	// FIXME completely unnecessary
}
