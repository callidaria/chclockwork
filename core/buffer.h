#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER

#include "blitter.h"
#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget

class Framebuffer
{
public:
	Framebuffer() {  }
	void setup(FrameDimensions& fd,ShaderPipeline& sp,s16 result_buffer=-1);
	void vanish();

	// usage
	void record();
	static void stop();
	void bind_colour_component(u8 channel,u8 i);
	void bind_depth_component(u8 channel);

private:
	void _define_colour_component(u8 i,VkAttachmentDescription& desc,s16 rb);
	void _define_depth_component(u8 depth_channel);
	void _finalize();

public:
	vector<VkImage> attachment_images;
	vector<__fbuffer_component> components;
	vector<VkClearValue> m_ClearValues;

private:
#ifdef VKBUILD
	vector<VkDeviceMemory> m_AttachmentMemory;
	VkFramebuffer m_Framebuffer;
	VkRenderPass m_RenderPass;
	BitwiseWords m_ResultAttachmentMap;
	FrameDimensions m_FrameDimensions;
#else
	u32 m_Buffer;
#endif
};


#endif
