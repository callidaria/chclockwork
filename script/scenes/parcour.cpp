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

	// load textures
	GPUPixelBuffer* __GoldColourTexture = g_Renderer.register_texture("./res/test/gold_colour.png");
	GPUPixelBuffer* __GoldNormalTexture = g_Renderer.register_texture("./res/test/gold_normal.png");
	GPUPixelBuffer* __GoldMaterialTexture = g_Renderer.register_texture("./res/test/gold_material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

	// texture assignment
	m_Materials.colour = __GoldColourTexture->memID;
	m_Materials.normal = __GoldNormalTexture->memID;
	m_Materials.material = __GoldMaterialTexture->memID;
	m_Materials.emission = __NeutralEmissionTexture->memID;
	__EnviroBatch->pcm = &m_Materials;

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
