#ifndef SCRIPT_TEST_HEADER
#define SCRIPT_TEST_HEADER

#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/ui.h"
#include "../core/wheel.h"

#include "scenes/spriteworks.h"
#include "scenes/voxelgrid.h"
#include "scenes/parcour.h"


class SceneListing
{
public:
	SceneListing(Font* font);
	void update();
	void vanish();

private:
	// ui
	Font* m_Font;
	lptr<UIBatch> m_UI;
	lptr<Button> m_BTNTextures,m_BTNVoxel,m_BTNParcour;

	// scenes
	SpriteWorks m_SpriteWorks;
	RoomVoxels m_RoomVoxels;
	ParcourParcs m_ParcourParcs;

	// self reference
	lptr<UpdateRoutine> m_Self;
};


#endif
