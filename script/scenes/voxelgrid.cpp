#include "voxelgrid.h"


/**
 *	TODO
 */
void RoomVoxels::init(Font* font)
{
	// load confirmation text
	lptr<Text> __Text = g_Renderer.write_text(font,"Loading Affirmation Text",vec3(10,-10,7),15,vec4(1),
											  Alignment{ .alignment=SCREEN_ALIGN_TOPLEFT });

	// setup batch
	lptr<ShaderPipeline> __RoomShader
		= g_Renderer.register_pipeline("./shader/vulkan/bin/mesh.vert","./shader/vulkan/bin/mesh.frag",1,true);
	lptr<GeometryBatch> __RoomBatch = g_Renderer.register_geometry_batch(__RoomShader);

	// load room
	Mesh __RoomMesh = Mesh("./res/private/test.obj");
	GPUPixelBuffer* __RoomTexture = g_Renderer.register_texture("./res/private/test.png");
	vector<Texture*> __RoomTextures = { };
	__RoomBatch->add_geometry(__RoomMesh,__RoomTextures);
	__RoomBatch->load();

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
