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
// Graphical Frame

/**
 *	open a graphical window
 */
Frame::Frame()
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
	m_Frame = SDL_CreateWindow(FRAME_PROGRAM_TITLE,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
							   FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,/*SDL_WINDOW_RESIZABLE|*/SDL_WINDOW_VULKAN);

	// application info
	VkApplicationInfo __ApplicationInfo = {  };
	__ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	__ApplicationInfo.pApplicationName = FRAME_PROGRAM_TITLE;
	__ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
	__ApplicationInfo.pEngineName = "C. Hansen's Counter-Clockwork";
	__ApplicationInfo.engineVersion = VK_MAKE_VERSION(0,0,2);

	// api version selection with fallback to 1.0 if necessary
	if (vkEnumerateInstanceVersion) vkEnumerateInstanceVersion(&__ApplicationInfo.apiVersion);
	else
	{
		COMM_MSG(LOG_BLUE,"[INFO] failed to acquire api version through enumeration, falling back to 1.0.0");
		__ApplicationInfo.apiVersion = VK_API_VERSION_1_0;
	}
	COMM_MSG(LOG_GREEN,"vulkan (%d) v%d.%d.%d",
			 VK_API_VERSION_VARIANT(__ApplicationInfo.apiVersion),
			 VK_API_VERSION_MAJOR(__ApplicationInfo.apiVersion),
			 VK_API_VERSION_MINOR(__ApplicationInfo.apiVersion),
			 VK_API_VERSION_PATCH(__ApplicationInfo.apiVersion));

	// extensions
	u32 __ExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(m_Frame,&__ExtensionCount,nullptr);
	vector<const char*> __Extensions(__ExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(m_Frame,&__ExtensionCount,&__Extensions[0]);

#ifdef DEBUG
	VkDebugUtilsMessengerCreateInfoEXT __DebugMessengerInfo = {  };
	__DebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	// additional validation features
#if LOG_STRICTNESS_CONSERVATIVE
	VkValidationFeatureEnableEXT __ValidationFeaturesEnabled[] = {
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
	};
	VkValidationFeaturesEXT __ValidationFeatures = {  };
	__ValidationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	__ValidationFeatures.enabledValidationFeatureCount = 2;
	__ValidationFeatures.pEnabledValidationFeatures = __ValidationFeaturesEnabled;
	__DebugMessengerInfo.pNext = &__ValidationFeatures;
#endif

	// debug messenger
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
			= (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
					m_Instance,"vkCreateDebugUtilsMessengerEXT"
				);
	__Result = __CreateMessenger(m_Instance,&__DebugMessengerInfo,nullptr,&debug_messenger);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to set up gpu error logging");
#endif

	COMM_LOG("setting up render surface");
	bool __SurfaceResult = SDL_Vulkan_CreateSurface(m_Frame,m_Instance,&m_Surface);
	COMM_ERR_COND(!__SurfaceResult,"failed to initialize render surface");

	// hardware detection & gpu selection
	m_Hardware.detect(m_Instance,m_Surface);
	m_Hardware.gpus[did].select();
	_assemble_swapchain();
	// FIXME just selecting the first possible gpu without feature checking or evaluating is dangerous!

	// setup swap command information
	m_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	m_PresentInfo.waitSemaphoreCount = 1;
	m_PresentInfo.swapchainCount = 1;
	m_PresentInfo.pSwapchains = &swapchain.swapchain;
	m_PresentInfo.pResults = nullptr;


	// ----------------------------------------------------------------------------------------------------
	// OpenGL Setup
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);

	COMM_MSG(LOG_CYAN,"opening OpenGL window");
	m_Frame = SDL_CreateWindow(FRAME_PROGRAM_TITLE,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
							   FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,SDL_WINDOW_OPENGL);
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
	glViewport(0,0,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
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
	if (FRAME_BLITTER_VSYNC) gpu_vsync_on();
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
	// get next swapchain image
	VkResult __Result = vkAcquireNextImageKHR(
			g_GPU.gpu,g_Frame.swapchain.swapchain,UINT64_MAX,g_GPU.acquire_graphical_command_buffer()->ready,
			VK_NULL_HANDLE,&g_Frame.frame_id
		);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"available target frame could not be acquired");
#else
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
}
// TODO rename this and move the clear from swap. 

/**
 *	flip buffer
 */
void Frame::update()
{
#ifdef VKBUILD
	m_PresentInfo.pWaitSemaphores = &render_done[frame_id];
	m_PresentInfo.pImageIndices = &frame_id;
	VkResult __Result = vkQueuePresentKHR(g_GPU.presentation_queue,&m_PresentInfo);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"there has been an issue with frame presentation");
#else
	SDL_GL_SwapWindow(m_Frame);
#endif

	// calculate delta time
	m_LastFrameTime = fstart;
	fstart = std::chrono::steady_clock::now();
	delta_time_real = calculate_delta_time_s(m_LastFrameTime);
	delta_time = delta_time_real*time_factor;

#ifdef DEBUG
	f64 __LFrameUpdate = calculate_delta_time_ms(m_LastFrameUpdate);
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
	for (u8 i=0;i<result_images.size();i++) g_GPU.free(render_done[i]);
	_destroy_swapchain();
	g_GPU.stop();
	vkDestroySurfaceKHR(m_Instance,m_Surface,nullptr);
#ifdef DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT __DestroyMessenger
		= (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance,
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
	clear_colour[0] = {{ colour.r,colour.g,colour.b,1.f }};
	clear_colour[1] = { 1.f,.0f };
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
 * TODO
 */
void Frame::set_viewport(u32 width,u32 height)
{
#ifdef VKBUILD
	// TODO

#else
	glViewport(0,0,width,height);
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


#ifdef VKBUILD

/**
 *	TODO
 */
void Frame::rebuild_swapchain()
{
	g_GPU.expect_idle();
	_destroy_swapchain();
	_assemble_swapchain();
	// TODO recreate render pass as well?
	//_finalize_swapchain();  // TODO recreate result buffer somehow?
}

/**
 *	TODO
 */
void Frame::_assemble_swapchain()
{
	// format selection
	COMM_LOG("running swap chain setup");
	for (VkSurfaceFormatKHR& p_Format : g_GPU.device_info->swapchain_info.formats)
	{
		if (p_Format.format==VK_FORMAT_B8G8R8A8_SRGB&&p_Format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapchain.format = p_Format;
			goto swap_chain_selection_presentation;
		}
	}
	COMM_MSG(LOG_YELLOW,"WARNING: SRGB8 format not supported, falling back to swap chain standard");
	swapchain.format = g_GPU.device_info->swapchain_info.formats[0];

	// presentation mode selection
swap_chain_selection_presentation:
	VkPresentModeKHR __Mode;
	for (VkPresentModeKHR& p_Mode : g_GPU.device_info->swapchain_info.modes)
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
	if (g_GPU.device_info->swapchain_info.capabilities.currentExtent.width!=UINT32_MAX)
	{
		COMM_MSG(LOG_BLUE,"INFO: device refuses the swapchain extent override, using fixed extent instead");
		swapchain.extent = g_GPU.device_info->swapchain_info.capabilities.currentExtent;
		goto swap_chain_creation;
	}
	SDL_Vulkan_GetDrawableSize(m_Frame,&__Width,&__Height);
	swapchain.extent = {
		.width = glm::clamp((u32)__Width,
							g_GPU.device_info->swapchain_info.capabilities.minImageExtent.width,
							g_GPU.device_info->swapchain_info.capabilities.maxImageExtent.width),
		.height = glm::clamp((u32)__Height,
							 g_GPU.device_info->swapchain_info.capabilities.minImageExtent.height,
							 g_GPU.device_info->swapchain_info.capabilities.maxImageExtent.height),
	};

	// create swapchain
swap_chain_creation:
	u32 __ImageCount = g_GPU.device_info->swapchain_info.capabilities.minImageCount+FRAME_BLITTER_SWAP_IMAGES;
	__ImageCount = (g_GPU.device_info->swapchain_info.capabilities.maxImageCount>0
					&&__ImageCount>g_GPU.device_info->swapchain_info.capabilities.maxImageCount)
			? g_GPU.device_info->swapchain_info.capabilities.maxImageCount : __ImageCount;

	// swapchain definition
	VkSwapchainCreateInfoKHR __SwapchainInfo = {  };
	__SwapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	__SwapchainInfo.surface = m_Surface;
	__SwapchainInfo.minImageCount = __ImageCount;
	__SwapchainInfo.imageFormat = swapchain.format.format;
	__SwapchainInfo.imageColorSpace = swapchain.format.colorSpace;
	__SwapchainInfo.imageExtent = swapchain.extent;
	__SwapchainInfo.imageArrayLayers = 1;
	__SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // TODO change to TRANSFER_DST_BIT later
	__SwapchainInfo.preTransform = g_GPU.device_info->swapchain_info.capabilities.currentTransform;
	__SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // TODO very, very interesting...
	__SwapchainInfo.presentMode = __Mode;
	__SwapchainInfo.clipped = VK_TRUE;
	__SwapchainInfo.oldSwapchain = VK_NULL_HANDLE;  // TODO geez this looks like a ton of work in the future

	// in case of split graphics & presentation queue
	vector<u32> __Queues = vector<u32>(g_GPU.device_info->queues.begin(),g_GPU.device_info->queues.end());
	if (g_GPU.device_info->graphical_queue!=g_GPU.device_info->presentation_queue)
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
	result_images.resize(__SCICount);
	vkGetSwapchainImagesKHR(g_GPU.gpu,swapchain.swapchain,&__SCICount,&result_images[0]);
	// TODO investigate against buffer count & discrepancy details...

	// image view memory & creation info setup
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
	result_image_views.resize(__SCICount);
	for (u32 i=0;i<__SCICount;i++)
	{
		__IVInfo.image = result_images[i];
		__Result = vkCreateImageView(g_GPU.gpu,&__IVInfo,nullptr,&result_image_views[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"faled to create image view for swapchain image %i",i);
	}
	// TODO when having an idea of the bigger *picture* outsource this to buffer as texture gen AND rndtarget
	// TODO repeating code is fine here? just a small definition for result buffers?

	// viewport setup
	viewport = {
		.x = .0f,
		.y = .0f,
		.width = (f32)swapchain.extent.width,
		.height = (f32)swapchain.extent.height,
		.minDepth = .0f,
		.maxDepth = 1.f,  // TODO is this value range or actual distance, probably the former right?
	};

	// scissor setup
	scissor = {
		.offset = { 0,0 },
		.extent = swapchain.extent,
	};

	// image semaphore creation
	VkSemaphoreCreateInfo __SemaphoreInfo = {  };
	__SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	render_done.resize(__SCICount);
	for (u8 i=0;i<__SCICount;i++)
	{
		VkResult __Result = vkCreateSemaphore(g_GPU.gpu,&__SemaphoreInfo,nullptr,&render_done[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to setup image semaphore %u",i);
	}
}
// TODO shortcut some features when recreating the swapchain, some selections not always necessary
// TODO make all those features selectable by the user

/**
 *	TODO
 */
void Frame::_destroy_swapchain()
{
	g_GPU.free(swapchain.swapchain);
}

#endif
