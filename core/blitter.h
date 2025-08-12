#ifndef CORE_BLITTER_HEADER
#define CORE_BLITTER_HEADER


#include "base.h"


constexpr vec3 BLITTER_CLEAR_COLOUR = vec3(0);


#ifdef VKBUILD
struct Eruption
{
	// utility
	void erupt(SDL_Window* frame);
	void vanish();

	// data
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDevice gpu;
	VkQueue graphical_queue;
	VkQueue presentation_queue;
	VkSwapchainKHR swapchain;
#ifdef DEBUG
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
};

struct SwapChain
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> modes;
};

struct GPU
{
	// utility
	void select(SDL_Window* frame,Eruption& vke);

	// data
	VkPhysicalDevice gpu;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	vector<VkExtensionProperties> extensions;
	SwapChain swap_chain;
	set<u32> queues;
	s64 graphical_queue = -1;
	s64 presentation_queue = -1;
	u32 supported = 0;  // TODO set bits in order to define which options are avaiable to the user
};

struct Hardware
{
	// utility
	void detect(const Eruption& vke);

	// data
	vector<GPU> gpus;
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
	static inline void set_clear_colour(vec3 colour) { glClearColor(colour.r,colour.g,colour.b,0); }
	static inline void set_clear_depth(f32 depth) { glClearDepth(depth); }

	// framerate
	void gpu_vsync_on();
	void gpu_vsync_off();

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

	// hardware
	Eruption m_VulkanEruption;
	Hardware m_Hardware;

#else
	SDL_GLContext m_Context;
#endif
};

inline Frame g_Frame = Frame("C. Hanson's Clockwork",FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,FRAME_BLITTER_VSYNC);


#endif
