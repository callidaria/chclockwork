#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER


#include "blitter.h"
#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget Colour Buffers

typedef
#ifdef VKBUILD
VkImageView
#else
u32
#endif
__fbuffer_component;

class Framebuffer
{
public:
	Framebuffer() {  }
	void setup(f32 width,f32 height,ShaderPipeline& sp,s16 result_buffer=-1);
	void vanish();

	// usage
	void record();
	static void stop();
	void bind_colour_component(u8 channel,u8 i);
	void bind_depth_component(u8 channel);

private:
#ifdef VKBUILD
private:
	vector<VkImage> m_AttachmentImages;
	vector<VkDeviceMemory> m_AttachmentMemory;
	VkFramebuffer m_Framebuffer;
	VkRenderPass m_RenderPass;
	BitwiseWords m_ResultAttachmentMap;
#else
	u32 m_Buffer;
#endif
	u8 m_DepthChannel;
	bool m_HasDepth;

	// textures
	vector<__fbuffer_component> components;  // FIXME only when not target? how to?
};


#endif
