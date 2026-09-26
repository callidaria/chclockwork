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
	GPUPixelBuffer* __GoldColourTexture = g_Renderer.register_texture("./res/test/gold_colour.png");
	GPUPixelBuffer* __GoldNormalTexture = g_Renderer.register_texture("./res/test/gold_normal.png");
	GPUPixelBuffer* __GoldMaterialTexture = g_Renderer.register_texture("./res/test/gold_material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

	// texture assignment
	__EnviroBatch->pcm = &m_MeshData;

	// register textures
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],0,__GoldColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],1,__GoldNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[2],2,__GoldMaterialTexture);
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
