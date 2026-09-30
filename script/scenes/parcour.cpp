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
	u32 __CubeID = __EnviroBatch->add_geometry(__Cube,__Textures);
	u32 __SphereID = __EnviroBatch->add_geometry(__Sphere,__Textures);
	__EnviroBatch->load();

	// transform objects
	__EnviroBatch->objects[__CubeID].transform.transform(vec3(0,0,1.5f),vec3(10,10,1),vec3(.0f));

	// load textures
	GPUPixelBuffer* __FabricColourTexture = g_Renderer.register_texture("./res/test/fabric_colour.png",
																		TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FabricNormalTexture = g_Renderer.register_texture("./res/test/fabric_normal.png");
	GPUPixelBuffer* __FabricMaterialTexture = g_Renderer.register_texture("./res/test/fabric_material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

	// setup lighting
	g_Renderer.add_sunlight(vec3(10,10,10),vec3(1,1,1),1.f);
	//g_Renderer.add_pointlight(vec3(4,4,4),vec3(1,1,1),10.f,100.f,10.f,.4f);
	//g_Renderer.add_pointlight(vec3(-4,4,-4),vec3(.4f,1,.8f),10.f,100.f,10.f,.4f);

	// register textures
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],0,__FabricColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],1,__FabricNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],2,__FabricMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],3,__NeutralEmissionTexture);

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
