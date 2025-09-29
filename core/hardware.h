#ifndef CORE_HARDWARE_HEADER
#define CORE_HARDWARE_HEADER

#include "base.h"


#ifdef VKBUILD

enum GPUFeatureSupport : u64
{
	GPU_FEATURE_SUPPORT_NONE = 0x0000000000000000;
	GPU_FEATURE_SUPPORT_BASIC = 0x0000000000000001;
	// TODO add features to available bits and combine through bitwise or
};

struct SwapChainInfo
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> modes;
};

struct GPUDevice
{
	// utility
	void select();

	// data
	VkPhysicalDevice gpu;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	vector<VkExtensionProperties> extensions;
	SwapChainInfo swapchain_info;
	set<u32> queues;
	s64 graphical_queue = -1;
	s64 presentation_queue = -1;
	VkPhysicalDeviceMemoryProperties memory_properties;
	u64 supported = GPU_FEATURE_SUPPORT_NONE;
};

struct Hardware
{
	void detect();
	vector<GPUDevice> gpus;
};

struct GPU
{
	// utility
	// TODO
	void stop();

	// data
	GPUDevice* device_info;
	VkDevice gpu;
	VkQueue graphical_queue;
	VkQueue presentation_queue;
};

inline GPU g_GPU;

#endif


#endif
