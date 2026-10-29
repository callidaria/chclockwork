#include "spriteworks.h"


/**
 *	setup spriteworks test scene
 *	\param font: engine font
 */
void SpriteWorks::init(Font* font)
{
	Rect* __CursedBird = g_Renderer.register_sprite_texture("./res/test/cld.jpeg");
	Sprite* __TestSprite0 = g_Renderer.register_sprite(__CursedBird,vec3(100,-100,2),vec2(100,100),.0f,1.f,
													   { .alignment=SCREEN_ALIGN_TOPLEFT });
	Sprite* __TestSprite1 = g_Renderer.register_sprite(__CursedBird,vec3(0,0,5),vec2(100,100),.0f,.25f,
													   { .alignment=SCREEN_ALIGN_CENTER });
	Sprite* __TestSprite2 = g_Renderer.register_sprite(__CursedBird,vec3(-250,250,4),vec2(120,120),.0f,1.f,
													   { .alignment=SCREEN_ALIGN_BOTTOMRIGHT });
	g_Wheel.call(this);
}

/**
 *	update spriteworks test scene
 */
void SpriteWorks::update()
{
}

/**
 *	unload spriteworks test scene
 */
void SpriteWorks::vanish()
{
}
