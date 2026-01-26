#include "hardware.h"


#ifdef VKBUILD

/**
 *	select detected gpu
 */
void GPUDevice::select()
{
	COMM_ERR_COND(!supported,"selected gpu %s is not supported",properties.deviceName)
	COMM_LOG_FALLBACK("selecting gpu %s",properties.deviceName);
	g_GPU.device_info = this;

	// queue creation
	f32 __QueuePriority = 1.f;
	vector<VkDeviceQueueCreateInfo> __QueueInfos;
	__QueueInfos.reserve(queues.size());
	for (u32 __QueueID : queues)
	{
		__QueueInfos.push_back({  });
		__QueueInfos.back().sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		__QueueInfos.back().queueFamilyIndex = __QueueID;
		__QueueInfos.back().queueCount = 1;
		__QueueInfos.back().pQueuePriorities = &__QueuePriority;
	}

	// device features
	VkPhysicalDeviceFeatures __DeviceFeatures = {  };  // TODO

	// device creation specifics
	VkDeviceCreateInfo __DeviceInfo = {  };
	__DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	__DeviceInfo.queueCreateInfoCount = (u32)__QueueInfos.size();
	__DeviceInfo.pQueueCreateInfos = &__QueueInfos[0];
	__DeviceInfo.enabledExtensionCount = (u32)g_GPUExtensions.size();
	__DeviceInfo.ppEnabledExtensionNames = &g_GPUExtensions[0];
	__DeviceInfo.pEnabledFeatures = &__DeviceFeatures;

	// enable validation layers here as well for safety, even though it's deprecated
#ifdef DEBUG
	__DeviceInfo.enabledLayerCount = (u32)g_ValidationLayers.size();
	__DeviceInfo.ppEnabledLayerNames = &g_ValidationLayers[0];
#else
	__DeviceInfo.enabledLayerCount = 0;
#endif

	// create device
	VkResult __Result = vkCreateDevice(gpu,&__DeviceInfo,nullptr,&g_GPU.gpu);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create logical interface for gpu %s",properties.deviceName);

	// initialize queues
	vkGetDeviceQueue(g_GPU.gpu,graphical_queue,0,&g_GPU.graphical_queue);
	vkGetDeviceQueue(g_GPU.gpu,presentation_queue,0,&g_GPU.presentation_queue);
	// TODO pack this setup into gpu maybe?

	// command buffers
	g_GPU.setup_command_buffers();
}

/**
 *	hardware detection routine
 *	TODO
 */
void Hardware::detect(VkInstance instance,VkSurfaceKHR surface)
{
	COMM_LOG("detecting available GPUs");
	u32 __GPUCount;
	vkEnumeratePhysicalDevices(instance,&__GPUCount,nullptr);
	COMM_ERR_COND(!__GPUCount,"no vulkan capable gpus found. use opengl version!")
	COMM_SCC_FALLBACK("found %u vulkan capable graphics card%s",__GPUCount,(__GPUCount>1)?"s":"");
	vector<VkPhysicalDevice> __PhysicalGPUs = vector<VkPhysicalDevice>(__GPUCount);
	gpus.resize(__GPUCount);
	vkEnumeratePhysicalDevices(instance,&__GPUCount,&__PhysicalGPUs[0]);
	// TODO the fallback macro should be using the pluralization macro once it's done

	// scanning available gpus for specifics
	gpus.resize(__GPUCount);
	for (u8 i=0;i<__GPUCount;i++)
	{
		gpus[i].gpu = __PhysicalGPUs[i];

		// get available queue families
		u32 __QueueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(gpus[i].gpu,&__QueueCount,nullptr);
		vector<VkQueueFamilyProperties> __Queues(__QueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpus[i].gpu,&__QueueCount,&__Queues[0]);

		// iterate queue families & extract ids
		for (u32 j=0;j<__QueueCount;j++)
		{
			// check for graphical support
			if (__Queues[j].queueFlags&VK_QUEUE_GRAPHICS_BIT) gpus[i].graphical_queue = j;

			// check for presentation support
			VkBool32 __PresentingQueue = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i].gpu,j,surface,&__PresentingQueue);
			if (__PresentingQueue) gpus[i].presentation_queue = j;

			// check for sufficient queue support & abort to align graphical queue with presenting queue
			if (gpus[i].graphical_queue!=-1&&gpus[i].presentation_queue!=-1)
			{
				gpus[i].queues = { (u32)gpus[i].graphical_queue,(u32)gpus[i].presentation_queue };
				gpus[i].supported = GPU_FEATURE_SUPPORT_BASIC;
				break;
			}
			// TODO iterate fully & check for aligning ids, to avoid queue split in edge-cases
		}

		// interrupt gpu read should queue support not be sufficient
		if (!gpus[i].supported)
		{
			COMM_ERR("interrupting GPU read at index %i due to insufficient queue support",i);
			continue;
		}

		// get available extensions
		u32 __ExtensionCount,__FormatCount,__ModeCount;
		vkEnumerateDeviceExtensionProperties(gpus[i].gpu,nullptr,&__ExtensionCount,nullptr);
		gpus[i].extensions.resize(__ExtensionCount);
		vkEnumerateDeviceExtensionProperties(gpus[i].gpu,nullptr,&__ExtensionCount,&gpus[i].extensions[0]);

		// checking extension support
		set<string> __RequiredExtensions = set<string>(g_GPUExtensions.begin(),g_GPUExtensions.end());
		for (VkExtensionProperties& __Extension : gpus[i].extensions)
			__RequiredExtensions.erase(__Extension.extensionName);
		gpus[i].supported = __RequiredExtensions.empty()*GPU_FEATURE_SUPPORT_BASIC;
		if (!gpus[i].supported)
		{
			COMM_ERR("interrupting GPU read at index %i, the device is missing crucial extensions",i);
			continue;
		}

		// get device specifics
		vkGetPhysicalDeviceProperties(gpus[i].gpu,&gpus[i].properties);
		vkGetPhysicalDeviceFeatures(gpus[i].gpu,&gpus[i].features);
		COMM_SCC("found supported GPU %s",gpus[i].properties.deviceName);
		// TODO later, read the capabilities of the selected device, allow to change it and change features

		// get swap chain format capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpus[i].gpu,surface,&gpus[i].swapchain_info.capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[i].gpu,surface,&__FormatCount,nullptr);
		if (!!__FormatCount)
		{
			gpus[i].swapchain_info.formats.resize(__FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[i].gpu,surface,
												 &__FormatCount,&gpus[i].swapchain_info.formats[0]);
		}
		COMM_ERR_FALLBACK("no surface formats found for GPU %s",gpus[i].properties.deviceName);

		// get swap chain mode capabilities
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[i].gpu,surface,&__ModeCount,nullptr);
		if (!!__ModeCount)
		{
			gpus[i].swapchain_info.modes.resize(__ModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[i].gpu,surface,
													  &__ModeCount,&gpus[i].swapchain_info.modes[0]);
		}
		COMM_ERR_FALLBACK("no presentation modes found for GPU %s",gpus[i].properties.deviceName);

		// get memory types
		vkGetPhysicalDeviceMemoryProperties(gpus[i].gpu,&gpus[i].memory_properties);
	}
}

#endif


// ----------------------------------------------------------------------------------------------------
// GPU Feature Implementation

/**
 *	TODO
 */
void GPU::cull_backfaces(bool backfaces)
{
#ifdef VKBUILD
	// TODO

#else
	glCullFace(GL_FRONT+backfaces);
#endif
}

#ifndef VKBUILD
GLenum _gpu_features[GPU_FEATURE_COUNT] = {
	GL_DEPTH_TEST,
};
#endif

/**
 *	TODO
 */
void GPU::enable_feature(GPUFeature feature)
{
#ifdef VKBUILD
	// TODO
#else
	glEnable(_gpu_features[feature]);
#endif
}

/**
 *	TODO
 */
void GPU::disable_feature(GPUFeature feature)
{
#ifdef VKBUILD
	// TODO
#else
	glDisable(_gpu_features[feature]);
#endif
}


#ifdef VKBUILD

/**
 *	TODO
 */
void GPU::setup_command_buffers()
{
	// setup command pool
	VkCommandPoolCreateInfo __CMDPoolInfo = {  };
	__CMDPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	__CMDPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	__CMDPoolInfo.queueFamilyIndex = device_info->graphical_queue;
	VkResult __Result = vkCreateCommandPool(gpu,&__CMDPoolInfo,nullptr,&cmd_pool);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create vulkan command pool");

	// setup command buffer
	VkCommandBuffer __CommandBuffers[GPU_BUFFER_COUNT];
	VkCommandBufferAllocateInfo __CMDBufferInfo = {  };
	__CMDBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	__CMDBufferInfo.commandPool = cmd_pool;
	__CMDBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	__CMDBufferInfo.commandBufferCount = GPU_BUFFER_COUNT;
	__Result = vkAllocateCommandBuffers(gpu,&__CMDBufferInfo,__CommandBuffers);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate vulkan command buffer");
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++) cmd_buffers[i].buffer = __CommandBuffers[i];
	// TODO pre-store certain usual commands as secondary... yeah some research in the future about this one

	// setup buffer threading constraints info
	VkSemaphoreCreateInfo __SemaphoreInfo = {  };
	__SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo __FenceInfo = {  };
	__FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	__FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// iterate buffer semaphore creation
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		// create command buffer semaphore
		__Result = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&cmd_buffers[i].ready);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup buffer semaphore %u",i);

		// create command buffer fence
		__Result = vkCreateFence(gpu,&__FenceInfo,nullptr,&cmd_buffers[i].processing);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup host fence");
	}
}

/**
 *	TODO
 */
CommandBuffer* GPU::aquire_command_buffer()
{
	// tick command buffer
	CommandBuffer* out = &cmd_buffers[active_buffer];
	active_buffer = (active_buffer+1)%GPU_BUFFER_COUNT;

	// wait until draw is ready
	vkWaitForFences(gpu,1,&out->processing,VK_TRUE,UINT64_MAX);
	vkResetFences(gpu,1,&out->processing);
	return out;
}

/**
 *	TODO
 */
VkCommandBuffer GPU::start_command_buffer()
{
	// create buffer
	VkCommandBuffer cmdb;
	VkCommandBufferAllocateInfo __CmdBufferAllocInfo = {  };
	__CmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	__CmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	__CmdBufferAllocInfo.commandPool = g_GPU.cmd_pool;
	__CmdBufferAllocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(g_GPU.gpu,&__CmdBufferAllocInfo,&cmdb);

	// start buffer
	VkCommandBufferBeginInfo __CMDBeginInfo = {  };
	__CMDBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	__CMDBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdb,&__CMDBeginInfo);
	return cmdb;
}
// TODO use a CommandBuffer pointer instead so semaphore procedure is also happening when using one-time cmds

/**
 *	TODO
 */
void GPU::execute_command_buffer(VkCommandBuffer cmd)
{
	vkEndCommandBuffer(cmd);
	VkSubmitInfo __SubmitInfo = {  };
	__SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	__SubmitInfo.commandBufferCount = 1;
	__SubmitInfo.pCommandBuffers = &cmd;
	vkQueueSubmit(g_GPU.graphical_queue,1,&__SubmitInfo,VK_NULL_HANDLE);
	vkQueueWaitIdle(g_GPU.graphical_queue);
	g_GPU.free(&cmd);
	// TODO also fence this etc to allow for more parallelism even while vertex buffer upload is happening
}

/**
 *	free given gpu related resources
 *	\param res: resource of any supported type, that will be removed
 */
void GPU::free(VkBuffer res) { vkDestroyBuffer(gpu,res,nullptr); }
void GPU::free(VkDeviceMemory res) { vkFreeMemory(gpu,res,nullptr); }
void GPU::free(VkSwapchainKHR res) { vkDestroySwapchainKHR(gpu,res,nullptr); }
void GPU::free(VkShaderModule res) { vkDestroyShaderModule(gpu,res,nullptr); }
void GPU::free(VkPipeline res) { vkDestroyPipeline(gpu,res,nullptr); }
void GPU::free(VkPipelineLayout res) { vkDestroyPipelineLayout(gpu,res,nullptr); }
void GPU::free(VkDescriptorPool res) { vkDestroyDescriptorPool(gpu,res,nullptr); }
void GPU::free(VkDescriptorSetLayout res) { vkDestroyDescriptorSetLayout(gpu,res,nullptr); }
void GPU::free(VkRenderPass res) { vkDestroyRenderPass(gpu,res,nullptr); }
void GPU::free(VkImageView res) { vkDestroyImageView(gpu,res,nullptr); }
void GPU::free(VkFramebuffer res) { vkDestroyFramebuffer(gpu,res,nullptr); }
void GPU::free(VkCommandBuffer* res) { vkFreeCommandBuffers(gpu,cmd_pool,1,res); }
void GPU::free(VkSemaphore res) { vkDestroySemaphore(gpu,res,nullptr); }
void GPU::free(VkFence res) { vkDestroyFence(gpu,res,nullptr); }

/**
 *	wait until device is idle
 */
void GPU::expect_idle()
{
	vkDeviceWaitIdle(gpu);
}

/**
 *	free gpu from this process
 */
void GPU::stop()
{
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		free(cmd_buffers[i].ready);
		free(cmd_buffers[i].processing);
	}
	vkDestroyCommandPool(gpu,cmd_pool,nullptr);
	vkDestroyDevice(gpu,nullptr);
}

#endif
