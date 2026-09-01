#include "parcour.h"


/**
 *	TODO
 */
void ParcourParcs::init()
{
	// setup batch
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch();

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
