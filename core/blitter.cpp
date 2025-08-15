#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// GPU Error Callbacks

#ifdef DEBUG
#ifdef VKBUILD
VKAPI_ATTR VkBool32 VKAPI_CALL _gpu_error_callback(VkDebugUtilsMessageSeverityFlagBitsEXT sev,
												   VkDebugUtilsMessageTypeFlagsEXT type,
												   const VkDebugUtilsMessengerCallbackDataEXT* cb,
												   void* udata)
{
	// error type recognition
	string __ErrType = "";
	__ErrType += (type&VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) ? "!General " : "";
	__ErrType += (type&VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) ? "!Specifics " : "";
	__ErrType += (type&VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "!Performance " : "";
	//__ErrType += (type&VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) ? "!Memory " : "";
	// TODO enable VK_EXT_device_address_binding_report

	// error logging
	if (sev&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		COMM_ERR("[GPU] %s%s%s%s",LOG_BLUE,__ErrType.c_str(),LOG_RED,cb->pMessage)
	else if (sev&VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		COMM_MSG(LOG_YELLOW,"[GPU Warning] %s%s%s%s",LOG_BLUE,__ErrType.c_str(),LOG_YELLOW,cb->pMessage);
	return VK_FALSE;
}

#else

void GLAPIENTRY _gpu_error_callback(GLenum src,GLenum type,GLenum id,GLenum sev,GLsizei len,
									const GLchar* msg,const void* usrParam)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: COMM_ERR("[GPU] %s",msg);
		break;
	case GL_DEBUG_TYPE_PERFORMANCE: COMM_MSG(LOG_RED,"[GPU Performance Warning] %s",msg);
		break;
	};
}

#endif
#endif


// ----------------------------------------------------------------------------------------------------
// Hardware Interaction

#ifdef VKBUILD

/**
 *	TODO
 */
void Eruption::erupt(SDL_Window* frame)
{
	// application info
	VkApplicationInfo __ApplicationInfo = {  };
	__ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	__ApplicationInfo.pApplicationName = FRAME_GAME_NAME;
	__ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
	__ApplicationInfo.pEngineName = "C. Hanson's Clockwork";
	__ApplicationInfo.engineVersion = VK_MAKE_VERSION(0,0,1);
	__ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	// extensions
	u32 __ExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(frame,&__ExtensionCount,nullptr);
	vector<const char*> __Extensions(__ExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(frame,&__ExtensionCount,&__Extensions[0]);
#ifdef DEBUG
	VkDebugUtilsMessengerCreateInfoEXT __DebugMessengerInfo = {  };
	__DebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	__DebugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	__DebugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			//|VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
	__DebugMessengerInfo.pfnUserCallback = _gpu_error_callback;
	__Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	COMM_LOG("creating vulkan instance");
	VkInstanceCreateInfo __CreateInfo = {  };
	__CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	__CreateInfo.pApplicationInfo = &__ApplicationInfo;
	__CreateInfo.enabledLayerCount = 0;
	__CreateInfo.enabledExtensionCount = (u32)__Extensions.size();
	__CreateInfo.ppEnabledExtensionNames = &__Extensions[0];

	// setup validation layers for gpu auto-logging
#ifdef DEBUG
	__CreateInfo.enabledLayerCount = (u32)g_ValidationLayers.size();
	__CreateInfo.ppEnabledLayerNames = &g_ValidationLayers[0];
	__CreateInfo.pNext = &__DebugMessengerInfo;
#endif

	// creating vulkan instance
	VkResult __Result = vkCreateInstance(&__CreateInfo,nullptr,&instance);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create vulkan instance");

#ifdef DEBUG
	COMM_LOG("setting up gpu error log");
	PFN_vkCreateDebugUtilsMessengerEXT __CreateMessenger
		= (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT");
	__Result = __CreateMessenger(instance,&__DebugMessengerInfo,nullptr,&debug_messenger);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to set up gpu error logging");
#endif

	COMM_LOG("setting up render surface");
	bool __SurfaceResult = SDL_Vulkan_CreateSurface(frame,instance,&surface);
	COMM_ERR_COND(!__SurfaceResult,"failed to initialize render surface");
}

/**
 *	TODO
 */
void Eruption::register_pipeline(VkRenderPass render_pass)
{
	COMM_LOG("registration of final destination pipeline");

	// basic setup for all final framebuffers
	VkFramebufferCreateInfo __FramebufferInfo = {  };
	__FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	__FramebufferInfo.renderPass = render_pass;
	__FramebufferInfo.attachmentCount = 1;
	__FramebufferInfo.width = sc_extent.width;
	__FramebufferInfo.height = sc_extent.height;
	__FramebufferInfo.layers = 1;

	// allocate & iterate framebuffer creation
	VkResult __Result;
	framebuffers.resize(image_views.size());
	for (u32 i=0;i<image_views.size();i++)
	{
		__FramebufferInfo.pAttachments = &image_views[i];
		__Result = vkCreateFramebuffer(gpu,&__FramebufferInfo,nullptr,&framebuffers[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create framebuffer %u",i);
	}

	// setup command pool
	VkCommandPoolCreateInfo __CMDPoolInfo = {  };
	__CMDPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	__CMDPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	__CMDPoolInfo.queueFamilyIndex = graphical_queue_id;
	__Result = vkCreateCommandPool(gpu,&__CMDPoolInfo,nullptr,&cmds);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create vulkan command pool");

	// setup command buffer
	VkCommandBufferAllocateInfo __CMDBufferInfo = {  };
	__CMDBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	__CMDBufferInfo.commandPool = cmds;
	__CMDBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	__CMDBufferInfo.commandBufferCount = 1;
	__Result = vkAllocateCommandBuffers(gpu,&__CMDBufferInfo,&cmd_buffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate vulkan command buffer");
	// TODO pre-store certain usual commands as secondary... yeah some research in the future about this one

	// gpu async processing locking setup
	VkSemaphoreCreateInfo __SemaphoreInfo = {  };
	VkFenceCreateInfo __FenceInfo = {  };
	__SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	__FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	__FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// semaphore & fence creation
	VkResult __ResultSem0 = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&image_ready);
	VkResult __ResultSem1 = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&render_done);
	VkResult __ResultFence = vkCreateFence(gpu,&__FenceInfo,nullptr,&frame_progress);
	COMM_ERR_COND(__ResultSem0!=VK_SUCCESS||__ResultSem1!=VK_SUCCESS||__ResultFence!=VK_SUCCESS,
				  "failed to setup semaphores & fence");
}

/**
 *	TODO
 */
void Eruption::vanish()
{
	vkDestroySemaphore(gpu,image_ready,nullptr);
	vkDestroySemaphore(gpu,render_done,nullptr);
	vkDestroyFence(gpu,frame_progress,nullptr);
	vkDestroyCommandPool(gpu,cmds,nullptr);
	for (VkFramebuffer p_Framebuffer : framebuffers) vkDestroyFramebuffer(gpu,p_Framebuffer,nullptr);
	for (VkImageView p_ImageView : image_views) vkDestroyImageView(gpu,p_ImageView,nullptr);
	vkDestroySwapchainKHR(gpu,swapchain,nullptr);
	vkDestroyDevice(gpu,nullptr);
	vkDestroySurfaceKHR(instance,surface,nullptr);
#ifdef DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT __DestroyMessenger
		= (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
																	  "vkDestroyDebugUtilsMessengerEXT");
	__DestroyMessenger(instance,debug_messenger,nullptr);
#endif
	vkDestroyInstance(instance,nullptr);
}

/**
 *	TODO
 */
void GPU::select(SDL_Window* frame)
{
	COMM_ERR_COND(!supported,"selected gpu is not supported");
	COMM_LOG("selecting gpu %s",properties.deviceName);

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
	VkResult __Result = vkCreateDevice(gpu,&__DeviceInfo,nullptr,&g_Vk.gpu);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create logical interface for gpu %s",properties.deviceName);

	// cache current active queue ids
	g_Vk.graphical_queue_id = graphical_queue;
	g_Vk.presentation_queue_id = presentation_queue;

	// initialize queues
	vkGetDeviceQueue(g_Vk.gpu,graphical_queue,0,&g_Vk.graphical_queue);
	vkGetDeviceQueue(g_Vk.gpu,presentation_queue,0,&g_Vk.presentation_queue);

	// format selection
	COMM_LOG("running swap chain setup");
	for (VkSurfaceFormatKHR& p_Format : swap_chain.formats)
	{
		if (p_Format.format==VK_FORMAT_B8G8R8A8_SRGB&&p_Format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			g_Vk.sc_format = p_Format;
			goto swap_chain_selection_presentation;
		}
	}
	COMM_MSG(LOG_YELLOW,"WARNING: SRGB8 format not supported, falling back to swap chain standard");
	g_Vk.sc_format = swap_chain.formats[0];

	// presentation mode selection
swap_chain_selection_presentation:
	VkPresentModeKHR __Mode;
	for (VkPresentModeKHR& p_Mode : swap_chain.modes)
	{
		if ((p_Mode==VK_PRESENT_MODE_MAILBOX_KHR&&FRAME_BLITTER_VSYNC)
			||(p_Mode==VK_PRESENT_MODE_IMMEDIATE_KHR&&!FRAME_BLITTER_VSYNC))
		{
			__Mode = p_Mode;
			goto swap_chain_selection_extent;
		}
	}
	COMM_MSG(LOG_YELLOW,"WARNING: desired mode not available, falling back to fifo mode");
	__Mode = VK_PRESENT_MODE_FIFO_KHR;

	// swap extent selection
swap_chain_selection_extent:
	s32 __Width,__Height;
	if (swap_chain.capabilities.currentExtent.width!=UINT32_MAX)
	{
		COMM_MSG(LOG_YELLOW,"WARNING: vulkan refuses the swapchain extent override, using fixed extent instead");
		g_Vk.sc_extent = swap_chain.capabilities.currentExtent;
		goto swap_chain_creation;
	}
	SDL_Vulkan_GetDrawableSize(frame,&__Width,&__Height);
	g_Vk.sc_extent = {
		.width = glm::clamp((u32)__Width,
							swap_chain.capabilities.minImageExtent.width,
							swap_chain.capabilities.maxImageExtent.width),
		.height = glm::clamp((u32)__Height,
							 swap_chain.capabilities.minImageExtent.height,
							 swap_chain.capabilities.maxImageExtent.height),
	};

	// create swapchain
swap_chain_creation:
	u32 __ImageCount = swap_chain.capabilities.minImageCount+FRAME_BLITTER_SWAP_IMAGES;
	__ImageCount = (swap_chain.capabilities.maxImageCount>0&&__ImageCount>swap_chain.capabilities.maxImageCount)
			? swap_chain.capabilities.maxImageCount : __ImageCount;

	// swapchain definition
	VkSwapchainCreateInfoKHR __SwapchainInfo = {  };
	__SwapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	__SwapchainInfo.surface = g_Vk.surface;
	__SwapchainInfo.minImageCount = __ImageCount;
	__SwapchainInfo.imageFormat = g_Vk.sc_format.format;
	__SwapchainInfo.imageColorSpace = g_Vk.sc_format.colorSpace;
	__SwapchainInfo.imageExtent = g_Vk.sc_extent;
	__SwapchainInfo.imageArrayLayers = 1;
	__SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // TODO change to TRANSFER_DST_BIT later
	__SwapchainInfo.preTransform = swap_chain.capabilities.currentTransform;
	__SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // TODO very, very interesting...
	__SwapchainInfo.presentMode = __Mode;
	__SwapchainInfo.clipped = VK_TRUE;
	__SwapchainInfo.oldSwapchain = VK_NULL_HANDLE;  // TODO geez this looks like a ton of work in the future

	// in case of split graphics & presentation queue
	vector<u32> __Queues = vector<u32>(queues.begin(),queues.end());
	if (graphical_queue!=presentation_queue)
	{
		COMM_MSG(LOG_YELLOW,"%s %s","WARNING: graphical & presentation queues are distict,",
				 "concurrent mode could result in performance issues");
		__SwapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		__SwapchainInfo.queueFamilyIndexCount = 2;
		__SwapchainInfo.pQueueFamilyIndices = &__Queues[0];
	}
	else __SwapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// TODO optimize away concurrent mode in this case

	// initialize swapchain
	__Result = vkCreateSwapchainKHR(g_Vk.gpu,&__SwapchainInfo,nullptr,&g_Vk.swapchain);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not initialize swap chain");

	// reference swapchain images
	u32 __SCICount;
	vkGetSwapchainImagesKHR(g_Vk.gpu,g_Vk.swapchain,&__SCICount,nullptr);
	COMM_ERR_COND(!__SCICount,"no swapchain images to reference");
	g_Vk.images.resize(__SCICount);
	vkGetSwapchainImagesKHR(g_Vk.gpu,g_Vk.swapchain,&__SCICount,&g_Vk.images[0]);

	// image view memory & creation info setup
	g_Vk.image_views.resize(__SCICount);
	VkImageViewCreateInfo __IVInfo = {  };
	__IVInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	__IVInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	__IVInfo.format = g_Vk.sc_format.format;
	__IVInfo.components = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY,
	};
	__IVInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__IVInfo.subresourceRange.baseMipLevel = 0;
	__IVInfo.subresourceRange.levelCount = 1;
	__IVInfo.subresourceRange.baseArrayLayer = 0;
	__IVInfo.subresourceRange.layerCount = 1;

	// iterate images to create image views
	for (u32 i=0;i<__SCICount;i++)
	{
		__IVInfo.image = g_Vk.images[i];
		__Result = vkCreateImageView(g_Vk.gpu,&__IVInfo,nullptr,&g_Vk.image_views[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"faled to create image view for swapchain image %i",i);
	}
	// TODO when having an idea of the bigger *picture* outsource this to buffer as texture gen AND rndtarget
}
// TODO make all those features selectable by the user

/**
 *	detect gpus
 *	TODO
 */
void Hardware::detect()
{
	COMM_LOG("detecting available GPUs");
	u32 __GPUCount;
	vkEnumeratePhysicalDevices(g_Vk.instance,&__GPUCount,nullptr);
	COMM_ERR_COND(!__GPUCount,"no vulkan capable gpus found. use opengl version!")
	COMM_SCC_FALLBACK("found %u vulkan capable graphics card%s",__GPUCount,(__GPUCount>1)?"s":"");
	vector<VkPhysicalDevice> __PhysicalGPUs = vector<VkPhysicalDevice>(__GPUCount);
	gpus.resize(__GPUCount);
	vkEnumeratePhysicalDevices(g_Vk.instance,&__GPUCount,&__PhysicalGPUs[0]);

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
			vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i].gpu,j,g_Vk.surface,&__PresentingQueue);
			if (__PresentingQueue) gpus[i].presentation_queue = j;

			// check for sufficient queue support & abort to align graphical queue with presenting queue
			if (gpus[i].graphical_queue!=-1&&gpus[i].presentation_queue!=-1)
			{
				gpus[i].queues = { (u32)gpus[i].graphical_queue,(u32)gpus[i].presentation_queue };
				gpus[i].supported = true;
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
		gpus[i].supported = __RequiredExtensions.empty();
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
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpus[i].gpu,g_Vk.surface,&gpus[i].swap_chain.capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[i].gpu,g_Vk.surface,&__FormatCount,nullptr);
		if (!!__FormatCount)
		{
			gpus[i].swap_chain.formats.resize(__FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[i].gpu,g_Vk.surface,
												 &__FormatCount,&gpus[i].swap_chain.formats[0]);
		}
		COMM_ERR_FALLBACK("no surface formats found for GPU %s",gpus[i].properties.deviceName);

		// get swap chain mode capabilities
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[i].gpu,g_Vk.surface,&__ModeCount,nullptr);
		if (!!__ModeCount)
		{
			gpus[i].swap_chain.modes.resize(__ModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[i].gpu,g_Vk.surface,
													  &__ModeCount,&gpus[i].swap_chain.modes[0]);
		}
		COMM_ERR_FALLBACK("no presentation modes found for GPU %s",gpus[i].properties.deviceName);
	}
}

#endif


// ----------------------------------------------------------------------------------------------------
// Graphical Frame

/**
 *	open a graphical window
 *	\param title: window title displayed in decoration and program listing
 *	\param width: window dimension width
 *	\param height: window dimension height
 *	TODO all those parameters are provided by config and are therefore global. remove this pass!
 */
Frame::Frame(const char* title,u16 width,u16 height,bool vsync)
{
	const char* __BitWidth =
#ifdef __SYSTEM_64BIT
		"64-bit";
#else
		"32-bit";
#endif

	COMM_MSG(LOG_YELLOW,"setup sdl version 3.3. %s",__BitWidth);
	u32 __InitSuccess = SDL_Init(SDL_INIT_VIDEO);
	COMM_ERR_COND(!!__InitSuccess,"sdl initialization failed!");

	// ----------------------------------------------------------------------------------------------------
	// OpenGL Setup
#ifndef VKBUILD
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);

	COMM_MSG(LOG_CYAN,"opening OpenGL window");
	m_Frame = SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
							   width,height,SDL_WINDOW_OPENGL);
	m_Context = SDL_GL_CreateContext(m_Frame);

	COMM_LOG("opengl setup");
	glewInit();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glViewport(0,0,width,height);

	// gpu error log
#if defined(DEBUG) && !defined(__APPLE__)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(_gpu_error_callback,nullptr);
#endif

	// ----------------------------------------------------------------------------------------------------
	// Vulkan Setup
#else
	COMM_MSG(LOG_CYAN,"opening vulkan window");
	u8 did = 0;
	m_Frame = SDL_CreateWindow(FRAME_GAME_NAME,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
							   width,height,SDL_WINDOW_VULKAN);
	g_Vk.erupt(m_Frame);
	m_Hardware.detect();
	m_Hardware.gpus[did].select(m_Frame);
	// FIXME just selecting the first possible gpu without feature checking or evaluating is dangerous!
#endif

	// vsync
	if (vsync) gpu_vsync_on();
	else gpu_vsync_off();

	// standard settings
	//set_clear_colour(BLITTER_CLEAR_COLOUR);

	COMM_SCC("blitter ready.");
}

/**
 *	clear framebuffer, should be done ideally before drawing to the framebuffer
 */
void Frame::clear()
{
#ifdef VKBUILD
	// TODO
#else
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
}

/**
 *	flip buffer
 */
void Frame::update()
{
#ifdef VKBUILD
	// TODO
#else
	SDL_GL_SwapWindow(m_Frame);
#endif

	// calculate delta time
	m_LastFrameTime = m_CurrentFrameTime;
	m_CurrentFrameTime = std::chrono::steady_clock::now();
	delta_time_real = (m_CurrentFrameTime-m_LastFrameTime).count()*MATH_CONVERSION_SC;
	delta_time = delta_time_real*time_factor;

	// fps counter
#ifdef DEBUG
	f64 __LFrameUpdate = (std::chrono::steady_clock::now()-m_LastFrameUpdate).count()*MATH_CONVERSION_MS;
	if (__LFrameUpdate>1000)
	{
		fps = m_LFps;
		m_LFps = 0;
		m_LastFrameUpdate = std::chrono::steady_clock::now();
	}
	else m_LFps++;
#endif
}

/**
 *	close the window
 */
void Frame::close()
{
	COMM_MSG(LOG_CYAN,"closing window");

#ifdef VKBUILD
	g_Vk.vanish();
#else
	SDL_GL_DeleteContext(m_Context);
#endif

	SDL_Quit();
	COMM_SCC("goodbye.");
}

/**
 *	enable gpu based vsync, adaptive if possible: fallback regular vsync
 */
void Frame::gpu_vsync_on()
{
	COMM_AWT("setting gpu vsync");
#ifdef VKBUILD
	// TODO
#else
	if (SDL_GL_SetSwapInterval(-1)==-1)
	{
		COMM_ERR("adaptive vsync is not supported");
		SDL_GL_SetSwapInterval(1);
	}
#endif
	COMM_CNF();
}

/**
 *	disable gpu based vsync
 */
void Frame::gpu_vsync_off()
{
#ifdef VKBUILD
	// TODO
#else
	SDL_GL_SetSwapInterval(0);
#endif
}
