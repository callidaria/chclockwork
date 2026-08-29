#ifndef SCRIPT_VOXELGRID_HEADER
#define SCRIPT_VOXELGRID_HEADER


#include "../../core/renderer.h"
#include "../../core/input.h"
#include "../../core/wheel.h"


struct UploadData
{
	u32 texture;
};


class RoomVoxels
{
public:
	void init(Font* font);
	void update();
	void vanish();

private:
	UploadData m_TextureData;
};


#endif
