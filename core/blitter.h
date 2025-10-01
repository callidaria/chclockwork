#ifndef CORE_BLITTER_HEADER
#define CORE_BLITTER_HEADER


#include "hardware.h"


constexpr vec3 BLITTER_CLEAR_COLOUR = vec3(.0f,.0f,.0f);


enum GPUFeature : u8
{
	GPU_FEATURE_DEPTH_TEST,
	GPU_FEATURE_COUNT
};


#ifdef VKBUILD

struct Eruption
{
	// data
	// vulkan
	SDL_Window* ref_frame;
	VkRenderPass ref_render_pass;  // TODO those refs will be removed once the architecture starts to make sense
	vector<VkImage> images;
	vector<VkImageView> image_views;  // TODO outsource this part into buffer later!
	vector<VkFramebuffer> framebuffers;
	VkCommandPool cmds;

	// command buffer
	vector<CommandBuffer> cmd_buffers;
	vector<VkSemaphore> render_done;

	// viewport

	// state
	VkPipeline pipeline;
	VkClearValue clear_colour;
	u8 active_buffer = 0;
};
#endif


class Frame
{
public:
	Frame(const char* title,u16 width,u16 height,bool vsync=true);

	// utilty
	static void clear();
	void update();
	void close();

	// settings
	static void set_clear_colour(vec3 colour);
	static void set_clear_depth(f32 depth);

	// framerate
	void gpu_vsync_on();
	void gpu_vsync_off();

	// processing
	static void gpu_set_viewport(u32 width,u32 height);
	static void gpu_cull_backfaces(bool backfaces);
	static void gpu_enable_feature(GPUFeature feature);
	static void gpu_disable_feature(GPUFeature feature);

#ifdef VKBUILD
private:
	void _assemble_swapchain();
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
	u32 frame_id = 0;

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


#ifdef VKBUILD
inline Eruption g_Vk;
#endif
inline Frame g_Frame = Frame("C. Hanson's Clockwork",FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,FRAME_BLITTER_VSYNC);


#endif
