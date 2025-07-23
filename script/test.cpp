#include "test.h"


/**
 *	setup test scene
 */
TestScene::TestScene()
{
	// resources
	vector<Texture*> __DudeTexture = {
		g_Renderer.register_texture("./res/test/dude_colour.png",TEXTURE_FORMAT_SRGB)
	};

	// shaders
	VertexShader __AnimationVertexShader = VertexShader("./core/shader/animation.vert");
	FragmentShader __AnimationFragmentShader = FragmentShader("./core/shader/animation.frag");
	lptr<ShaderPipeline> __AnimationShader = g_Renderer.register_pipeline(__AnimationVertexShader,
																		  __AnimationFragmentShader);

	// animation batch
	lptr<GeometryBatch> __AnimationBatch = g_Renderer.register_geometry_batch(__AnimationShader);
	u32 __DudeID = __AnimationBatch->add_geometry(m_Dude,__DudeTexture);
	__AnimationBatch->load();
	__AnimationBatch->object[__DudeID].transform.rotate_x(-90.f);

	// lighting
	g_Renderer.add_sunlight(vec3(75,-50,100),vec3(1),1.f);
	g_Renderer.upload_lighting();

	g_Wheel.call(UpdateRoutine{ &TestScene::_update,(void*)this });
}

/**
 *	update test scene
 */
void TestScene::update()
{
	m_Dude.update();
}
