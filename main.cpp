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
	Font* __Ubuntu = g_Renderer.register_font("./res/font/ubuntu.ttf",15);
	Clockwork __Clockwork = Clockwork(__Ubuntu);
	TestScene __Test = TestScene(__Ubuntu);
	bool running = true;
	while (running)
	{
		g_Input.update(running);
		g_Wheel.update();
		g_Camera.update();
		g_Frame.clear();
		g_UI.update();
		g_Renderer.update();
		g_GPU.update(&g_Frame.render_done[g_Frame.frame_id]);
		g_Frame.update();
		if (running) g_GPU.swap();
		// TODO this is not quite efficient to do it right after frame update, but it's circumventing a flaw
		//		in the render system right now, where initial uploads are executed at construction before loop
		//		this will naturally solve itself, once a real streaming system is setup, due to upload scheduling
		// FIXME also the condition is a workaround for the same reasons
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

	g_Input.vanish();
	g_Renderer.vanish();
	g_Frame.close();
	return 0;
}
