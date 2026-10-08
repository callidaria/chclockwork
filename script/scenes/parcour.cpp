#include "parcour.h"


/**
 *	TODO
 */
void ParcourParcs::init()
{
	// setup batch
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch();
	vector<Texture*> __Textures = {  };
	Mesh __Cube = Mesh::cube();
	Mesh __Sphere = Mesh::sphere();
	u32 __SphereID = __EnviroBatch->add_geometry(__Sphere,__Textures);
	u32 __CubeID = __EnviroBatch->add_geometry(__Cube,__Textures);
	__EnviroBatch->load();

	// transform objects
	__EnviroBatch->objects[__SphereID].transform.transform(vec3(0,0,1.5f),vec3(1,1,1),vec3(.0f));
	__EnviroBatch->objects[__CubeID].texel = 20.f;
	__EnviroBatch->objects[__CubeID].transform.scale(vec3(10,10,.2f));

	// load textures
	GPUPixelBuffer* __FabricColourTexture = g_Renderer.register_texture("./res/test/fabric_colour.png",
																		TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FabricNormalTexture = g_Renderer.register_texture("./res/test/fabric_normal.png");
	GPUPixelBuffer* __FabricMaterialTexture = g_Renderer.register_texture("./res/test/fabric_material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

	// setup lighting
	g_Renderer.add_sunlight(vec3(20,20,40),vec3(1,1,1),1.f);
	//g_Renderer.add_pointlight(vec3(4,4,4),vec3(1,1,1),10.f,100.f,10.f,.4f);
	//g_Renderer.add_pointlight(vec3(-4,4,-4),vec3(.4f,1,.8f),10.f,100.f,10.f,.4f);
	// TODO offcenter position + sun behaves like a pointlight? this is all wrong

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
