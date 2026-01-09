#ifndef CORE_HARDWARE_HEADER
#define CORE_HARDWARE_HEADER

#include "base.h"


#ifdef VKBUILD

enum GPUFeatureSupport : u64
{
	GPU_FEATURE_SUPPORT_NONE = 0x0000000000000000,
	GPU_FEATURE_SUPPORT_BASIC = 0x0000000000000001
	// TODO add features to available bits and combine through bitwise or
};

struct SwapChainInfo
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> modes;
};

struct SwapChain
{
	VkSwapchainKHR swapchain;
	VkExtent2D extent;
	VkSurfaceFormatKHR format;
};

struct CommandBuffer
{
	VkCommandBuffer buffer;
	VkSemaphore ready;
	VkFence processing;
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
	void detect(VkInstance instance,VkSurfaceKHR surface);
	vector<GPUDevice> gpus;
};

#endif


enum GPUFeature : u8
{
	GPU_FEATURE_DEPTH_TEST,
	GPU_FEATURE_COUNT
};


struct GPU
{
	// utility
	// settings
	void cull_backfaces(bool backfaces);
	void enable_feature(GPUFeature feature);
	void disable_feature(GPUFeature feature);

#ifdef VKBUILD
	// command buffers
	void setup_command_buffers();
	CommandBuffer* aquire_command_buffer();

	// resources
	void free(VkBuffer res);
	void free(VkDeviceMemory res);
	void free(VkSwapchainKHR res);
	void free(VkShaderModule res);
	void free(VkPipeline res);
	void free(VkPipelineLayout res);
	void free(VkDescriptorSetLayout res);
	void free(VkRenderPass res);
	void free(VkImageView res);
	void free(VkFramebuffer res);
	void free(VkCommandBuffer* res);
	void free(VkSemaphore res);
	void free(VkFence res);

	// state
	void expect_idle();
	void stop();

	// data
	// device
	GPUDevice* device_info;
	VkDevice gpu;
	VkQueue graphical_queue;
	VkQueue presentation_queue;

	// gpu commands
	VkCommandPool cmd_pool;
	CommandBuffer cmd_buffers[GPU_BUFFER_COUNT];
	u8 active_buffer = 0;
#endif
};

inline GPU g_GPU;


#endif
