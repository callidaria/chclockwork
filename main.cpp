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
	// FIXME frame_id is never increased?
	// TODO also wait on setup done
	// TODO abstract fence & semaphore creation?
	//g_Frame.clear();  // TODO implement a way to allow graphics operation without image acquisition
	g_GPU.submit_transfer();
	g_GPU.submit_graphics(&g_Frame.setup_done);
	//g_GPU.update();
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
		g_Frame.update();
	}

	// CLOSE
	g_Input.vanish();
	g_Renderer.vanish();
	g_Frame.close();
	return 0;
}
