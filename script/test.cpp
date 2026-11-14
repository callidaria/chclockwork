#include "test.h"


/**
 *	setup scene listing
 *	\param font: engine font
 */
SceneListing::SceneListing(Font* font)
	: m_Font(font)
{
	// textures
	Rect* __ButtonBase = g_Renderer.register_sprite_texture("./res/ui/button_base.png");
	Rect* __ButtonHover = g_Renderer.register_sprite_texture("./res/ui/button_hover.png");
	Rect* __ButtonClick = g_Renderer.register_sprite_texture("./res/ui/button_click.png");
	// TODO register instead as a tuple, then refer to the button style anywhere in the program

	// heading
	lptr<Text> __Text = g_Renderer.write_text(font,"Test Scenes",vec3(0,-10,7),15,vec4(1),
											  Alignment{ .alignment=SCREEN_ALIGN_TOPCENTER });

	// selection scene ui
	m_UI = g_UI.add_batch(font);
	m_BTNTextures = m_UI->add_button("texture playground",__ButtonBase,__ButtonHover,__ButtonClick,
									 vec3(0,25,7),vec2(250,25),Alignment{ .alignment=SCREEN_ALIGN_CENTER });
	m_BTNVoxel = m_UI->add_button("voxelmatrix projection",__ButtonBase,__ButtonHover,__ButtonClick,
								  vec3(0,0,7),vec2(250,25),Alignment{ .alignment=SCREEN_ALIGN_CENTER });
	m_BTNParcour = m_UI->add_button("parcour parcs",__ButtonBase,__ButtonHover,__ButtonClick,
									vec3(0,-25,7),vec2(250,25),Alignment{ .alignment=SCREEN_ALIGN_CENTER });

	m_Self = g_Wheel.call(this);
}

/**
 *	check for scene requests by user
 */
void SceneListing::update()
{
	u8 __Action = m_BTNTextures->confirm+m_BTNVoxel->confirm*2+m_BTNParcour->confirm*3;
	if (!__Action) return;

	// load requested scene
	switch (__Action)
	{
	case 1: m_SpriteWorks.init(m_Font);
		break;
	case 2: m_RoomVoxels.init(m_Font);
		break;
	case 3: m_ParcourParcs.init();
		break;
	};

	g_Wheel.stop(m_Self);
}

/**
 *	destruct test scene
 */
void SceneListing::vanish()
{
	g_UI.remove_batch(m_UI);
}
