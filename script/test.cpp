#include "test.h"


/**
 *	setup test scene
 */
TestScene::TestScene()
{
	// resources
	vector<Texture*> __DudeTexture = {
		g_Renderer.register_texture("./res/test/texmex.jpg",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/standard/normal.png"),
		g_Renderer.register_texture("./res/standard/material.png"),
	};
	vector<Texture*> __FloorTexture = {
		g_Renderer.register_texture("./res/test/floor_colour.png",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/test/floor_normal.png"),
		g_Renderer.register_texture("./res/test/floor_material.png"),
	};
	// TODO pre-store standard texture fallbacks

	// geometry
	Mesh __Cube = Mesh::cube();

	// shaders
	VertexShader __AnimationVertexShader = VertexShader("./core/shader/animation.vert");
	VertexShader __AnimationShadowShader = VertexShader("./core/shader/animation_shadow.vert");
	FragmentShader __AnimationFragmentShader = FragmentShader("./core/shader/gpass.frag");
	FragmentShader __ShadowShader = FragmentShader("./core/shader/shadow.frag");
	lptr<ShaderPipeline> __AnimationShader = g_Renderer.register_pipeline(__AnimationVertexShader,
																		  __AnimationFragmentShader);
	lptr<ShaderPipeline> __AnimationShadowPipeline = g_Renderer.register_pipeline(__AnimationShadowShader,
																				  __ShadowShader);

	// animation batch
	lptr<GeometryBatch> __AnimationBatch = g_Renderer.register_deferred_geometry_batch(__AnimationShader);
	u32 __DudeID = __AnimationBatch->add_geometry(m_Dude,__DudeTexture);
	__AnimationBatch->load();
	__AnimationBatch->objects[__DudeID].transform.rotate_x(90.f);
	g_Renderer.register_shadow_batch(__AnimationBatch,__AnimationShadowPipeline);

	// geometry batch
	lptr<GeometryBatch> __PhysicalBatch = g_Renderer.register_deferred_geometry_batch();
	u32 __FloorID = __PhysicalBatch->add_geometry(__Cube,__FloorTexture);
	u32 __BoxID = __PhysicalBatch->add_geometry(__Cube,__FloorTexture);
	__PhysicalBatch->load();
	g_Renderer.register_shadow_batch(__PhysicalBatch);

	// environment scale
	__PhysicalBatch->objects[__FloorID].transform.scale(glm::vec3(10,10,.1f));
	__PhysicalBatch->objects[__FloorID].transform.translate(glm::vec3(0,0,-.1f));
	__PhysicalBatch->objects[__FloorID].texel = 10.f;
	__PhysicalBatch->objects[__BoxID].transform.translate(glm::vec3(4,4,1));

	// lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1),4.f);
	g_Renderer.upload_lighting();
	// FIXME the sunlight emission is coming from a left handed coordinate system somehow (y-axis flip)

	g_Wheel.call(UpdateRoutine{ &TestScene::_update,(void*)this });
}

/**
 *	update test scene
 */
void TestScene::update()
{
	m_Dude.update();
}
