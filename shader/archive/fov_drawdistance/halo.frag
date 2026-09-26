#version 330 core


in vec3 Position;
in vec2 UV;
in vec3 Normal;

layout(location = 0) out vec4 pixelColour;

uniform sampler2D tex;


void main()
{
	// load texture
	pixelColour = texture(tex,UV);
}
