#version 450 core


layout(location = 0) in vec2 position;
layout(location = 1) in vec2 edge_coordinates;

// engine: ibo
layout(location = 2) in vec3 offset;
layout(location = 3) in vec2 scale;
layout(location = 4) in vec2 bearing;
layout(location = 5) in vec4 colour;
layout(location = 6) in vec2 atlas_position;
layout(location = 7) in vec2 atlas_dimension;

layout(location = 0) out vec2 EdgeCoordinates;
layout(location = 1) out vec4 Colour;

layout(set = 0,binding = 3) uniform SpriteTransformation
{
	mat4 view;
	mat4 proj;
} csys;


void main()
{
	vec2 Position = position+vec2(.5,.5);
	Position = Position*scale+offset.xy+bearing;
	gl_Position = csys.proj*csys.view*vec4(Position.x,Position.y-scale.y,offset.z,1.);

	// pass
	EdgeCoordinates = vec2(edge_coordinates.x,1-edge_coordinates.y);
	EdgeCoordinates = atlas_position+atlas_dimension*EdgeCoordinates;
	Colour = colour;
}

// TODO there must be a clear definition how the text is aligned towards its position in shader
// 	it cannot be normal that i have to guess here based on some vector flips between versions
