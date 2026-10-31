#include "core/blitter.h"
#include "core/input.h"
#include "core/renderer.h"
#include "core/ui.h"
#include "core/wheel.h"

// engine
#ifdef DEBUG
#include "script/clockwork.h"
#endif
#include "script/test.h"


s32 main(s32 argc,char** argv)
{
	// SETUP
	Font* __EngineFont = g_Renderer.register_font("./res/font/garamond.ttf",50);	
#ifdef DEBUG
	Clockwork __Clockwork = Clockwork(__EngineFont);
#endif
	SceneListing __Test = SceneListing(__EngineFont);

	// RUN
	bool running = true;
	while (running)
	{
		g_Input.update(running);
		g_Camera.update();
		g_Frame.clear();
		g_Renderer.precalculate();
		g_Wheel.update();
		g_UI.update();
		g_Renderer.update();
#ifdef VKBUILD
		g_GPU.update(&g_Frame.render_done[g_Frame.frame_id]);
#endif
		g_Frame.update();
#ifdef VKBUILD
		if (running) g_GPU.swap();
		// TODO this is not quite efficient to do it right after frame update, but it's circumventing a flaw
		//		in the render system right now, where initial uploads are executed at construction before loop
		//		this will naturally solve itself, once a real streaming system is setup, due to upload scheduling
		// FIXME also the condition is a workaround for the same reasons
		g_Frame.fid++;
#endif
	}

	// CLOSE
	g_Input.vanish();
	g_Renderer.vanish();
	g_Frame.close();
	return 0;
}
