#include "parcour.h"


/**
 *	TODO
 */
void ParcourParcs::init()
{
	// setup batch
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch();
	vector<Texture*> __Textures = {  };
	Mesh __Sphere = Mesh::sphere();
	__EnviroBatch->add_geometry(__Sphere,__Textures);
	__EnviroBatch->load();

	// load textures
	GPUPixelBuffer* __FloorColourTexture = g_Renderer.register_texture("./res/test/floor_colour.png");
	GPUPixelBuffer* __FloorNormalTexture = g_Renderer.register_texture("./res/test/floor_normal.png");
	GPUPixelBuffer* __FloorMaterialTexture = g_Renderer.register_texture("./res/test/floor_material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

	// setup lighting
	g_Renderer.add_pointlight(vec3(1,1,1),vec3(1,1,1),10.f,100.f,10.f,.4f);
	g_Renderer.add_pointlight(vec3(-1,1,-1),vec3(.4f,1,.8f),10.f,100.f,10.f,.4f);

	// texture assignment (old, remove)
	__EnviroBatch->pcm = &m_MeshData;

	// register textures
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],0,__FloorColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],1,__FloorNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],2,__FloorMaterialTexture);
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
