#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER


#include "blitter.h"
#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget Colour Buffers

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
	vector<VkImage> m_AttachmentImages;
	vector<VkDeviceMemory> m_AttachmentMemory;
	VkFramebuffer m_Framebuffer;
	VkRenderPass m_RenderPass;
	BitwiseWords m_ResultAttachmentMap;
#else
	u32 m_Buffer;
#endif

public:
	vector<__fbuffer_component> components;  // FIXME only when not target? how to?
	vector<VkClearValue> m_ClearValues;
};


#endif
