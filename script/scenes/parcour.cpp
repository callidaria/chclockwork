#include "parcour.h"


/**
 *	TODO
 */
void ParcourParcs::init()
{
	// pipelines
	lptr<ShaderPipeline> __BulbPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,"./shader/vulkan/bin/bulb.vert","./shader/vulkan/bin/bulb.frag",
			gformats,RENDERER_GBUFFER_FORMAT_SIZE,true
		);

	// batches
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch();
	lptr<ParticleBatch> __BulbBatch = g_Renderer.register_deferred_particle_batch(__BulbPipeline);

	// setup batch
	vector<Texture*> __Textures = {  };
	Mesh __Sphere = Mesh::sphere();
	Mesh __Cube = Mesh::cube();
	u32 __SphereID = __EnviroBatch->add_geometry(__Sphere,__Textures);
	u32 __CubeID = __EnviroBatch->add_geometry(__Cube,__Textures);
	__EnviroBatch->load();

	// transform objects
	__EnviroBatch->objects[__SphereID].transform.transform(vec3(0,0,1.2f),vec3(1,1,1),vec3(.0f));
	__EnviroBatch->objects[__CubeID].texel = 20.f;
	__EnviroBatch->objects[__CubeID].transform.scale(vec3(10,10,.2f));

	// load textures
	GPUPixelBuffer* __FabricColourTexture = g_Renderer.register_texture("./res/test/fabric_colour.png",
																		TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FabricNormalTexture = g_Renderer.register_texture("./res/test/fabric_normal.png");
	GPUPixelBuffer* __FabricMaterialTexture = g_Renderer.register_texture("./res/test/fabric_material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

	// lightbulbs
	BallIndex __BulbIndices[TEST_LIGHTBULB_COUNT] = {
		{ vec3(9,9,.2f),.2f,vec3(1,0,0),vec2(0,.4f) },
		{ vec3(9,-9,.7f),.2f,vec3(1,1,0),vec2(0,.4f) },
		{ vec3(-9,9,1.2f),.2f,vec3(0,1,0),vec2(0,.4f) },
		{ vec3(-9,-9,.2f),.2f,vec3(0,1,1),vec2(0,.4f) },
		{ vec3(5,9,.7f),.2f,vec3(0,0,1),vec2(0,.4f) },
		{ vec3(5,-9,1.2f),.2f,vec3(1,0,1),vec2(0,.4f) },
		{ vec3(-9,5,.2f),.2f,vec3(.5f,.5f,0),vec2(0,.4f) },
		{ vec3(-9,-5,.7f),.2f,vec3(0,.5f,.5f),vec2(0,.4f) },
	};

	// setup lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1.f),.5f,true);
	for (u8 i=0;i<TEST_LIGHTBULB_COUNT;i++)
		g_Renderer.add_pointlight(__BulbIndices[i].position,__BulbIndices[i].colour,10.f,1.f,.8f,.24f);

	// register textures
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],0,__FabricColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],1,__FabricNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],2,__FabricMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],0,__FabricColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],1,__FabricNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],2,__FabricMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],3,__NeutralEmissionTexture);

	g_Wheel.call(this);
}

/**
 *	TODO
 */
void ParcourParcs::update()
{
	// TODO
}

/**
 *	TODO
 */
void ParcourParcs::vanish()
{
	// TODO
}
