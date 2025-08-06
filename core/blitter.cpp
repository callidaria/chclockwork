#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// GPU Error Callbacks

#ifdef DEBUG
#ifdef VKBUILD

vector<const char*> _validation_layers = { "VK_LAYER_KHRONOS_validation" };
const char* _gpu_error_types[] = { "General","Specifics","Performance" };
VKAPI_ATTR VkBool32 VKAPI_CALL _gpu_error_callback(VkDebugUtilsMessageSeverityFlagBitsEXT sev,
												   VkDebugUtilsMessageTypeFlagsEXT type,
												   const VkDebugUtilsMessengerCallbackDataEXT* cb,
												   void* udata)
{
	switch (sev)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		COMM_ERR("[GPU] %s!%s%s %s",LOG_BLUE,_gpu_error_types[type],LOG_RED,cb->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		COMM_MSG(LOG_YELLOW,"[GPU Warning] %s!%s%s %s",LOG_BLUE,_gpu_error_types[type],LOG_YELLOW,cb->pMessage);
		break;
	};
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
	SDL_Init(SDL_INIT_VIDEO);

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
	VkApplicationInfo __ApplicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = title,
		.applicationVersion = VK_MAKE_VERSION(0,0,1),
		.pEngineName = "C. Hanson's Clockwork",
		.engineVersion = VK_MAKE_VERSION(0,0,1),
		.apiVersion = VK_API_VERSION_1_0,
	};

	// extensions
	u32 __ExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(m_Frame,&__ExtensionCount,nullptr);
	vector<const char*> __Extensions(__ExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(m_Frame,&__ExtensionCount,&__Extensions[0]);
#ifdef DEBUG
	VkDebugUtilsMessengerCreateInfoEXT __DebugMessengerInfo {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				|VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
				|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
				|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = _gpu_error_callback,
	};
	__Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	COMM_LOG("creating vulkan instance");
	VkInstanceCreateInfo __CreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &__ApplicationInfo,
		.enabledLayerCount = 0,
		.enabledExtensionCount = (u32)__Extensions.size(),
		.ppEnabledExtensionNames = &__Extensions[0],
	};
#ifdef DEBUG
	__CreateInfo.enabledLayerCount = (u32)_validation_layers.size();
	__CreateInfo.ppEnabledLayerNames = &_validation_layers[0];
	__CreateInfo.pNext = &__DebugMessengerInfo;
#endif
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
#endif

	// vsync
	if (vsync) gpu_vsync_on();
	else gpu_vsync_off();

	// standard settings
	set_clear_colour(BLITTER_CLEAR_COLOUR);
//#endif

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
	PFN_vkDestroyDebugUtilsMessengerEXT __DestroyMessenger
			= (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance,
																		  "vkDestroyDebugUtilsMessengerEXT");
	__DestroyMessenger(m_Instance,m_DebugMessenger,nullptr);
	vkDestroyInstance(m_Instance,nullptr);
	// TODO test if unclean destruction of device and buffer leads to the validation layer complaining
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
