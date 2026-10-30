#ifndef CORE_BLITTER_HEADER
#define CORE_BLITTER_HEADER


#include "hardware.h"


constexpr vec3 BLITTER_CLEAR_COLOUR = vec3(.0f);


#ifdef VKBUILD
struct FrameDimensions
{
	// utility
	void initialize(f32 width,f32 height);

	// data
	VkExtent2D extent;
	VkViewport viewport;
	VkRect2D scissor;
};

struct SwapChain
{
	VkSwapchainKHR swapchain;
	VkExtent2D extent;
	VkSurfaceFormatKHR format;
};
#endif


class Frame
{
public:
	Frame();

	// utilty
	static void clear();
	void update();
	void close();

	// settings
	void set_clear_colour(vec3 colour);
	void set_clear_depth(f32 depth);
	void set_viewport(u32 width,u32 height);
	void gpu_vsync(bool vsync);

#ifdef VKBUILD
	void rebuild_swapchain();
#endif

private:
#ifdef VKBUILD
	void _assemble_swapchain();
	void _destroy_swapchain();
#endif

public:
	// time & fps
	std::chrono::steady_clock::time_point fstart = std::chrono::steady_clock::now();
	f32 delta_time_real = .0f;
	f32 delta_time = .0f;
	f32 time_factor = 1.f;
	u32 fps;

#ifdef VKBUILD
	// swapchain
	SwapChain swapchain;
	VkClearValue clear_colour[2];
	size_t fid = 0;

	// image buffers
	vector<VkImage> result_images;
	vector<VkImageView> result_image_views;
	vector<VkSemaphore> render_done;
	u32 frame_id = 0;
#endif

private:
	// hardware
	SDL_Window* m_Frame;

	// time & fps
	std::chrono::steady_clock::time_point m_LastFrameTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point m_LastFrameUpdate = std::chrono::steady_clock::now();
	u32 m_LFps;

#ifdef VKBUILD
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

inline Frame g_Frame = Frame();


#endif
