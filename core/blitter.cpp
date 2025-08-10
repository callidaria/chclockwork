#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// GPU Error Callbacks

#ifdef DEBUG
#ifdef VKBUILD
vector<const char*> _validation_layers = { "VK_LAYER_KHRONOS_validation" };
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
	__ErrType += (type&VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) ? "!Memory " : "";
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
// Hardware Detection

#ifdef VKBUILD

/**
 *	TODO
 */
VkSwapchainKHR SwapChain::select(SDL_Window* frame,VkDevice gpu,VkSurfaceKHR surface,u32 gqueue,
								 u32 pqueue,set<u32>& queues)
{
	COMM_LOG("running swap chain setup");

	// format selection
	VkSurfaceFormatKHR __Format;
	for (VkSurfaceFormatKHR& p_Format : formats)
	{
		if (p_Format.format==VK_FORMAT_B8G8R8A8_SRGB&&p_Format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			__Format = p_Format;
			goto swap_chain_selection_presentation;
		}
	}
	COMM_MSG(LOG_YELLOW,"WARNING: SRGB8 format not supported, falling back to swap chain standard");
	__Format = formats[0];

	// presentation mode selection
swap_chain_selection_presentation:
	VkPresentModeKHR __Mode;
	for (VkPresentModeKHR& p_Mode : modes)
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
	VkExtent2D __Extent;
	s32 __Width,__Height;
	if (capabilities.currentExtent.width!=UINT32_MAX)
	{
		COMM_MSG(LOG_YELLOW,"WARNING: vulkan refuses the swapchain extent override, using fixed extent instead");
		__Extent = capabilities.currentExtent;
		goto swap_chain_creation;
	}
	SDL_Vulkan_GetDrawableSize(frame,&__Width,&__Height);
	__Extent = {
		.width = glm::clamp((u32)__Width,capabilities.minImageExtent.width,capabilities.maxImageExtent.width),
		.height = glm::clamp((u32)__Height,
							 capabilities.minImageExtent.height,capabilities.maxImageExtent.height),
	};

	// create swapchain
swap_chain_creation:
	u32 __ImageCount = capabilities.minImageCount+FRAME_BLITTER_SWAP_IMAGES;
	__ImageCount = (capabilities.maxImageCount>0&&__ImageCount>capabilities.maxImageCount)
			? capabilities.maxImageCount : __ImageCount;

	// swapchain definition
	VkSwapchainCreateInfoKHR __SwapchainInfo = {};
	__SwapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	__SwapchainInfo.surface = surface;
	__SwapchainInfo.minImageCount = __ImageCount;
	__SwapchainInfo.imageFormat = __Format.format;
	__SwapchainInfo.imageColorSpace = __Format.colorSpace;
	__SwapchainInfo.imageExtent = __Extent;
	__SwapchainInfo.imageArrayLayers = 1;
	__SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // TODO change to TRANSFER_DST_BIT later
	__SwapchainInfo.preTransform = capabilities.currentTransform;
	__SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // TODO very, very interesting...
	__SwapchainInfo.presentMode = __Mode;
	__SwapchainInfo.clipped = VK_TRUE;
	__SwapchainInfo.oldSwapchain = VK_NULL_HANDLE;  // TODO geez this looks like a ton of work in the future

	// in case of split graphics & presentation queue
	vector<u32> __Queues = vector<u32>(queues.begin(),queues.end());
	if (gqueue!=pqueue)
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
	VkSwapchainKHR __SwapChain;
	VkResult __Result = vkCreateSwapchainKHR(gpu,&__SwapchainInfo,nullptr,&__SwapChain);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not initialize swap chain");
	return __SwapChain;
}
// TODO make all those features selectable by the user

/**
 *	detect gpus
 *	TODO
 */
void Hardware::detect(VkInstance instance,VkSurfaceKHR surface)
{
	COMM_LOG("detecting available GPUs");
	u32 __GPUCount;
	vkEnumeratePhysicalDevices(instance,&__GPUCount,nullptr);
	COMM_ERR_COND(!__GPUCount,"no vulkan capable gpus found. use opengl version!")
	COMM_SCC_FALLBACK("found %u vulkan capable graphics card%s",__GPUCount,(__GPUCount>1)?"s":"");
	physical_gpus.resize(__GPUCount);
	vkEnumeratePhysicalDevices(instance,&__GPUCount,&physical_gpus[0]);

	// scanning available gpus for specifics
	gpus.resize(__GPUCount);
	for (u8 i=0;i<__GPUCount;i++)
	{
		// get available queue families
		u32 __QueueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_gpus[i],&__QueueCount,nullptr);
		vector<VkQueueFamilyProperties> __Queues(__QueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_gpus[i],&__QueueCount,&__Queues[0]);

		// iterate queue families & extract ids
		for (u32 j=0;j<__QueueCount;j++)
		{
			// relevant queue features
			bool __GraphicalQueue = __Queues[j].queueFlags&VK_QUEUE_GRAPHICS_BIT;
			VkBool32 __PresentingQueue = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_gpus[i],j,surface,&__PresentingQueue);

			// store info & check for sufficient queue support
			if (__GraphicalQueue) gpus[i].graphical_queue = j;
			if (__PresentingQueue) gpus[i].presentation_queue = j;
			if (gpus[i].graphical_queue!=-1&&gpus[i].presentation_queue!=-1)
			{
				gpus[i].queues = { (u32)gpus[i].graphical_queue,(u32)gpus[i].presentation_queue };
				gpus[i].supported = true;
				break;
			}
		}

		// interrupt gpu read should queue support not be sufficient
		if (!gpus[i].supported)
		{
			COMM_ERR("interrupting GPU read at index %i due to insufficient queue support",i);
			continue;
		}

		// get available extensions
		u32 __ExtensionCount,__FormatCount,__ModeCount;
		vkEnumerateDeviceExtensionProperties(physical_gpus[i],nullptr,&__ExtensionCount,nullptr);
		gpus[i].extensions.resize(__ExtensionCount);
		vkEnumerateDeviceExtensionProperties(physical_gpus[i],nullptr,&__ExtensionCount,&gpus[i].extensions[0]);

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
		vkGetPhysicalDeviceProperties(physical_gpus[i],&gpus[i].properties);
		vkGetPhysicalDeviceFeatures(physical_gpus[i],&gpus[i].features);
		COMM_SCC("found supported GPU %s",gpus[i].properties.deviceName);
		// TODO later, read the capabilities of the selected device, allow to change it and change features
		// TODO something something, queue families, tldr okok i will do this later, probably works on my system
		// TODO another something, not only should the required qfs be available but also all needed extensions

		// get swap chain format capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_gpus[i],surface,&gpus[i].swap_chain.capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_gpus[i],surface,&__FormatCount,nullptr);
		if (!!__FormatCount)
		{
			gpus[i].swap_chain.formats.resize(__FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_gpus[i],surface,
												 &__FormatCount,&gpus[i].swap_chain.formats[0]);
		}
		COMM_ERR_FALLBACK("no surface formats found for GPU %s",gpus[i].properties.deviceName);

		// get swap chain mode capabilities
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_gpus[i],surface,&__ModeCount,nullptr);
		if (!!__ModeCount)
		{
			gpus[i].swap_chain.modes.resize(__ModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_gpus[i],surface,
													  &__ModeCount,&gpus[i].swap_chain.modes[0]);
		}
		COMM_ERR_FALLBACK("no presentation modes found for GPU %s",gpus[i].properties.deviceName);
		// TODO depending on the swap chain capabilities, rule out possible bad devices & increase safety
		//		careful! if the gpu has no swap chain extension in the first place this can get ugly!
	}
}

/**
 *	TODO
 */
void Hardware::select_gpu(VkDevice& logical_gpu,VkQueue& gqueue,VkQueue& pqueue,u8 id)
{
	COMM_LOG("selecting gpu %s",gpus[id].properties.deviceName);
	COMM_ERR_COND(!gpus[id].supported,"selected gpu %u is not supported",id);

	// queue creation
	f32 __QueuePriority = 1.f;
	vector<VkDeviceQueueCreateInfo> __QueueInfos;
	__QueueInfos.reserve(gpus[id].queues.size());
	for (u32 __QueueID : gpus[id].queues)
	{
		__QueueInfos.push_back({  });
		__QueueInfos.back().sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		__QueueInfos.back().queueFamilyIndex = __QueueID;
		__QueueInfos.back().queueCount = 1;
		__QueueInfos.back().pQueuePriorities = &__QueuePriority;
	}

	// device features
	VkPhysicalDeviceFeatures __DeviceFeatures = {};  // TODO

	// device creation specifics
	VkDeviceCreateInfo __DeviceInfo = {};
	__DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	__DeviceInfo.queueCreateInfoCount = (u32)__QueueInfos.size();
	__DeviceInfo.pQueueCreateInfos = &__QueueInfos[0];
	__DeviceInfo.enabledExtensionCount = (u32)g_GPUExtensions.size();
	__DeviceInfo.ppEnabledExtensionNames = &g_GPUExtensions[0];
	__DeviceInfo.pEnabledFeatures = &__DeviceFeatures;

	// enable validation layers here as well for safety, even though it's deprecated
#ifdef DEBUG
	__DeviceInfo.enabledLayerCount = (u32)_validation_layers.size();
	__DeviceInfo.ppEnabledLayerNames = &_validation_layers[0];
#else
	__DeviceInfo.enabledLayerCount = 0;
#endif

	// create device
	VkResult __Result = vkCreateDevice(physical_gpus[id],&__DeviceInfo,nullptr,&logical_gpu);
	COMM_ERR_COND(__Result!=VK_SUCCESS,
				  "could not create logical interface for gpu %s",gpus[id].properties.deviceName);

	// initialize queues
	vkGetDeviceQueue(logical_gpu,gpus[id].graphical_queue,0,&gqueue);
	vkGetDeviceQueue(logical_gpu,gpus[id].presentation_queue,0,&pqueue);
}

#endif


// ----------------------------------------------------------------------------------------------------
// Graphical Frame

/**
 *	open a graphical window
 *	\param title: window title displayed in decoration and program listing
 *	\param width: window dimension width
 *	\param height: window dimension height
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
	m_Frame = SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
							   width,height,SDL_WINDOW_VULKAN);

	// application info
	VkApplicationInfo __ApplicationInfo = {};
	__ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	__ApplicationInfo.pApplicationName = title;
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
	VkDebugUtilsMessengerCreateInfoEXT __DebugMessengerInfo = {};
	__DebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	__DebugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	__DebugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
			|VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
	__DebugMessengerInfo.pfnUserCallback = _gpu_error_callback;
	__Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	COMM_LOG("creating vulkan instance");
	VkInstanceCreateInfo __CreateInfo = {};
	__CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	__CreateInfo.pApplicationInfo = &__ApplicationInfo;
	__CreateInfo.enabledLayerCount = 0;
	__CreateInfo.enabledExtensionCount = (u32)__Extensions.size();
	__CreateInfo.ppEnabledExtensionNames = &__Extensions[0];

	// setup validation layers for gpu auto-logging
#ifdef DEBUG
	__CreateInfo.enabledLayerCount = (u32)_validation_layers.size();
	__CreateInfo.ppEnabledLayerNames = &_validation_layers[0];
	__CreateInfo.pNext = &__DebugMessengerInfo;
#endif

	// creating vulkan instance
	VkResult __Result = vkCreateInstance(&__CreateInfo,nullptr,&m_Instance);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create vulkan instance");

#ifdef DEBUG
	COMM_LOG("setting up gpu error log");
	PFN_vkCreateDebugUtilsMessengerEXT __CreateMessenger
			= (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance,
																		"vkCreateDebugUtilsMessengerEXT");
	__Result = __CreateMessenger(m_Instance,&__DebugMessengerInfo,nullptr,&m_DebugMessenger);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to set up gpu error logging");
#endif

	COMM_LOG("setting up render surface");
	bool __SurfaceResult = SDL_Vulkan_CreateSurface(m_Frame,m_Instance,&m_Surface);
	COMM_ERR_COND(!__SurfaceResult,"failed to initialize render surface");

	// gpu setup
	u8 did = 0;
	m_Hardware.detect(m_Instance,m_Surface);
	m_Hardware.select_gpu(m_GPULogical,m_GraphicsQueue,m_PresentationQueue,did);
	m_SwapChain = m_Hardware.gpus[did].swap_chain.select(m_Frame,m_GPULogical,m_Surface,
														 m_Hardware.gpus[did].graphical_queue,
														 m_Hardware.gpus[did].presentation_queue,
														 m_Hardware.gpus[did].queues);
	// FIXME just selecting the first possible gpu without feature checking or evaluating is dangerous!
	// FIXME architecture of those calls create barely survivable conditions for my future career
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
	vkDestroySwapchainKHR(m_GPULogical,m_SwapChain,nullptr);
	vkDestroyDevice(m_GPULogical,nullptr);
	vkDestroySurfaceKHR(m_Instance,m_Surface,nullptr);
#ifdef DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT __DestroyMessenger
			= (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance,
																		  "vkDestroyDebugUtilsMessengerEXT");
	__DestroyMessenger(m_Instance,m_DebugMessenger,nullptr);
#endif
	vkDestroyInstance(m_Instance,nullptr);
	// TODO test if unclean destruction of device and buffer leads to the validation layer complaining
	//		it does not even if it normally should be, further investigation necessary. verbose & warning works.
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
