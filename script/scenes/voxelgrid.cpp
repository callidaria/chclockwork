#include "voxelgrid.h"


/**
 *	TODO
 */
RoomVoxels::RoomVoxels(Font* font)
{
	lptr<Text> __Text = g_Renderer.write_text(font,"Loading Affirmation Text",vec3(0,0,7),15,vec4(1),
											  Alignment{ .alignment=SCREEN_ALIGN_CENTER });
	g_Wheel.call(this);
}

/**
 *	TODO
 */
void RoomVoxels::update()
{
	// TODO
}

/**
 *	TODO
 */
void RoomVoxels::vanish()
{
	// TODO
}
