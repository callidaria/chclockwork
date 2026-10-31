#version 330 core


in vec3 Position;
in vec2 UV;
in vec3 Normal;

layout(location = 0) out vec4 pixelColour;

uniform sampler2D tex;

uniform vec3 light_position;


// lighting constants
const float TerminatorSharpness = .4;


void main()
{
	float light_intensity = max(dot(Normal,normalize(light_position-Position)),.0);
	light_intensity = pow(light_intensity,TerminatorSharpness);
	vec4 density = texture(tex,UV);
	float dc = min(length(density.rgb),1.);
	pixelColour = vec4(density.rgb*light_intensity,dc);
}
// FIXME now it looks awful. more correct but awful!
