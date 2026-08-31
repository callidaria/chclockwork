#include "voxelgrid.h"


const s32 TEST_INSTANCE_AMOUNT_X = 3;
const s32 TEST_INSTANCE_AMOUNT_Y = 3;
const s32 TEST_INSTANCE_AMOUNT_Z = 3;
const s32 TEST_INSTANCE_AMOUNT_GENERAL
			= TEST_INSTANCE_AMOUNT_X*TEST_INSTANCE_AMOUNT_Y*TEST_INSTANCE_AMOUNT_Z;


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
	lptr<ParticleBatch> __RoomBatch = g_Renderer.register_particle_batch(__RoomShader);

	// load room
	Mesh __RoomMesh = Mesh("./res/private/test.obj");
	GPUPixelBuffer* __RoomTexture = g_Renderer.register_texture("./res/private/test.png");
	__RoomBatch->load(__RoomMesh,TEST_INSTANCE_AMOUNT_GENERAL,sizeof(vec3));

	// select texture
	m_TextureData.texture = __RoomTexture->memID;
	__RoomBatch->pcm = &m_TextureData;

	// grid instances
	u32 i = 0;
	vec3 __Instances[TEST_INSTANCE_AMOUNT_GENERAL] = {  };
	for (s32 z=-TEST_INSTANCE_AMOUNT_Z/2;z<(TEST_INSTANCE_AMOUNT_Z/2)+TEST_INSTANCE_AMOUNT_Z%2;z++)
	{
		for (s32 y=-TEST_INSTANCE_AMOUNT_Y/2;y<(TEST_INSTANCE_AMOUNT_Y/2)+TEST_INSTANCE_AMOUNT_Y%2;y++)
		{
			for (s32 x=-TEST_INSTANCE_AMOUNT_X/2;x<(TEST_INSTANCE_AMOUNT_X/2)+TEST_INSTANCE_AMOUNT_Z%2;x++)
			{
				__Instances[i] = { .position = vec3(x*2,y*2,z*2) };
				i++;
			}
		}
	}
	__RoomBatch->ibo.upload(__Instances,sizeof(__Instances));

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
