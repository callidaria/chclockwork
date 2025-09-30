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
// TODO more detailed gpu error logging for ogl version

#endif
#endif


// ----------------------------------------------------------------------------------------------------
// Hardware Interaction

#ifdef VKBUILD

/**
 *	TODO
 */
/*
void Eruption::register_pipeline(VkRenderPass render_pass)
{
	COMM_LOG("registration of final destination pipeline");

	// generate framebuffers
	ref_render_pass = render_pass;
	finish_swapchain();

	// setup command pool
	VkCommandPoolCreateInfo __CMDPoolInfo = {  };
	__CMDPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	__CMDPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	__CMDPoolInfo.queueFamilyIndex = graphical_queue_id;
	VkResult __Result = vkCreateCommandPool(gpu,&__CMDPoolInfo,nullptr,&cmds);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create vulkan command pool");

	// setup command buffer
	VkCommandBuffer __CommandBuffers[FRAME_BLITTER_BUFFERS];
	VkCommandBufferAllocateInfo __CMDBufferInfo = {  };
	__CMDBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	__CMDBufferInfo.commandPool = cmds;
	__CMDBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	__CMDBufferInfo.commandBufferCount = FRAME_BLITTER_BUFFERS;
	__Result = vkAllocateCommandBuffers(gpu,&__CMDBufferInfo,__CommandBuffers);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate vulkan command buffer");
	// TODO pre-store certain usual commands as secondary... yeah some research in the future about this one

	// store command buffers
	cmd_buffers.resize(FRAME_BLITTER_BUFFERS);
	for (u8 i=0;i<cmd_buffers.size();i++) cmd_buffers[i].buffer = __CommandBuffers[i];

	// setup buffer threading constraints info
	VkSemaphoreCreateInfo __SemaphoreInfo = {  };
	__SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo __FenceInfo = {  };
	__FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	__FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// iterate buffer semaphore creation
	for (u8 i=0;i<FRAME_BLITTER_BUFFERS;i++)
	{
		// create command buffer semaphore
		__Result = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&cmd_buffers[i].ready);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup buffer semaphore %u",i);

		// create command buffer fence
		__Result = vkCreateFence(gpu,&__FenceInfo,nullptr,&cmd_buffers[i].processing);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup host fence");
	}

	// image semaphore creation
	render_done.resize(images.size());
	for (u8 i=0;i<images.size();i++)
	{
		__Result = vkCreateSemaphore(gpu,&__SemaphoreInfo,nullptr,&render_done[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup image semaphore %u",i);
	}

	COMM_SCC("render pipeline ready.");
}
*/

/**
 *	TODO
 */
/*
void Eruption::vanish()
{
	for (u8 i=0;i<images.size();i++) vkDestroySemaphore(gpu,render_done[i],nullptr);
	for (CommandBuffer& p_Buffer : cmd_buffers)
	{
		vkDestroySemaphore(gpu,p_Buffer.ready,nullptr);
		vkDestroyFence(gpu,p_Buffer.processing,nullptr);
	}
	vkDestroyCommandPool(gpu,cmds,nullptr);
	destroy_swapchain();
}
*/

/**
 *	TODO
 */
/*
void Eruption::finish_swapchain()
{
	// basic setup for all final framebuffers
	VkFramebufferCreateInfo __FramebufferInfo = {  };
	__FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	__FramebufferInfo.renderPass = ref_render_pass;
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
}
*/

/**
 *	TODO
 */
/*
void Eruption::rebuild_swapchain()
{
	vkDeviceWaitIdle(gpu);
	destroy_swapchain();
	selected_gpu->assemble_swapchain(ref_frame);
	// TODO recreate render pass as well
	finish_swapchain();
}
*/

/**
 *	TODO
 */
/*
void Eruption::destroy_swapchain()
{
	for (VkFramebuffer p_Framebuffer : framebuffers) vkDestroyFramebuffer(gpu,p_Framebuffer,nullptr);
	for (VkImageView p_ImageView : image_views) vkDestroyImageView(gpu,p_ImageView,nullptr);
	vkDestroySwapchainKHR(gpu,swapchain,nullptr);
}
*/

/**
 *	TODO
 */
/*
CommandBuffer* Eruption::aquire_command_buffer()
{
	// tick command buffer
	CommandBuffer* out = &cmd_buffers[active_buffer];
	active_buffer = (active_buffer+1)%FRAME_BLITTER_BUFFERS;

	// wait until draw is ready
	vkWaitForFences(g_Vk.gpu,1,&out->processing,VK_TRUE,UINT64_MAX);
	vkResetFences(g_Vk.gpu,1,&out->processing);
	return out;
}
*/

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
	// Vulkan Setup
#ifdef VKBUILD
	COMM_MSG(LOG_CYAN,"opening vulkan window");
	u8 did = 0;
	m_Frame = SDL_CreateWindow(FRAME_GAME_NAME,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
							   width,height,/*SDL_WINDOW_RESIZABLE|*/SDL_WINDOW_VULKAN);

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
	SDL_Vulkan_GetInstanceExtensions(m_Frame,&__ExtensionCount,nullptr);
	vector<const char*> __Extensions(__ExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(m_Frame,&__ExtensionCount,&__Extensions[0]);
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
	VkResult __Result = vkCreateInstance(&__CreateInfo,nullptr,&m_Instance);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create vulkan instance");

#ifdef DEBUG
	COMM_LOG("setting up gpu error log");
	PFN_vkCreateDebugUtilsMessengerEXT __CreateMessenger
		= (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance,"vkCreateDebugUtilsMessengerEXT");
	__Result = __CreateMessenger(m_Instance,&__DebugMessengerInfo,nullptr,&debug_messenger);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to set up gpu error logging");
#endif

	COMM_LOG("setting up render surface");
	bool __SurfaceResult = SDL_Vulkan_CreateSurface(m_Frame,m_Instance,&m_Surface);
	COMM_ERR_COND(!__SurfaceResult,"failed to initialize render surface");

	// hardware detection & gpu selection
	m_Hardware.detect(m_Instance,m_Surface);
	m_Hardware.gpus[did].select();
	// FIXME just selecting the first possible gpu without feature checking or evaluating is dangerous!

	// TODO assemble swapchain

	// setup swap command information
	m_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	m_PresentInfo.waitSemaphoreCount = 1;
	m_PresentInfo.swapchainCount = 1;
	m_PresentInfo.pSwapchains = &g_Vk.swapchain;
	m_PresentInfo.pResults = nullptr;


	// ----------------------------------------------------------------------------------------------------
	// OpenGL Setup
#else
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
	// TODO all of this will be capsd as gpu settings features. those initials should be called in each version

	// gpu error log
#if defined(DEBUG) && !defined(__APPLE__)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(_gpu_error_callback,nullptr);
#endif
	// TODO this can also be it's own feature. those setup steps don't have to have a strict macro border
#endif

	// vsync
	if (vsync) gpu_vsync_on();
	else gpu_vsync_off();

	// standard settings
	set_clear_colour(BLITTER_CLEAR_COLOUR);

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
	m_PresentInfo.pWaitSemaphores = &g_Vk.render_done[frame_id];
	m_PresentInfo.pImageIndices = &frame_id;
	VkResult __Result = vkQueuePresentKHR(g_GPU.presentation_queue,&m_PresentInfo);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"there has been an issue with frame presentation");
#else
	SDL_GL_SwapWindow(m_Frame);
#endif

	// calculate delta time
	m_LastFrameTime = m_CurrentFrameTime;
	m_CurrentFrameTime = std::chrono::steady_clock::now();
	delta_time_real = (m_CurrentFrameTime-m_LastFrameTime).count()*MATH_CONVERSION_SC;
	delta_time = delta_time_real*time_factor;

#ifdef DEBUG
	// fps counter
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
	g_GPU.stop();
	vkDestroySurfaceKHR(m_Instance,m_Surface,nullptr);
#ifdef DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT __DestroyMessenger
		= (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance,
																	  "vkDestroyDebugUtilsMessengerEXT");
	__DestroyMessenger(m_Instance,debug_messenger,nullptr);
#endif
	vkDestroyInstance(m_Instance,nullptr);
#else
	SDL_GL_DeleteContext(m_Context);
#endif

	SDL_Quit();
	COMM_SCC("goodbye.");
}

/**
 *	TODO
 */
void Frame::set_clear_colour(vec3 colour)
{
#ifdef VKBUILD
	g_Vk.clear_colour = {{{ colour.r,colour.g,colour.b,1.f }}};
#else
	glClearColor(colour.r,colour.g,colour.b,0);
#endif
}

/**
 *	TODO
 */
void Frame::set_clear_depth(f32 depth)
{
#ifdef VKBUILD
	// TODO implement vulkan equivalent after depth buffering feature exists
	//		also this maybe not a necessary feature. i think this issue has been solved alternatively (sdwbounds)

#else
	glClearDepth(depth);
#endif
}

/**
 *	enable gpu based vsync, adaptive if possible: fallback regular vsync
 */
void Frame::gpu_vsync_on()
{
	COMM_AWT("setting gpu vsync");
#ifdef VKBUILD
	// TODO vsync switch happens when selecting the gpu in the vulkan version. make it switchable

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
	// TODO see gpu_vsync_on todo comment to find out why there is nothing here!

#else
	SDL_GL_SetSwapInterval(0);
#endif
}

/**
 * TODO
 */
void Frame::gpu_set_viewport(u32 width,u32 height)
{
#ifdef VKBUILD
	// TODO

#else
	glViewport(0,0,width,height);
#endif
}

/**
 *	TODO
 */
void Frame::gpu_cull_backfaces(bool backfaces)
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
void Frame::gpu_enable_feature(GPUFeature feature)
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
void Frame::gpu_disable_feature(GPUFeature feature)
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
void Frame::_assemble_swapchain()
{
	// format selection
	COMM_LOG("running swap chain setup");
	for (VkSurfaceFormatKHR& p_Format : g_GPU.swapchain_info.formats)
	{
		if (p_Format.format==VK_FORMAT_B8G8R8A8_SRGB&&p_Format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapchain.format = p_Format;
			goto swap_chain_selection_presentation;
		}
	}
	COMM_MSG(LOG_YELLOW,"WARNING: SRGB8 format not supported, falling back to swap chain standard");
	swapchain.format = g_GPU.swapchain_info.formats[0];

	// presentation mode selection
swap_chain_selection_presentation:
	VkPresentModeKHR __Mode;
	for (VkPresentModeKHR& p_Mode : g_GPU.swapchain_info.modes)
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
	if (g_GPU.swapchain_info.capabilities.currentExtent.width!=UINT32_MAX)
	{
		COMM_MSG(LOG_YELLOW,"WARNING: vulkan refuses the swapchain extent override, using fixed extent instead");
		swapchain.extent = g_GPU.swapchain_info.capabilities.currentExtent;
		goto swap_chain_creation;
	}
	SDL_Vulkan_GetDrawableSize(frame,&__Width,&__Height);
	swapchain.extent = {
		.width = glm::clamp((u32)__Width,
							g_GPU.swapchain_info.capabilities.minImageExtent.width,
							g_GPU.swapchain_info.capabilities.maxImageExtent.width),
		.height = glm::clamp((u32)__Height,
							 g_GPU.swapchain_info.capabilities.minImageExtent.height,
							 g_GPU.swapchain_info.capabilities.maxImageExtent.height),
	};

	// create swapchain
swap_chain_creation:
	u32 __ImageCount = g_GPU.swapchain_info.capabilities.minImageCount+FRAME_BLITTER_SWAP_IMAGES;
	__ImageCount = (g_GPU.swapchain_info.capabilities.maxImageCount>0
					&&__ImageCount>g_GPU.swapchain_info.capabilities.maxImageCount)
			? g_GPU.swapchain_info.capabilities.maxImageCount : __ImageCount;

	// swapchain definition
	VkSwapchainCreateInfoKHR __SwapchainInfo = {  };
	__SwapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	__SwapchainInfo.surface = g_Vk.surface;
	__SwapchainInfo.minImageCount = __ImageCount;
	__SwapchainInfo.imageFormat = swapchain.format.format;
	__SwapchainInfo.imageColorSpace = swapchain.format.colorSpace;
	__SwapchainInfo.imageExtent = swapchain.extent;
	__SwapchainInfo.imageArrayLayers = 1;
	__SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // TODO change to TRANSFER_DST_BIT later
	__SwapchainInfo.preTransform = g_GPU.swapchain_info.capabilities.currentTransform;
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
	VkResult __Result = vkCreateSwapchainKHR(g_GPU.gpu,&__SwapchainInfo,nullptr,&swapchain.swapchain);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not initialize swap chain");

	// reference swapchain images
	u32 __SCICount;
	vkGetSwapchainImagesKHR(g_GPU.gpu,swapchain.swapchain,&__SCICount,nullptr);
	COMM_ERR_COND(!__SCICount,"no swapchain images to reference");
	g_Vk.images.resize(__SCICount);
	vkGetSwapchainImagesKHR(g_GPU.gpu,swapchain.swapchain,&__SCICount,&g_Vk.images[0]);

	// image view memory & creation info setup
	g_Vk.image_views.resize(__SCICount);
	VkImageViewCreateInfo __IVInfo = {  };
	__IVInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	__IVInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	__IVInfo.format = swapchain.format.format;
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
		__Result = vkCreateImageView(g_GPU.gpu,&__IVInfo,nullptr,&g_Vk.image_views[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"faled to create image view for swapchain image %i",i);
	}
	// TODO when having an idea of the bigger *picture* outsource this to buffer as texture gen AND rndtarget

	// viewport setup
	g_Vk.viewport = {
		.x = .0f,
		.y = .0f,
		.width = (f32)swapchain.extent.width,
		.height = (f32)swapchain.extent.height,
		.minDepth = .0f,
		.maxDepth = 1.f,  // TODO is this value range or actual distance, probably the former right?
	};

	// scissor setup
	g_Vk.scissor = {
		.offset = { 0,0 },
		.extent = swapchain.extent,
	};
}
// TODO shortcut some features when recreating the swapchain, some selections not always necessary
// TODO make all those features selectable by the user

#endif
