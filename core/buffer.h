#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER

#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget

// format tuple, important information for render pass creation
// usage: drop this as a new render stage in renderer, predefine formats & depth. then pass it in fb setup. done.
struct RenderPass
{
	VkRenderPass render_pass;
	const vector<GPUBufferFormat> formats;
	const bool depth;
};

// the actual rendertarget, colour buffer result from shader pipeline rasterization stage
class Framebuffer
{
public:
	Framebuffer() {  }
	void setup(TargetResolution res,RenderPass* rp,s16 result_buffer=-1);
	static void setup_result(RenderPass* rp,vector<Framebuffer>& fbs);
	void vanish();

	// usage
	void record();
	static void stop();
	void bind_colour_component(u8 channel,u8 i);
	void bind_depth_component(u8 channel);

private:
	void _create_render_pass(RenderPass* rp);
	void _define_colour_component(u8 i,s16 rb);
	void _define_depth_component(u8 depth_channel);

public:
	vector<VkImage> attachment_images;
	vector<__fbuffer_component> components;

private:
#ifdef VKBUILD
	// component attributes
	vector<VkDeviceMemory> m_AttachmentMemory;
	vector<VkAttachmentDescription> m_AttachDesc;
	vector<VkClearValue> m_ClearValues;
	VkFramebuffer m_Framebuffer;
	FrameDimensions* pm_FrameDimension;
	VkRenderPassBeginInfo m_RPBeginInfo = {  };
	u8 m_ComponentCount = 0;
	bool m_ResultBuffer = false;
#else
	u32 m_Buffer;
#endif
};


#endif
