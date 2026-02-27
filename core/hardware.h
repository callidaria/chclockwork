#ifndef CORE_HARDWARE_HEADER
#define CORE_HARDWARE_HEADER

#include "base.h"


#ifdef VKBUILD

enum GPUFeatureSupport : u64
{
	GPU_FEATURE_SUPPORT_NONE = 0,
	GPU_FEATURE_SUPPORT_BASIC = 1,
	GPU_FEATURE_SUPPORT_ANISOTROPY = 1<<1,
};
// TODO add features to available bits and combine through bitwise or

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
// FIXME never used!

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
	s64 transfer_queue = -1;
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
	// formats
	VkFormat choose_texture_format(const vector<VkFormat>& fs,VkImageTiling tile,VkFormatFeatureFlags feat);

	// command buffers
	void setup_command_buffers();
	CommandBuffer* aquire_graphical_command_buffer();
	CommandBuffer* aquire_transfer_command_buffer();
	static VkCommandBuffer start_command_buffer();
	static void execute_command_buffer(VkCommandBuffer cmd);

	// resources
	void free(VkBuffer res);
	void free(VkImage res);
	void free(VkSampler res);
	void free(VkDeviceMemory res);
	void free(VkSwapchainKHR res);
	void free(VkShaderModule res);
	void free(VkPipeline res);
	void free(VkPipelineLayout res);
	void free(VkDescriptorPool res);
	void free(VkDescriptorSetLayout res);
	void free(VkRenderPass res);
	void free(VkImageView res);
	void free(VkFramebuffer res);
	void free_graphical(VkCommandBuffer* res);
	void free_transfer(VkCommandBuffer* res);
	void free(VkSemaphore res);
	void free(VkFence res);

	// state
	void expect_idle();
	void stop();

	// data
	// device
	GPUDevice* device_info;
	VkDevice gpu;
	VkQueue transfer_queue;
	VkQueue graphical_queue;
	VkQueue presentation_queue;

	// gpu commands
	VkCommandPool cmd_pool_gfx,cmd_pool_trf;
	CommandBuffer cmd_buffers_gfx[GPU_BUFFER_COUNT],cmd_buffers_trf[GPU_TRANSFER_COUNT];
	u8 active_buffer_gfx = 0,active_buffer_trf = 0;
#endif
};

inline GPU g_GPU;


#endif
