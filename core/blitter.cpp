#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// GPU Error Callbacks

void GLAPIENTRY _gpu_error_callback(GLenum src,GLenum type,GLenum id,GLenum sev,GLsizei len,
									const GLchar* msg,const void* usrParam)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: COMM_ERR("[GPU] %s",msg);
		break;
	case GL_DEBUG_TYPE_PERFORMANCE: COMM_MSG(LOG_RED,"[GPU Performance Warning] %s",msg);
	};
}



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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);

	COMM_MSG(LOG_CYAN,"opening window");
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
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

/**
 *	flip buffer
 */
void Frame::update()
{
	SDL_GL_SwapWindow(m_Frame);

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

	SDL_GL_DeleteContext(m_Context);
	SDL_Quit();

	COMM_SCC("goodbye.");
}

/**
 *	set frame background colour
 *	\param colour: rgb background colour value
 */
void Frame::set_clear_colour(vec3 colour)
{
	glClearColor(colour.r,colour.g,colour.b,0);
}

/**
 *	set depth buffer default value
 *	\param depth: default depth
 */
void Frame::set_clear_depth(f32 depth)
{
	glClearDepth(depth);
}

/**
 *	enable gpu based vsync, adaptive if possible: fallback regular vsync
 */
void Frame::gpu_vsync_on()
{
	COMM_AWT("setting gpu vsync");
	if (SDL_GL_SetSwapInterval(-1)==-1)
	{
		COMM_ERR("adaptive vsync is not supported");
		SDL_GL_SetSwapInterval(1);
	}
	COMM_CNF();
}

/**
 *	disable gpu based vsync
 */
void Frame::gpu_vsync_off()
{
	SDL_GL_SetSwapInterval(0);
}
