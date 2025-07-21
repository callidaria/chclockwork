#version 330 core


in vec3 position;
in vec2 edge_coordinates;
in vec3 normals;
in vec3 tangent;

out vec3 Position;
out vec2 EdgeCoordinates;
out mat3 TBN;

uniform mat4 view;
uniform mat4 proj;

uniform mat4 model;
uniform float texel = 1.;


void main()
{
	vec4 world_position = model*vec4(position,1.);
	Position = world_position.xyz;
	gl_Position = proj*view*world_position;

	// calculate texture coordinates
	EdgeCoordinates = edge_coordinates*texel;

	// gram-schmidt orthogonalization
	vec3 Tangent = normalize((model*vec4(tangent,0)).xyz);
	vec3 Normals = normalize((model*vec4(normals,0)).xyz);
	Tangent = normalize(Tangent-dot(Tangent,Normals)*Normals);
	vec3 Bitangent = cross(Normals,Tangent);
	TBN = mat3(Tangent,Bitangent,Normals);
}
