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
	Font* __Ubuntu = g_Renderer.register_font("./res/font/ubuntu.ttf",20);

	// testing
#ifdef VKBUILD
	ShaderPipeline* __TestingPipeline = new ShaderPipeline();  // FIXME obviously not how it will be used later
	__TestingPipeline->assemble("./core/shader/vulkan/bin/triangle.vert",
								"./core/shader/vulkan/bin/triangle.frag");
	g_Vk.register_pipeline(__TestingPipeline->render_pass);
#else

	// engine components
#ifdef DEBUG
	Clockwork __Clockwork = Clockwork(__Ubuntu);
#endif
	TestScene __Test = TestScene();
#endif

	bool running = true;
	while (running)
	{
		g_Frame.clear();
		g_Input.update(running);
		g_Wheel.update();
		g_Camera.update();
#ifdef VKBUILD
		__TestingPipeline->render();
#endif
		g_UI.update();
		g_Renderer.update();
		g_Frame.update();
	}

#ifdef VKBUILD
	delete __TestingPipeline;
#endif

	g_Renderer.exit();
	g_Frame.close();
	return 0;
}
