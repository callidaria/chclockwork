#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER

#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget

// format tuple, important information for render pass creation
constexpr u8 RENDER_PASS_MAXIMUM_CHANNELS = 8;
struct RenderPassFormat
{
	GPUBufferFormat formats[RENDER_PASS_MAXIMUM_CHANNELS];
	u8 size;
	bool depth;
};

class Framebuffer
{
public:
	Framebuffer() {  }
	void setup(TargetResolution res,RenderPassFormat& format,u8 result_buffer=-1);
	static void setup_result(vector<Framebuffer>& fbs);
	void vanish();

	// usage
	void record();
	static void stop();
	void bind_colour_component(u8 channel,u8 i);
	void bind_depth_component(u8 channel);

private:
	void _create_render_pass(RenderPassFormat& format);
	void _define_colour_component(u8 i,s16 rb);
	void _define_depth_component();

public:
	vector<VkImage> attachment_images;
	vector<__fbuffer_component> components;
	vector<VkClearValue> m_ClearValues;
	VkRenderPass render_pass;

private:
#ifdef VKBUILD
	vector<VkDeviceMemory> m_AttachmentMemory;
	vector<VkAttachmentDescription> m_AttachDesc;
	VkFramebuffer m_Framebuffer;
	FrameDimensions* pm_FrameDimension;
	u8 m_Channels;
	u8 m_ComponentCount = 0;
	bool m_Depth = false;
	bool m_ResultBuffer = false;
#else
	u32 m_Buffer;
#endif
};


#endif
