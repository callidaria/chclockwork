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
	__DeviceFeatures.samplerAnisotropy = !!(supported&GPU_FEATURE_SUPPORT_ANISOTROPY);

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
	vkGetDeviceQueue(g_GPU.gpu,transfer_queue,0,&g_GPU.transfer_queue);
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

		// iterate queue families & extract ids based on utility
		vector<u32> __TransferQueues,__GraphicalQueues,__PresentationQueues;
		__TransferQueues.reserve(__QueueCount);
		__GraphicalQueues.reserve(__QueueCount);
		__PresentationQueues.reserve(__QueueCount);
		for (u32 j=0;j<__QueueCount;j++)
		{
			// check for presentation support
			VkBool32 __PresentingQueue = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i].gpu,j,surface,&__PresentingQueue);
			if (__PresentingQueue) __PresentationQueues.push_back(j);
			if (__Queues[j].queueFlags&VK_QUEUE_GRAPHICS_BIT) __GraphicalQueues.push_back(j);
			if (__Queues[j].queueFlags&VK_QUEUE_TRANSFER_BIT) __TransferQueues.push_back(j);
		}

		// assign appropriate queues to their respective tasks
		// this procedure should try to SPLIT graphics and transfer queue if supported by hardware, but
		// conversely it should also try to use a SINGLE queue for graphics and presentation to avoid overhead
		vector<u32> __JoinedGraphicalQueues,__UniqueTransferQueues;
		__JoinedGraphicalQueues.resize(__QueueCount);
		__UniqueTransferQueues.resize(__QueueCount);
		std::set_intersection(__GraphicalQueues.begin(),__GraphicalQueues.end(),
							  __PresentationQueues.begin(),__PresentationQueues.end(),
							  __JoinedGraphicalQueues.begin());
		std::set_difference(__TransferQueues.begin(),__TransferQueues.end(),
							__JoinedGraphicalQueues.begin(),__JoinedGraphicalQueues.end(),
							__UniqueTransferQueues.begin());

		// select graphical and presentation queue
		if (__JoinedGraphicalQueues.empty())
		{
			COMM_MSG(LOG_BLUE,"[INFO] failed to assign presentation and graphical queue to the same id");
			gpus[i].presentation_queue = (__PresentationQueues.empty())?__PresentationQueues[0]:-1;
			gpus[i].graphical_queue = (__GraphicalQueues.empty())?__GraphicalQueues[0]:-1;
		}
		for (u32 qid : __JoinedGraphicalQueues)
		{
			bool __PreferredWithoutTransfer = !(__Queues[qid].queueFlags&VK_QUEUE_TRANSFER_BIT);
			if (gpus[i].graphical_queue!=-1&&!__PreferredWithoutTransfer) continue;
			gpus[i].presentation_queue = qid;
			gpus[i].graphical_queue = qid;
			if (__PreferredWithoutTransfer) break;
		}

		// attempt to select a split transfer queue
		if (!__UniqueTransferQueues.empty()) gpus[i].transfer_queue = __UniqueTransferQueues[0];
		for (u32 qid : __TransferQueues)
		{
			bool __PreferredSplit = (qid!=gpus[i].presentation_queue)&&(qid!=gpus[i].graphical_queue);
			if (gpus[i].transfer_queue!=-1&&!__PreferredSplit) continue;
			gpus[i].transfer_queue = qid;
			if (__PreferredSplit) break;
		}
		// TODO mark gpus that fulfill the preferred assignment as preferred by the engine,
		//		this will be extremely useful to guide automatic device selection later

		// check for sufficient queue support
		if (gpus[i].transfer_queue!=-1&&gpus[i].graphical_queue!=-1&&gpus[i].presentation_queue!=-1)
		{
			gpus[i].queues = {
				(u32)gpus[i].transfer_queue,
				(u32)gpus[i].graphical_queue,
				(u32)gpus[i].presentation_queue,
			};
			gpus[i].supported = GPU_FEATURE_SUPPORT_BASIC;
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

		// get device specifics
		vkGetPhysicalDeviceProperties(gpus[i].gpu,&gpus[i].properties);
		vkGetPhysicalDeviceFeatures(gpus[i].gpu,&gpus[i].features);
		COMM_SCC("found supported GPU %s",gpus[i].properties.deviceName);
		COMM_LOG("(presentation: %ld, graphics: %ld, transfer: %ld)",
				 gpus[i].presentation_queue,gpus[i].graphical_queue,gpus[i].transfer_queue);
		// TODO later, read the capabilities of the selected device, allow to change it and change features

		// checking extension support
		set<string> __RequiredExtensions = set<string>(g_GPUExtensions.begin(),g_GPUExtensions.end());
		for (VkExtensionProperties& __Extension : gpus[i].extensions)
			__RequiredExtensions.erase(__Extension.extensionName);
		gpus[i].supported = (__RequiredExtensions.empty()*GPU_FEATURE_SUPPORT_BASIC)
				| (gpus[i].features.samplerAnisotropy*GPU_FEATURE_SUPPORT_ANISOTROPY);
		if (!(gpus[i].supported&GPU_FEATURE_SUPPORT_BASIC))
		{
			COMM_ERR("interrupting GPU read at index %i, the device is missing crucial extensions",i);
			continue;
		}

		// some info about gpu features
		COMM_MSG_COND(!(gpus[i].supported&GPU_FEATURE_SUPPORT_ANISOTROPY),
					  LOG_YELLOW,"Warning: anisotropy not supported by this gpu");

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
VkFormat GPU::choose_texture_format(const vector<VkFormat>& fs,VkImageTiling tile,VkFormatFeatureFlags feat)
{
	for (VkFormat __Format : fs)
	{
		VkFormatProperties __Properties;
		vkGetPhysicalDeviceFormatProperties(device_info->gpu,__Format,&__Properties);
		if (tile==VK_IMAGE_TILING_LINEAR&&(__Properties.linearTilingFeatures&feat)==feat) return __Format;
		else if (tile==VK_IMAGE_TILING_OPTIMAL&&(__Properties.optimalTilingFeatures&feat)==feat) return __Format;
	}
	COMM_ERR("no format in list supports desired features");
	return {};
}

/**
 *	TODO
 */
void GPU::setup_command_buffers()
{
	// setup graphical command pool
	VkCommandPoolCreateInfo __CMDPoolInfo = {  };
	__CMDPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	__CMDPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	__CMDPoolInfo.queueFamilyIndex = device_info->graphical_queue;
	VkResult __Result = vkCreateCommandPool(gpu,&__CMDPoolInfo,nullptr,&cmd_pool_gfx);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create graphical command pool");

	// setup transfer command pool
	__CMDPoolInfo.queueFamilyIndex = device_info->transfer_queue;
	__Result = vkCreateCommandPool(gpu,&__CMDPoolInfo,nullptr,&cmd_pool_trf);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create transfer command pool");
	// TODO consider only making a second pool if graphical_queue != transfer_queue

	// setup graphical command buffers
	VkCommandBuffer __CommandBuffers[GPU_BUFFER_COUNT];
	VkCommandBufferAllocateInfo __CMDBufferInfo = {  };
	__CMDBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	__CMDBufferInfo.commandPool = cmd_pool_gfx;
	__CMDBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	__CMDBufferInfo.commandBufferCount = GPU_BUFFER_COUNT;
	__Result = vkAllocateCommandBuffers(gpu,&__CMDBufferInfo,__CommandBuffers);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate graphical command buffers");
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++) cmd_buffers_gfx[i].buffer = __CommandBuffers[i];
	// TODO pre-store certain usual commands as secondary... yeah some research in the future about this one

	// setup transfer command buffers
	VkCommandBuffer __TransferBuffers[GPU_BUFFER_COUNT];
	__CMDBufferInfo.commandPool = cmd_pool_trf;
	__CMDBufferInfo.commandBufferCount = GPU_BUFFER_COUNT;
	__Result = vkAllocateCommandBuffers(gpu,&__CMDBufferInfo,__TransferBuffers);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate transfer command buffers");
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++) cmd_buffers_trf[i].buffer = __TransferBuffers[i];

	// setup buffer threading constraints info
	VkSemaphoreCreateInfo __SemaphoreInfo = {  };
	__SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo __FenceInfo = {  };
	__FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	__FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// iterate buffer semaphore creations for graphics
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		// create command buffer semaphore
		__Result = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&cmd_buffers_gfx[i].ready);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup buffer semaphore %u for graphics",i);
		__Result = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&cmd_buffers_gfx[i].transferred);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup buffer semaphore %u for transfer",i);

		// create command buffer fence
		__Result = vkCreateFence(gpu,&__FenceInfo,nullptr,&cmd_buffers_gfx[i].processing);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup host fence %u for graphics",i);
		__Result = vkCreateFence(gpu,&__FenceInfo,nullptr,&cmd_buffers_trf[i].processing);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup host fence %u for transfer",i);
	}
}
// FIXME a lot of code repetition

/**
 *	TODO
 */
CommandBufferGFX* GPU::aquire_graphical_command_buffer()
{
	return &cmd_buffers_gfx[active_buffer];
}

/**
 *	TODO
 */
CommandBufferTRF* GPU::aquire_transfer_command_buffer()
{
	// tick command buffer
	return &cmd_buffers_trf[active_buffer];
}
// FIXME a lot of code repetition again and again

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
	__CmdBufferAllocInfo.commandPool = g_GPU.cmd_pool_gfx;
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
	g_GPU.free_graphical(&cmd);
	// TODO also fence this etc to allow for more parallelism even while vertex buffer upload is happening
}
// TODO remove this feature entirely. excluding the rest of the upload/draw processing, shall be prohibited

/**
 *	TODO
 */
static inline void _swap_buffer(VkDevice& gpu,VkCommandBuffer& buffer,VkFence* processing)
{
	// host synchronization
	vkWaitForFences(gpu,1,processing,VK_TRUE,UINT64_MAX);
	vkResetFences(gpu,1,processing);
	vkResetCommandBuffer(buffer,0);

	// begin command buffer
	VkCommandBufferBeginInfo __CMDInfo = {  };
	__CMDInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	__CMDInfo.flags = 0;
	__CMDInfo.pInheritanceInfo = nullptr;
	VkResult __Result = vkBeginCommandBuffer(buffer,&__CMDInfo);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"issue while starting a command buffer");
	// TODO the creation info can be pre-cached instead and then just used based on registration type later?
}
// TODO so sort graphical submissions in two different categories and maybe submit twice?
//		that would allow static geometry draw while transfer for dynamic objects still takes place
//		in addition to transfer taking place, while graphics is working on the previous frame...
//		then instanced draws (which is most of it) takes over, using a second submit
//		unfortunately heavy shading will probably have to wait for all geometry to finish, but that is ok due to
//		the fact that transfer for the next frame happens during those operations.
//		(1f buffer stall ipl should not be a problem, when this engine targets a very high frequency anyways)
// TODO a mode to reduce gpu parallelism for buffer upload while frame processing in exchange for zero
//		input lag rates. but the user is not recommended to actually use it!! for danmaku nerds only!!

/**
 *	TODO
 */
void GPU::swap()
{
	_swap_buffer(gpu,cmd_buffers_trf[active_buffer].buffer,&cmd_buffers_trf[active_buffer].processing);
	_swap_buffer(gpu,cmd_buffers_gfx[active_buffer].buffer,&cmd_buffers_gfx[active_buffer].processing);
}

/**
 *	TODO
 */
void GPU::update(VkSemaphore* blit_ready)
{
	// end recording
	VkResult __Result = vkEndCommandBuffer(cmd_buffers_trf[active_buffer].buffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to successfully finalize transfer command buffer");
	__Result = vkEndCommandBuffer(cmd_buffers_gfx[active_buffer].buffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to successfully finalize graphical command buffer");

	// submit to transfer queue
	VkPipelineStageFlags __TransferStageFlags[] = { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT };
	VkSubmitInfo __SubmitInfo = {  };
	__SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	__SubmitInfo.waitSemaphoreCount = 0;
	__SubmitInfo.pWaitDstStageMask = __TransferStageFlags;
	__SubmitInfo.commandBufferCount = 1;
	__SubmitInfo.pCommandBuffers = &cmd_buffers_trf[active_buffer].buffer;
	__SubmitInfo.signalSemaphoreCount = 1;
	__SubmitInfo.pSignalSemaphores = &cmd_buffers_gfx[active_buffer].transferred;
	__Result = vkQueueSubmit(transfer_queue,1,&__SubmitInfo,cmd_buffers_trf[active_buffer].processing);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to submit command buffer");

	// submit to graphical queue
	VkPipelineStageFlags __GraphicalStageFlags[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
	};
	__SubmitInfo.waitSemaphoreCount = 2;
	__SubmitInfo.pWaitSemaphores = &cmd_buffers_gfx[active_buffer].ready;
	__SubmitInfo.pWaitDstStageMask = __GraphicalStageFlags;
	__SubmitInfo.pCommandBuffers = &cmd_buffers_gfx[active_buffer].buffer;
	__SubmitInfo.signalSemaphoreCount = 1;
	__SubmitInfo.pSignalSemaphores = blit_ready;
	__Result = vkQueueSubmit(graphical_queue,1,&__SubmitInfo,cmd_buffers_gfx[active_buffer].processing);

	// swap buffer memory index
	active_buffer = (active_buffer+1)%GPU_BUFFER_COUNT;
}

/**
 *	free given gpu related resources
 *	\param res: resource of any supported type, that will be removed
 */
void GPU::free(VkBuffer res) { vkDestroyBuffer(gpu,res,nullptr); }
void GPU::free(VkImage res) { vkDestroyImage(gpu,res,nullptr); }
void GPU::free(VkSampler res) { vkDestroySampler(gpu,res,nullptr); }
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
void GPU::free_graphical(VkCommandBuffer* res) { vkFreeCommandBuffers(gpu,cmd_pool_gfx,1,res); }
void GPU::free_transfer(VkCommandBuffer* res) { vkFreeCommandBuffers(gpu,cmd_pool_trf,1,res); }
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
		free(cmd_buffers_gfx[i].ready);
		free(cmd_buffers_gfx[i].transferred);
		free(cmd_buffers_gfx[i].processing);
		free(cmd_buffers_trf[i].processing);
	}
	vkDestroyCommandPool(gpu,cmd_pool_gfx,nullptr);
	vkDestroyCommandPool(gpu,cmd_pool_trf,nullptr);
	vkDestroyDevice(gpu,nullptr);
}

#endif
