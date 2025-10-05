#ifndef CORE_BLITTER_HEADER
#define CORE_BLITTER_HEADER


#include "hardware.h"


constexpr vec3 BLITTER_CLEAR_COLOUR = vec3(.0f,.0f,.0f);


class Frame
{
public:
	Frame(const char* title,u16 width,u16 height,bool vsync=true);

	// utilty
	static void clear();
	void update();
	void close();

	// settings
	void set_clear_colour(vec3 colour);
	void set_clear_depth(f32 depth);
	void set_viewport(u32 width,u32 height);

	// framerate
	void gpu_vsync_on();
	void gpu_vsync_off();

#ifdef VKBUILD
	void rebuild_swapchain();
	void link_result(VkRenderPass render_pass);
#endif

private:

#ifdef VKBUILD
	void _assemble_swapchain();
	void _finalize_swapchain();
	void _destroy_swapchain();
#endif

public:

	// time
	f32 delta_time_real = .0f;
	f32 delta_time = .0f;
	f32 time_factor = 1.f;

private:

	// hardware
	SDL_Window* m_Frame;

	// time
	std::chrono::steady_clock::time_point m_LastFrameTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point m_CurrentFrameTime = std::chrono::steady_clock::now();

#ifdef DEBUG
public:
	u32 fps;
private:
	std::chrono::steady_clock::time_point m_LastFrameUpdate = std::chrono::steady_clock::now();
	u32 m_LFps;
#endif

#ifdef VKBUILD
public:
	SwapChain swapchain;
	VkViewport viewport;
	VkRect2D scissor;
	VkClearValue clear_colour;  // TODO make this private, this should not be relevant outside frame

	// image buffers
	vector<VkImage> images;
	vector<VkImageView> image_views;  // TODO outsource this part into image buffers later!
	vector<VkFramebuffer> framebuffers;
	vector<VkSemaphore> render_done;  // TODO this all belongs together i think
	u32 frame_id = 0;

	VkRenderPass ref_render_pass;  // §placeholder
	VkPipeline ref_pipeline;  // $placeholder

private:
	Hardware m_Hardware;
	VkInstance m_Instance;
	VkSurfaceKHR m_Surface;
	VkPresentInfoKHR m_PresentInfo = {  };
#ifdef DEBUG
	VkDebugUtilsMessengerEXT debug_messenger;
#endif

#else
	SDL_GLContext m_Context;
#endif
};

inline Frame g_Frame = Frame("C. Hanson's Clockwork",FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,FRAME_BLITTER_VSYNC);


#endif
