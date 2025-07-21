#ifndef CORE_BLITTER_HEADER
#define CORE_BLITTER_HEADER


#include "base.h"


constexpr vec3 BLITTER_CLEAR_COLOUR = vec3(0);


class Frame
{
public:
	Frame(const char* title,u16 width,u16 height,bool vsync=true);

	static void clear();
	void update();
	void close();

	void gpu_vsync_on();
	void gpu_vsync_off();

#ifdef DEBUG
public:
	u32 fps;
private:
	std::chrono::steady_clock::time_point m_LastFrameUpdate = std::chrono::steady_clock::now();
	u32 m_LFps;
#endif

private:
	SDL_Window* m_Frame;
	SDL_GLContext m_Context;
};

inline Frame g_Frame = Frame("BHT Projekt",FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,false);


#endif
