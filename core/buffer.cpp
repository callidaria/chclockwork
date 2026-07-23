#include "buffer.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget Colour Buffers

/**
 *	setup output framebuffer and allocate to satisfy given shader pipeline
 *	\param width: default resolution width for framebuffer components
 *	\param height: default resolution height for framebuffer components
 *	\param sp: shader pipeline, holding framebuffer component information
 *	\param result_buffer: (default -1) >-1 if render pass has result defined and attaches frame
 *			indexed by this variable
 */
void Framebuffer::setup(f32 width,f32 height,ShaderPipeline& sp,s16 result_buffer)
{
	// allocate component handles
	u8 __ComponentCount = sp.depth_channel+sp.has_depth;
	components.resize(__ComponentCount);

#ifdef VKBUILD
	COMM_ERR_COND(sp.render_pass==VK_NULL_HANDLE,
				  "shader pipeline must be assembled before passing it to framebuffer setup");

	// pipeline attribute store
	m_RenderPass = sp.render_pass;
	m_ResultAttachmentMap = BitwiseWords(sp.result_attachment);

	// allocate handler memory
	m_AttachmentImages.resize(__ComponentCount);
	m_AttachmentMemory.resize(__ComponentCount);
#endif

	for (u8 i=0;i<sp.depth_channel;i++)
	{
#ifdef VKBUILD
		if (m_ResultAttachmentMap[i])
		{
			COMM_ERR_COND(result_buffer<0,"result attachment defined but no buffer id given");
			m_AttachmentImages[i] = g_Frame.result_images[result_buffer];
			components[i] = g_Frame.result_image_views[result_buffer];
			continue;
		}

		// allocate image
		VkImageCreateInfo __ImageInfo = {  };
		__ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		__ImageInfo.imageType = VK_IMAGE_TYPE_2D;
		__ImageInfo.extent.width = width;
		__ImageInfo.extent.height = height;
		__ImageInfo.extent.depth = 1;
		__ImageInfo.mipLevels = 1;
		__ImageInfo.arrayLayers = 1;
		__ImageInfo.format = sp.descriptions[i].format;  // TODO consider solving this through friend instead
		__ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		__ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		__ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
		__ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		__ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		__ImageInfo.flags = 0;
		VkResult __Result = vkCreateImage(g_GPU.gpu,&__ImageInfo,nullptr,&m_AttachmentImages[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create colour attachment %d for some reason",i);

		// allocate vram
		VkMemoryRequirements __MemoryRequirements;
		vkGetImageMemoryRequirements(g_GPU.gpu,m_AttachmentImages[i],&__MemoryRequirements);
		VkMemoryAllocateInfo __MemoryInfo = {  };
		__MemoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		__MemoryInfo.allocationSize = __MemoryRequirements.size;
		__MemoryInfo.memoryTypeIndex = GPU::choose_memory_type(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															   __MemoryRequirements.memoryTypeBits);
		__Result = vkAllocateMemory(g_GPU.gpu,&__MemoryInfo,nullptr,&m_AttachmentMemory[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate VRAM for colour buffer %d for some reason",i);
		vkBindImageMemory(g_GPU.gpu,m_AttachmentImages[i],m_AttachmentMemory[i],0);
		// FIXME repeat code chunk for vram allocation here (identical to depth buffer allocation)
		// TODO check for allocation success (also for depth buffer)

		// colour buffer image view handle
		VkImageViewCreateInfo __ImageViewInfo = {  };
		__ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		__ImageViewInfo.image = m_AttachmentImages[i];
		__ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		__ImageViewInfo.format = sp.descriptions[i].format;
		__ImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		__ImageViewInfo.subresourceRange.baseMipLevel = 0;
		__ImageViewInfo.subresourceRange.levelCount = 1;
		__ImageViewInfo.subresourceRange.baseArrayLayer = 0;
		__ImageViewInfo.subresourceRange.layerCount = 1;
		__ImageViewInfo.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		};
		__Result = vkCreateImageView(g_GPU.gpu,&__ImageViewInfo,nullptr,&components[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"depth buffer image view creation failed");
#else
#endif
	}
	// FIXME repeated code again, with no concept of sensible abstraction

	// depth component allocation
	if (sp.has_depth)
	{
#ifdef VKBUILD
		// allocate image
		VkImageCreateInfo __ImageInfo = {  };
		__ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		__ImageInfo.imageType = VK_IMAGE_TYPE_2D;
		__ImageInfo.extent.width = width;
		__ImageInfo.extent.height = height;
		__ImageInfo.extent.depth = 1;
		__ImageInfo.mipLevels = 1;
		__ImageInfo.arrayLayers = 1;
		__ImageInfo.format = g_Formats.depthbuffer;
		__ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		__ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		__ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
		__ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		__ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		__ImageInfo.flags = 0;
		VkResult __Result = vkCreateImage(g_GPU.gpu,&__ImageInfo,nullptr,&m_AttachmentImages[sp.depth_channel]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create depth buffer for some reason");
		// TODO explicitly define different creation functions for depth and pixel buffer. then diversify

		// allocate vram
		VkMemoryRequirements __MemoryRequirements;
		vkGetImageMemoryRequirements(g_GPU.gpu,m_AttachmentImages[sp.depth_channel],&__MemoryRequirements);
		VkMemoryAllocateInfo __MemoryInfo = {  };
		__MemoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		__MemoryInfo.allocationSize = __MemoryRequirements.size;
		__MemoryInfo.memoryTypeIndex = GPU::choose_memory_type(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															   __MemoryRequirements.memoryTypeBits);
		__Result = vkAllocateMemory(g_GPU.gpu,&__MemoryInfo,nullptr,&m_AttachmentMemory[sp.depth_channel]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate VRAM for depth buffer for some reason");
		vkBindImageMemory(g_GPU.gpu,m_AttachmentImages[sp.depth_channel],m_AttachmentMemory[sp.depth_channel],0);
		// FIXME a lot of code repitition, but abstracting this will loose too much functionality?

		// depth buffer image view handle
		VkImageViewCreateInfo __ImageViewInfo = {  };
		__ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		__ImageViewInfo.image = m_AttachmentImages[sp.depth_channel];
		__ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		__ImageViewInfo.format = g_Formats.depthbuffer;
		__ImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		__ImageViewInfo.subresourceRange.baseMipLevel = 0;
		__ImageViewInfo.subresourceRange.levelCount = 1;
		__ImageViewInfo.subresourceRange.baseArrayLayer = 0;
		__ImageViewInfo.subresourceRange.layerCount = 1;
		__ImageViewInfo.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		};
		__Result = vkCreateImageView(g_GPU.gpu,&__ImageViewInfo,nullptr,&components[sp.depth_channel]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"depth buffer image view creation failed");
		// FIXME another code repetition here, see blitter.cpp. abstract and allow for multiple images by pointer
		// TODO this is much more abstractable, but also not really?
		// TODO allow for independent depth buffer allocation (?in blitter) and bind just result colour buffer

#else
#endif
	}

	// create framebuffer
#ifdef VKBUILD
	VkFramebufferCreateInfo __FramebufferInfo = {  };
	__FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	__FramebufferInfo.renderPass = sp.render_pass;
	__FramebufferInfo.attachmentCount = components.size();
	__FramebufferInfo.width = width;
	__FramebufferInfo.height = height;
	__FramebufferInfo.layers = 1;
	__FramebufferInfo.pAttachments = &components[0];
	VkResult __Result = vkCreateFramebuffer(g_GPU.gpu,&__FramebufferInfo,nullptr,&m_Framebuffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create framebuffer");
#else
	glGenFramebuffers(1,&m_Buffer);
	glGenTextures(__ComponentCount,&components[0]);
#endif
};

/**
 *	colour component definition, allowed as many as the constructor has allocated
 *	\param index: frambuffer component index
 *	\param width: resolution width
 *	\param height: resolution height
 *	\param skip_alloc: (default true) false if image allocation should be skipped, e.g. when result is linked
 *	\param fbuffer: (default false) true if floatbuffer when extra precision is needed
 *	TODO update
 */
/*
inline void Framebuffer::define_colour_component(u8 index,f32 width,f32 height,bool fbuffer,s8 result_buffer)
{
	COMM_ERR_COND(!(index<m_DepthChannel),"colour component definition index outside of valid allocated range");

#ifdef VKBUILD
	// TODO remove feature
#else
	glBindTexture(GL_TEXTURE_2D,components[index]);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA+0x6f12*fbuffer,width,height,0,GL_RGBA,GL_UNSIGNED_INT+fbuffer,NULL);
	Texture::set_texture_parameter_nearest_unfiltered();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+index,GL_TEXTURE_2D,components[index],0);
#endif
}
*/
// TODO maybe define index inside the framebuffer struct as a cursor counter variable

/**
 *	TODO
 */
/*
void Framebuffer::define_colour_component(u8 index,bool fbuffer,u8 result_buffer)
{
	define_colour_component(index,m_Width,m_Height,fbuffer,result_buffer);
}
*/

/**
 *	depth component definition, only a single one per framebuffer allowed for obvious reasons
 *	\param width: resolution width
 *	\param height: resolution height
 *	TODO
 */
/*
inline void Framebuffer::define_depth_component(f32 width,f32 height)
{
	COMM_ERR_COND(!m_HasDepth,
				  "framebuffer defines depth component, but no previous signal for allocation was set");

#ifdef VKBUILD
	// TODO remove definition feature
#else
	glGenTextures(1,&m_DepthComponent);
	glBindTexture(GL_TEXTURE_2D,m_DepthComponent);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,width,height,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
	Texture::set_texture_parameter_nearest_unfiltered();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,m_DepthComponent,0);
#endif
}
*/

/**
 *	TODO
 */
/*
void Framebuffer::define_depth_component()
{
	define_depth_component(m_Width,m_Height);
}
*/

/**
 *	combine previously defined framebuffer attachments
 *	NOTE: this has to happen after definitions of all components
 *	TODO update
 */
/*
void Framebuffer::finalize()
{
#ifdef VKBUILD
	// TODO remove feature
#else
	u32 __Attachments[components.size()];
	for (u8 i=0;i<components.size();i++) __Attachments[i] = GL_COLOR_ATTACHMENT0+i;
	glDrawBuffers(components.size(),__Attachments);
#endif
}
*/

/**
 *	destroys framebuffer and frees all allocated resources
 */
void Framebuffer::vanish()
{
#ifdef VKBUILD
	// free component handle
	for (u8 i=0;i<components.size();i++)
	{
		g_GPU.free(components[i]);

		// free component memory should it have been allocated by the framebuffer
		if (m_ResultAttachmentMap[i]) continue;
		g_GPU.free(m_AttachmentMemory[i]);
		g_GPU.free(m_AttachmentImages[i]);
	}
	m_ResultAttachmentMap.vanish();

	// kill framebuffer and render pass information
	g_GPU.free(m_Framebuffer);
#endif
}

/**
 *	clear buffer and start recording process
 */
void Framebuffer::record()
{
#ifdef VKBUILD
	CommandBufferGFX* __CMDBuffer = g_GPU.acquire_graphical_command_buffer();

	// setup begin draw
	VkRenderPassBeginInfo __RPBeginInfo = {  };
	__RPBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	__RPBeginInfo.renderPass = m_RenderPass;
	__RPBeginInfo.framebuffer = m_Framebuffer;
	__RPBeginInfo.renderArea.offset = { 0,0 };
	__RPBeginInfo.renderArea.extent = g_Frame.swapchain.extent;
	__RPBeginInfo.clearValueCount = components.size();
	__RPBeginInfo.pClearValues = g_Frame.clear_colour;
	vkCmdBeginRenderPass(__CMDBuffer->buffer,&__RPBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

	// viewport setup
	vkCmdSetViewport(__CMDBuffer->buffer,0,1,&g_Frame.viewport);
	vkCmdSetScissor(__CMDBuffer->buffer,0,1,&g_Frame.scissor);
	// FIXME investigate this, it seems like this could be solved with a little more elegance

#else
	glBindFramebuffer(GL_FRAMEBUFFER,m_Buffer);
	Frame::clear();
#endif
}

/**
 *	stop writing to the framebuffer
 */
void Framebuffer::stop()
{
#ifdef VKBUILD
	vkCmdEndRenderPass(g_GPU.acquire_graphical_command_buffer()->buffer);
	// TODO again, using all framebuffers like final render targets does not hold up
	// TODO outsource appropriately to pipeline probably
#else
	glBindFramebuffer(GL_FRAMEBUFFER,0);
#endif
}

/**
 *	bind colour component to a texture channel
 *	\param channel: texture channel
 *	\param i: colour component index of rendertarget
 */
#ifndef VKBUILD
void Framebuffer::bind_colour_component(u8 channel,u8 i)
{
	Texture::set_channel(channel);
	glBindTexture(GL_TEXTURE_2D,components[i]);
}

/**
 *	bind depth component to a texture channel
 *	\param channel: texture channel
 */
void Framebuffer::bind_depth_component(u8 channel)
{
	Texture::set_channel(channel);
	glBindTexture(GL_TEXTURE_2D,components[m_DepthChannel]);
}
#endif
