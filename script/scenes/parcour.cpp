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
	lptr<ShaderPipeline> __AnimationPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,"./shader/vulkan/bin/animation.vert","./shader/vulkan/bin/gpass.frag",
			gformats,RENDERER_GBUFFER_FORMAT_SIZE,true
		);
	lptr<ShaderPipeline> __BulbShadowPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,"./shader/vulkan/bin/bulb_shadow.vert","./shader/vulkan/bin/shadow.frag",
			nullptr,0,true,true
		);
	lptr<ShaderPipeline> __AnimationShadowPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,
			"./shader/vulkan/bin/animation_shadow.vert","./shader/vulkan/bin/shadow.frag",
			nullptr,0,true,true
		);
	// TODO maybe solve the last parameters by type (TYPE_SHADOW,TYPE_GPASS,TYPE_FORWARD, et cetera)
	// TODO namespace gformats by adding the constant inside the renderer class

	// batches
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch();
	lptr<ParticleBatch> __BulbBatch = g_Renderer.register_deferred_particle_batch(__BulbPipeline);
	m_AnimBatch = g_Renderer.register_deferred_geometry_batch(__AnimationPipeline);

	// setup geometry batch
	vector<Texture*> __Textures = {  };
	Mesh __Sphere = Mesh::sphere();
	Mesh __Cube = Mesh::cube();
	u32 __CubeID = __EnviroBatch->add_geometry(__Cube,__Textures);
	u32 __SphereID = __EnviroBatch->add_geometry(__Sphere,__Textures);
	__EnviroBatch->load();
	// FIXME remove textures as they are passed right now

	// setup animated character batch
	m_DudeID = m_AnimBatch->add_geometry(m_Dude,__Textures);
	m_AnimBatch->load();
	g_Frame.time_factor = 2.f;  // FIXME yes, this is bad
	m_Dude.set_default_animation(3,.4f);
	m_SpineJointTransform = &m_Dude.find_joint("metarig_spine")->transform;
	g_Renderer.animate(&m_Dude);

	// transform objects
	__EnviroBatch->objects[__CubeID].transform.scale(vec3(10,10,.1f));
	__EnviroBatch->objects[__CubeID].transform.translate(vec3(0,0,-.1f));
	__EnviroBatch->data[__CubeID].texel = 10.f;
	__EnviroBatch->objects[__SphereID].transform.transform(vec3(0,-4,.5f),vec3(.5f),vec3(.0f));

	// load textures
	GPUPixelBuffer* __FabricColourTexture = g_Renderer.register_texture("./res/test/fabric_colour.png",
																		TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FabricNormalTexture = g_Renderer.register_texture("./res/test/fabric_normal.png");
	GPUPixelBuffer* __FabricMaterialTexture = g_Renderer.register_texture("./res/test/fabric_material.png");
	GPUPixelBuffer* __FloorColourTexture = g_Renderer.register_texture("./res/test/floor_colour.png",
																	   TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FloorNormalTexture = g_Renderer.register_texture("./res/test/floor_normal.png");
	GPUPixelBuffer* __FloorMaterialTexture = g_Renderer.register_texture("./res/test/floor_material.png");
	GPUPixelBuffer* __DudeTexture = g_Renderer.register_texture("./res/private/texmex.jpg",TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __NeutralNormalMap = g_Renderer.register_texture("./res/standard/normal.png");
	GPUPixelBuffer* __NeutralMaterialMap = g_Renderer.register_texture("./res/standard/material.png");
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
	__BulbBatch->load(__Sphere,TEST_LIGHTBULB_COUNT,sizeof(BallIndex));
	__BulbBatch->ibo.upload(__BulbIndices,sizeof(__BulbIndices));

	// setup lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1.f),.2f,true);
	for (u8 i=0;i<TEST_LIGHTBULB_COUNT;i++)
		g_Renderer.add_pointlight(__BulbIndices[i].position,__BulbIndices[i].colour,7.5f,1.f,.8f,.24f);

	// register textures
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],0,__FloorColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],1,__FloorNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],2,__FloorMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__CubeID][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],0,__FabricColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],1,__FabricNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],2,__FabricMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],0,__DudeTexture);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],1,__NeutralNormalMap);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],2,__NeutralMaterialMap);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],3,__NeutralEmissionTexture);

	// shadows
	g_Renderer.register_shadow_batch(__EnviroBatch);
	g_Renderer.register_shadow_batch(__BulbBatch,__BulbShadowPipeline);
	g_Renderer.register_shadow_batch(m_AnimBatch,__AnimationShadowPipeline);

	g_Wheel.call(this);
}

/**
 *	TODO
 */
void ParcourParcs::update()
{
	vec3 __CenteredPosition = vec3((*m_SpineJointTransform)[3]);

	// camera
	g_Camera.target = m_PlayerPosition;

	// player transformation
	m_AnimBatch->objects[m_DudeID].transform.reset();
	m_AnimBatch->objects[m_DudeID].transform.rotate_x(90.f);
	m_AnimBatch->objects[m_DudeID].transform.translate(-__CenteredPosition+m_PlayerPosition);
}

/**
 *	TODO
 */
void ParcourParcs::vanish()
{
	// TODO
}
