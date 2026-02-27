#include "core/blitter.h"
#include "core/input.h"
#include "core/renderer.h"
#include "core/ui.h"
#include "core/wheel.h"

// engine
//#ifdef DEBUG
#include "script/clockwork.h"
//#endif

#include "script/test.h"


s32 main(s32 argc,char** argv)
{
#ifdef VKBUILD
	Clockwork __Clockwork = Clockwork();
	bool running = true;
	while (running)
	{
		g_Frame.clear();
		g_Input.update(running);
		g_Wheel.update();
		g_Camera.update();
		g_Renderer.update();
		g_Frame.update();
	}

#else
	Font* __Ubuntu = g_Renderer.register_font("./res/font/ubuntu.ttf",20);

	// engine components
//#ifdef DEBUG
	Clockwork __Clockwork = Clockwork(__Ubuntu);
//#endif

	// scripts
	TestScene __Test = TestScene();

	bool running = true;
	while (running)
	{
		g_Frame.clear();
		g_Input.update(running);
		g_Renderer.precalculate();
		g_Wheel.update();
		g_Camera.update();
		g_UI.update();
		g_Renderer.update();
		g_Frame.update();
	}
#endif

	g_Renderer.vanish();
	g_Frame.close();
	return 0;
}
