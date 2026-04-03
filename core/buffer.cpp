#include "buffer.h"


// ----------------------------------------------------------------------------------------------------
// Rendertarget Colour Buffers

/**
 *	TODO
 */
RenderPass::RenderPass(u8 count,bool depth)
	: depth_channel(count),has_depth(depth),result_attachment(count+depth)
{
	// render pass component setup
	u8 __ComponentCount = count+depth;
	VkAttachmentDescription* __Descriptions
			= (VkAttachmentDescription*)malloc(__ComponentCount*sizeof(VkAttachmentDescription));
	VkAttachmentReference* __References
			= (VkAttachmentReference*)malloc(__ComponentCount*sizeof(VkAttachmentReference));
}

/**
 *	TODO
 */
u8 RenderPass::define_colour_component(bool floatbuffer)
{
	COMM_ERR_COND(!(m_Cursor<depth_channel),
				  "colour component definition exceeds allocated range of definable components");
	_define_colour_component(m_Cursor,(floatbuffer) ? g_Formats.floatbuffer : g_Formats.colourbuffer);
	return m_Cursor++;
	// TODO overwrite framebuffer component default resolution given by construction
}

/**
 *	TODO
 */
u8 RenderPass::define_result_component()
{
	COMM_ERR_COND(!(m_Cursor<depth_channel),
				  "result component definition exceeds allocated range of definable components");
	_define_colour_component(m_Cursor,g_Frame.swapchain.format.format);
	result_attachment.set(m_Cursor);
	return m_Cursor++;
}

/**
 *	TODO
 */
void RenderPass::finalize()
{
	COMM_MSG_COND(m_Cursor!=depth_channel,LOG_YELLOW,
				  "render pass definition is called for finalization, but not all components were defined");

	if (has_depth)
	{
		// depth component
		descriptions[depth_channel] = {};
		descriptions[depth_channel].format = g_Formats.depthbuffer;
		descriptions[depth_channel].samples = VK_SAMPLE_COUNT_1_BIT;
		descriptions[depth_channel].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		descriptions[depth_channel].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		descriptions[depth_channel].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		descriptions[depth_channel].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		descriptions[depth_channel].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		descriptions[depth_channel].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// define as depth stencil component
		m_References[depth_channel] = {};
		m_References[depth_channel].attachment = depth_channel;
		m_References[depth_channel].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// specify graphical subpass
	VkSubpassDescription __SubpassDesc = {  };
	__SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	__SubpassDesc.colorAttachmentCount = depth_channel;
	__SubpassDesc.pColorAttachments = m_References;
	__SubpassDesc.pDepthStencilAttachment
			= (has_depth) ? &m_References[depth_channel] : nullptr;

	// subpass dependency
	VkSubpassDependency __SubpassDependency = {  };
	__SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	__SubpassDependency.dstSubpass = 0;
	__SubpassDependency.srcStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	__SubpassDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	__SubpassDependency.dstStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	__SubpassDependency.dstAccessMask
			= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// TODO feature selection based on component setup

	// render pass
	VkRenderPassCreateInfo __RPInfo = {  };
	__RPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	__RPInfo.attachmentCount = depth_channel+has_depth;
	__RPInfo.pAttachments = descriptions;
	__RPInfo.subpassCount = 1;
	__RPInfo.pSubpasses = &__SubpassDesc;
	__RPInfo.dependencyCount = 1;
	__RPInfo.pDependencies = &__SubpassDependency;
	VkResult __Result = vkCreateRenderPass(g_GPU.gpu,&__RPInfo,nullptr,&render_pass);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create render pass");
}

/**
 *	TODO
 */
void RenderPass::vanish()
{
	free(descriptions);
	free(m_References);
	result_attachment.vanish();
	g_GPU.free(render_pass);
}

/**
 *	TODO
 */
void RenderPass::_define_colour_component(u8 index,VkFormat format)
{
	// specify colour component
	descriptions[index] = {};
	descriptions[index].format = format;
	descriptions[index].samples = VK_SAMPLE_COUNT_1_BIT;
	descriptions[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	descriptions[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	descriptions[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	descriptions[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	descriptions[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	descriptions[index].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// specify fragment output location
	m_References[index] = {};
	m_References[index].attachment = index;
	m_References[index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

/**
 *	TODO
 */
void Framebuffer::setup(f32 width,f32 height,RenderPass& rp,s16 result_buffer)
{
	u8 __ComponentCount = rp.depth_channel+rp.has_depth;
	components.resize(__ComponentCount);
#ifdef VKBUILD
	m_RenderPass = &rp;

	// allocate handler memory
	m_AttachmentImages.resize(__ComponentCount);
	m_AttachmentMemory.resize(__ComponentCount);

	for (u8 i=0;i<rp.depth_channel;i++)
	{
		if (rp.result_attachment[i])
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
		__ImageInfo.format = rp.descriptions[i].format;
		__ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		__ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		__ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
		__ImageViewInfo.format = rp.descriptions[i].format;
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
	}
	// FIXME repeated code again, with no concept of sensible abstraction

	// depth component allocation
	if (rp.has_depth)
	{
		// TODO implement depth allocation
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
		__ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		__ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		__ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		__ImageInfo.flags = 0;
		VkResult __Result = vkCreateImage(g_GPU.gpu,&__ImageInfo,nullptr,&m_AttachmentImages[rp.depth_channel]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create depth buffer for some reason");
		// TODO explicitly define different creation functions for depth and pixel buffer. then diversify

		// allocate vram
		VkMemoryRequirements __MemoryRequirements;
		vkGetImageMemoryRequirements(g_GPU.gpu,m_AttachmentImages[rp.depth_channel],&__MemoryRequirements);
		VkMemoryAllocateInfo __MemoryInfo = {  };
		__MemoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		__MemoryInfo.allocationSize = __MemoryRequirements.size;
		__MemoryInfo.memoryTypeIndex = GPU::choose_memory_type(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															   __MemoryRequirements.memoryTypeBits);
		__Result = vkAllocateMemory(g_GPU.gpu,&__MemoryInfo,nullptr,&m_AttachmentMemory[rp.depth_channel]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate VRAM for depth buffer for some reason");
		vkBindImageMemory(g_GPU.gpu,m_AttachmentImages[rp.depth_channel],m_AttachmentMemory[rp.depth_channel],0);
		// FIXME a lot of code repitition, but abstracting this will loose too much functionality?

		// depth buffer image view handle
		VkImageViewCreateInfo __ImageViewInfo = {  };
		__ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		__ImageViewInfo.image = m_AttachmentImages[rp.depth_channel];
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
		__Result = vkCreateImageView(g_GPU.gpu,&__ImageViewInfo,nullptr,&components[rp.depth_channel]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"depth buffer image view creation failed");
		// FIXME another code repetition here, see blitter.cpp. abstract and allow for multiple images by pointer
		// TODO this is much more abstractable, but also not really?
		// TODO allow for independent depth buffer allocation (?in blitter) and bind just result colour buffer
	}

	// create framebuffer
	VkFramebufferCreateInfo __FramebufferInfo = {  };
	__FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	__FramebufferInfo.renderPass = rp.render_pass;
	__FramebufferInfo.attachmentCount = components.size();
	__FramebufferInfo.width = width;
	__FramebufferInfo.height = height;
	__FramebufferInfo.layers = 1;
	__FramebufferInfo.pAttachments = &components[0];
	VkResult __Result = vkCreateFramebuffer(g_GPU.gpu,&__FramebufferInfo,nullptr,&m_Framebuffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create framebuffer");
#else
	glGenFramebuffers(1,&m_Buffer);
	glGenTextures(count,&components[0]);
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
 *	TODO
 */
void Framebuffer::vanish()
{
#ifdef VKBUILD
	// free component handle
	for (u8 i=0;i<components.size();i++)
	{
		g_GPU.free(components[i]);

		// free component memory should it have been allocated by the framebuffer
		if (m_RenderPass->result_attachment[i]) continue;
		g_GPU.free(m_AttachmentMemory[i]);
		g_GPU.free(m_AttachmentImages[i]);
	}

	// kill framebuffer and render pass information
	g_GPU.free(m_Framebuffer);
	//g_GPU.free(render_pass);
	//m_ResultAttachment.vanish();
	// TODO this will lead to an overdefinition of render passes when there is a 1:1 of fb and render pass
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
	__RPBeginInfo.renderPass = m_RenderPass->render_pass;
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
void Framebuffer::bind_colour_component(u8 channel,u8 i)
{
	Texture::set_channel(channel);

#ifdef VKBUILD
	// TODO
#else
	glBindTexture(GL_TEXTURE_2D,m_ColourComponents[i]);
#endif
}

/**
 *	bind depth component to a texture channel
 *	\param channel: texture channel
 */
void Framebuffer::bind_depth_component(u8 channel)
{
	Texture::set_channel(channel);
#ifdef VKBUILD
	// TODO
#else
	glBindTexture(GL_TEXTURE_2D,m_DepthComponent);
#endif
}


// ----------------------------------------------------------------------------------------------------
// Vertex Buffer

/**
 *	TODO
 */
void VertexBuffer::allocate(size_t size,bool indexed)
{
#ifdef VKBUILD

	// generate buffers for host & device memory
	GPU::generate_buffer(vbo,m_Memory,size,
						 VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
						 |VK_BUFFER_USAGE_INDEX_BUFFER_BIT*indexed,
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	GPU::generate_buffer(m_StagingVBO,m_StagingMemory,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkMapMemory(g_GPU.gpu,m_StagingMemory,0,size,0,&m_Data);

	// assemble buffer copy info
	m_BufferCopy.srcOffset = 0;
	m_BufferCopy.dstOffset = 0;
	m_BufferCopy.size = size;

#else
	glGenVertexArrays(1,&m_VAO);
	glGenBuffers(1,&m_VBO);
#endif
}

/**
 *	TODO
 */
void VertexBuffer::upload(void* vertices,size_t vsize)
{
#ifdef VKBUILD
	memcpy(m_Data,vertices,vsize);
#else
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
	glBufferData(GL_ARRAY_BUFFER,vsize,vertices,GL_STATIC_DRAW);
#endif
}
// TODO is it possible to skip this memcpy here and to directly reference, to be able to address in cpu code?
// TODO then also store the threshold for written vertex information, when uploading again it can be amended
// TODO also directly write to certain segments in memory, maybe to overwrite a section of geometry or to
//		update instance information on the fly. there are a lot of interesting use-cases for this.

/**
 *	TODO
 */
void VertexBuffer::upload(void* vertices,size_t vsize,void* indices,size_t isize)
{
#ifdef VKBUILD
	index_offset = vsize;

	// fill staging buffer with vertex information
	memcpy(m_Data,vertices,vsize);
	memcpy((u8*)m_Data+vsize,indices,isize);

#else
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
	glBufferData(GL_ARRAY_BUFFER,vsize,vertices,GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER,isize,indices,GL_STATIC_DRAW);
	// TODO element upload (find out if a size of 0 will be guarded and is well defined) (?on condition?)
#endif
}
// TODO rename to upload and make vertex/element non-specific. ?this should just work without user decision?
// TODO consider a ring buffer system for multi-frame processing

#ifdef VKBUILD

/**
 *	TODO
 */
void VertexBuffer::update()
{
	m_CMDBuffer = g_GPU.acquire_transfer_command_buffer();

	// memory barrier access after last transfer
	VkBufferMemoryBarrier __MemoryBarrier = {  };
	__MemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	__MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	__MemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	__MemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MemoryBarrier.buffer = vbo;
	__MemoryBarrier.offset = 0;
	__MemoryBarrier.size = VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(m_CMDBuffer->buffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,
						 0,nullptr,1,&__MemoryBarrier,0,nullptr);

	// copy buffer
	vkCmdCopyBuffer(m_CMDBuffer->buffer,m_StagingVBO,vbo,1,&m_BufferCopy);

	// memory barrier to access from graphics queue
	__MemoryBarrier.dstAccessMask = 0;
	__MemoryBarrier.srcQueueFamilyIndex = g_GPU.device_info->transfer_queue;
	__MemoryBarrier.dstQueueFamilyIndex = g_GPU.device_info->graphical_queue;
	vkCmdPipelineBarrier(m_CMDBuffer->buffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,
						 0,nullptr,1,&__MemoryBarrier,0,nullptr);
}
// TODO consider using inheritance info to work with secondary command buffers
// FIXME code repetition & unfortunate recurring setup during loop
// TODO in order to prevent gpu-side performance issues reduce the amount of per-frame submits to 1 for uploads
//		parallelism and even multi-passes are not organized through separate submits and command buffers

/**
 *	TODO
 */
void VertexBuffer::free()
{
	vkUnmapMemory(g_GPU.gpu,m_StagingMemory);
	vkWaitForFences(g_GPU.gpu,1,&g_GPU.acquire_transfer_command_buffer()->processing,VK_TRUE,UINT64_MAX);
	g_GPU.free(m_StagingVBO);
	g_GPU.free(m_StagingMemory);
}

/**
 *	TODO
 */
void VertexBuffer::vanish()
{
	g_GPU.free(vbo);
	g_GPU.free(m_Memory);
}

#else

/**
 *	TODO
 */
void VertexBuffer::bind()
{
	glBindVertexArray(m_VAO);
}

#endif


#ifdef VKBUILD

/**
 *	TODO
 */
void VertexArray::allocate(u8 size)
{
	m_Buffers.reserve(size);
	m_Barriers.reserve(size);
	m_Offsets = vector<size_t>(size,0);
}

/**
 *	TODO
 */
inline static VkBufferMemoryBarrier _generate_memory_barrier()
{
	VkBufferMemoryBarrier __Barrier = {  };
	__Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	__Barrier.srcAccessMask = 0;
	__Barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	__Barrier.srcQueueFamilyIndex = g_GPU.device_info->transfer_queue;
	__Barrier.dstQueueFamilyIndex = g_GPU.device_info->graphical_queue;
	__Barrier.offset = 0;
	__Barrier.size = VK_WHOLE_SIZE;
	return __Barrier;
}
// TODO this belongs at the beginning of the file or maybe even in hardware?

/**
 *	TODO
 */
inline void VertexArray::register_buffer(const VertexBuffer& vb)
{
	COMM_MSG_COND(m_Buffers.size()>=m_Buffers.capacity(),LOG_YELLOW,
				  "WARNING: insufficient vertex array allocation. resizing.");
	m_Buffers.push_back(vb.vbo);
}

/**
 *	TODO
 */
void VertexArray::register_buffer_dynamic(const VertexBuffer& vb)
{
	register_buffer(vb);

	// register memory barrier for dynamic upload. this will require to transfer ownership!
	VkBufferMemoryBarrier __Barrier = _generate_memory_barrier();
	__Barrier.buffer = vb.vbo;
	m_Barriers.push_back(__Barrier);
}

/**
 *	TODO
 */
void VertexArray::register_buffer_indexed(const VertexBuffer& vb)
{
	COMM_MSG_COND(m_IndexSource>-1,LOG_YELLOW,"WARNING: a previous buffer has already set the index offset");
	m_IndexSource = m_Buffers.size();
	m_IndexOffset = vb.index_offset;
	register_buffer(vb);
}

/**
 *	TODO
 */
void VertexArray::transfer_ownership()
{
	VkCommandBuffer cmd_buffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	vkCmdPipelineBarrier(cmd_buffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,0,
						 0,nullptr,m_Barriers.size(),&m_Barriers[0],0,nullptr);
}
// FIXME this will just do a barrier for all memory, even though most of it will be only uploaded ONCE
//		maybe only request a barrier when uploading, it will loose out this combined barrier command though

/**
 *	TODO
 */
void VertexArray::bind()
{
	vkCmdBindVertexBuffers(g_GPU.acquire_graphical_command_buffer()->buffer,0,2,&m_Buffers[0],&m_Offsets[0]);
}

/**
 *	TODO
 */
void VertexArray::bind_indexed()
{
	COMM_ERR_COND(m_IndexSource<0,"an indexed bind is requested, but no source was ever defined");
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	vkCmdBindVertexBuffers(__CMDBuffer,0,2,&m_Buffers[0],&m_Offsets[0]);
	vkCmdBindIndexBuffer(__CMDBuffer,m_Buffers[m_IndexSource],m_IndexOffset,VK_INDEX_TYPE_UINT32);
}
// TODO upload sync by semaphore

#endif


// ----------------------------------------------------------------------------------------------------
// Colour Buffers

#ifndef VKBUILD
s32 _texture_format_channels[TEXTURE_FORMAT_COUNT] = {
	GL_RGBA,
	GL_RGBA,
	GL_RED
};

s32 _texture_format_internal[TEXTURE_FORMAT_COUNT] = {
	GL_RGBA,
	GL_SRGB8_ALPHA8,
	GL_RED
};
#endif

/**
 *	allocation and setup for texture data load
 *	\param format: (default TEXTURE_FORMAT_RGBA) texture channel format
 */
TextureData::TextureData(TextureFormat format)
	: m_Format(format)
{  }

/**
 *	make the cpu load the texture data & dimensions from file
 *	\param path: path to texture
 */
void TextureData::load(const char* path)
{
	COMM_ERR_COND(!check_file_exists(path),"texture %s could not be found",path);
	stbi_set_flip_vertically_on_load(true);  // FIXME this belongs in initialization
	data = stbi_load(path,&width,&height,0,STBI_rgb_alpha);
#ifdef VKBUILD
	mipcount = std::floor(std::log2(std::max(width,height)))+1;
#endif
	m_TextureFlag = true;
}

/**
 *	upload data to gpu
 *	NOTE has to be uploaded in main thread
 *	NOTE target texture has to be bound before uploading
 */
void TextureData::gpu_upload()
{
#ifdef VKBUILD
	// TODO

#else
	glTexImage2D(GL_TEXTURE_2D,0,_texture_format_internal[m_Format],width,height,0,
				 _texture_format_channels[m_Format],GL_UNSIGNED_BYTE,data);
#endif
	_free();
}

/**
 *	upload data as subtexture to atlas on gpu based on saved x & y axis offset
 *	NOTE has to be uploaded in main thread
 *	NOTE target texture has to be bound and allocated before uploading
 */
void TextureData::gpu_upload_subtexture()
{
#ifdef VKBUILD
	// TODO

#else
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,width,height,_texture_format_channels[m_Format],GL_UNSIGNED_BYTE,data);
#endif
	_free();
}

/**
 *	free buffer memory
 */
void TextureData::_free()
{
	if (m_TextureFlag) stbi_image_free(data);
	else free(data);
}


/**
 *	setup texture buffer
 */
Texture::Texture()
{
#ifdef VKBUILD
	// TODO

#else
	glGenTextures(1,&m_Memory);
#endif
}

/**
 *	set texture channel
 *	\param i: channel index, correlating to sampler2D integer upload
 */
void Texture::set_channel(u8 i)
{
#ifdef VKBUILD
	// TODO

#else
	glActiveTexture(GL_TEXTURE0+i);
#endif
}
// TODO i'm not sure this is even a thing in the vulkan version? how do we handle that?

/**
 *	bind texture buffer for read and write procedures
 *	\param i: channel index, correlating to sampler2D integer upload
 */
void Texture::bind(u8 i)
{
#ifdef VKBUILD
	// TODO

#else
	set_channel(i);
	glBindTexture(GL_TEXTURE_2D,m_Memory);
#endif
}

/**
 *	release any bound textures
 */
void Texture::unbind()
{
#ifdef VKBUILD
	// TODO

#else
	glBindTexture(GL_TEXTURE_2D,0);
#endif
}

/**
 *	define trilinear texture filter for mipmap generation
 *	NOTE texture should be bound
 *	NOTE this allows generate_mipmap() to be executed if no manual approach is chosen
 */
void Texture::set_texture_parameter_linear_mipmap()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
#endif
}

/**
 *	define bilinear texture filter for mipmap generation
 *	NOTE texture should be bound
 *	NOTE this allows generate_mipmap() to be executed if no manual approach is chosen
 */
void Texture::set_texture_parameter_nearest_mipmap()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
#endif
}

/**
 *	define linear filter without mipmapping
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_linear_unfiltered()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
#endif
}

/**
 *	define filterless pixelraster
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_nearest_unfiltered()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
#endif
}

/**
 *	define extension behaviour to stretch to maximum size
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_clamp_to_edge()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
#endif
}

/**
 *	define extension behaviour to be scaled towards custom borders, avoiding ratio manipulation
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_clamp_to_border()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
#endif
}

/**
 *	repeat texture over vertices without scaling
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_repeat()
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
#endif
}

/**
 *	define level of detail through filter bias (will additively shift the perceived lod value)
 *	\param bias: (default .0f) filter bias, addition bias means that .0f is no bias
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_filter_bias(float bias)
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_LOD_BIAS,bias);
#endif
}

/**
 *	define border colour for texture when set_texture_parameter_clamp_to_border() is defined
 *	\param colour: RGBA border colour as vector
 *	NOTE texture should be bound
 *	NOTE pointer trick not field tested, should border colour fail to work this is the most likely culprit
 */
void Texture::set_texture_parameter_border_colour(vec4 colour)
{
#ifdef VKBUILD
	// TODO

#else
	glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,&colour.r);
#endif
}

/**
 *	automatically generate mipmap when the appropriate texture parameters are set (_mipmap() suffix)
 *	NOTE texture should be bound
 */
void Texture::generate_mipmap()
{
#ifdef VKBUILD
	// TODO

#else
	glGenerateMipmap(GL_TEXTURE_2D);
#endif
}


// ----------------------------------------------------------------------------------------------------
// Pixel Buffer Feature

/**
 *	calculate estimated word length in given font
 *	\param word: given word for length estimation
 *	\param offset: (default 0) wordlength character offset to exclude buffer tail
 */
f32 Font::estimate_wordlength(string& word,u32 offset)
{
	f32 out = .0f;
	for (u32 i=0;i<word.size()-offset;i++) out += glyphs[word[i]-32].advance;
	return out;
}

/**
 *	allocate video memory to use as we, the programmers please
 *	\param width: buffer width
 *	\param height: buffer height
 *	\param format: colourspace format of pixels
 *	NOTE cannot be executed in subthread, uses context bound to main thread
 */
void GPUPixelBuffer::allocate(u32 width,u32 height,TextureFormat format)
{
	// store info
	dimensions_inv = vec2(1.f/width,1.f/height);

	// allocate memory
	memory_segments.push_back({
			.offset = vec2(0,0),
			.dimensions = vec2(width,height)
		});
#ifndef VKBUILD
	glTexImage2D(GL_TEXTURE_2D,0,_texture_format_channels[format],width,height,0,
				 _texture_format_internal[format],GL_UNSIGNED_BYTE,0);
#endif
}
// TODO i don't believe all this schnickschnack is necessary for the vulkan version at all.
//		vulkan allows for a way more direct malloc procedure, this might be the biggest discrepancy in the vers


#ifdef VKBUILD

// §§prototyping
void GPUPixelBuffer::load_texture(const char* path)
{
	// load texture data
	TextureData __TextureData;
	__TextureData.load(path);
	size_t __ImageSize = __TextureData.width*__TextureData.height*4;

	// image buffer
	GPU::generate_buffer(m_StagingBuffer,m_StagingMemory,__ImageSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkImageCreateInfo __ImageInfo = {  };
	__ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	__ImageInfo.flags = 0;
	__ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	__ImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	__ImageInfo.extent.width = __TextureData.width;
	__ImageInfo.extent.height = __TextureData.height;
	__ImageInfo.extent.depth = 1;
	__ImageInfo.mipLevels = __TextureData.mipcount;
	__ImageInfo.arrayLayers = 1;
	__ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	__ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	__ImageInfo.usage
			= VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	__ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	__ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkResult __Result = vkCreateImage(g_GPU.gpu,&__ImageInfo,nullptr,&m_Texture);
	// TODO as a possible late-stage optimization look into image unionization for multiple image destinations
	//		utilizing the same memory by setting an initialLayout and an alias
	//		this will probably just be useful for rendertarget results that do not overlap in timing

	/**
	 *	TODO so basically to port the new streaming system:
	 *		the vulkan system will also stream to a dynamic texture atlas, to ensure backwards compatibility
	 *		then areas are batched together by theme and streamed into memory on demand like in the ogl version
	 *		-> making all this run on very low-end systems, while being fast on modern systems
	 *		data streaming throttle cannot be decided by upload time and skipped on the fly like in ogl
	 *		-> the command buffer submission is uploaded once and unchangeable and will work independetly
	 *		-> this is very nice but makes it impossible to skip based on live frame data
	 *		-> gpu uploads will be throttled by data size instead.
	 *		-> initial load will measure base data throughput values to decide streaming capabilities
	 *		-> then when streaming data the value will be updated and modified to dynamically throttly
	 *	FIXME this also poses the question how to optimize dynamic atlasses for low-vram systems (case T450)
	 */

	// memory
	VkMemoryRequirements __MemoryRequirements;
	vkGetImageMemoryRequirements(g_GPU.gpu,m_Texture,&__MemoryRequirements);
	VkMemoryAllocateInfo __MemoryInfo = {  };
	__MemoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	__MemoryInfo.allocationSize = __MemoryRequirements.size;
	__MemoryInfo.memoryTypeIndex = GPU::choose_memory_type(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
														   __MemoryRequirements.memoryTypeBits);
	__Result = vkAllocateMemory(g_GPU.gpu,&__MemoryInfo,nullptr,&m_TextureMemory);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate VRAM for texture for some reason");
	vkBindImageMemory(g_GPU.gpu,m_Texture,m_TextureMemory,0);

	// map texture data
	void* __Data;
	vkMapMemory(g_GPU.gpu,m_StagingMemory,0,__ImageSize,0,&__Data);
	memcpy(__Data,__TextureData.data,__ImageSize);
	vkUnmapMemory(g_GPU.gpu,m_StagingMemory);

	// setup memory barrier
	VkImageMemoryBarrier __Barrier = {  };
	__Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	__Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__Barrier.image = m_Texture;
	__Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__Barrier.subresourceRange.baseMipLevel = 0;
	__Barrier.subresourceRange.levelCount = __TextureData.mipcount;
	__Barrier.subresourceRange.baseArrayLayer = 0;
	__Barrier.subresourceRange.layerCount = 1;
	__Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	__Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	__Barrier.srcAccessMask = 0;
	__Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	// buffer copy
	VkBufferImageCopy __BufferCopy = {  };
	__BufferCopy.bufferOffset = 0;
	__BufferCopy.bufferRowLength = 0;
	__BufferCopy.bufferImageHeight = 0;
	__BufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__BufferCopy.imageSubresource.mipLevel = 0;
	__BufferCopy.imageSubresource.baseArrayLayer = 0;
	__BufferCopy.imageSubresource.layerCount = 1;
	__BufferCopy.imageOffset = { 0,0,0 };
	__BufferCopy.imageExtent = { (u32)__TextureData.width,(u32)__TextureData.height,1 };

	// test for blitting support based on image format
#ifdef DEBUG
	VkFormatProperties __FormatProperties;
	vkGetPhysicalDeviceFormatProperties(g_GPU.device_info->gpu,VK_FORMAT_R8G8B8A8_SRGB,&__FormatProperties);
	COMM_ERR_COND(!(__FormatProperties.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT),
				  "texture format does not support blitting for mipmap generation purposes");
	// TODO and then maybe do something about it outside of debug cases... we are in trouble should this happen
	//		this is not a problem should the mip levels be pre-processed in addition to improved load times
#endif

	// mipmap generation
	VkImageMemoryBarrier __MMBarrier = {  };
	__MMBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	__MMBarrier.image = m_Texture;
	__MMBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MMBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MMBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__MMBarrier.subresourceRange.baseArrayLayer = 0;
	__MMBarrier.subresourceRange.layerCount = 1;
	__MMBarrier.subresourceRange.levelCount = 1;

	// mipmap blit
	VkImageBlit __MMBlit = {  };
	__MMBlit.srcOffsets[0] = { 0,0,0 };
	__MMBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__MMBlit.srcSubresource.baseArrayLayer = 0;
	__MMBlit.srcSubresource.layerCount = 1;
	__MMBlit.dstOffsets[0] = { 0,0,0 };
	__MMBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__MMBlit.dstSubresource.baseArrayLayer = 0;
	__MMBlit.dstSubresource.layerCount = 1;
	// TODO maybe move this to texture preprocessing and skip the blitting at load time

	// upload image
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0,0,nullptr,0,nullptr,1,&__Barrier);
	vkCmdCopyBufferToImage(__CMDBuffer,m_StagingBuffer,m_Texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1,&__BufferCopy);

	// generate mipmaps
	s32 __MMWidth = __TextureData.width;
	s32 __MMHeight = __TextureData.height;
	for (u16 i=1;i<__TextureData.mipcount;i++)
	{
		// memory barrier blit
		__MMBarrier.subresourceRange.baseMipLevel = i-1;
		__MMBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		__MMBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		__MMBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		__MMBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
							 0,0,nullptr,0,nullptr,1,&__MMBarrier);

		// blitting
		__MMBlit.srcOffsets[1] = { __MMWidth,__MMHeight,1 };
		__MMBlit.srcSubresource.mipLevel = i-1;
		__MMWidth = (__MMWidth>1)?__MMWidth>>1:1;
		__MMHeight = (__MMHeight>1)?__MMHeight>>1:1;
		__MMBlit.dstOffsets[1] = { __MMWidth,__MMHeight,1 };
		__MMBlit.dstSubresource.mipLevel = i;
		vkCmdBlitImage(__CMDBuffer,m_Texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   m_Texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&__MMBlit,VK_FILTER_LINEAR);

		// memory barrier to read after blit
		__MMBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		__MMBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		__MMBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		__MMBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							 0,0,nullptr,0,nullptr,1,&__MMBarrier);
	}

	// seal final blit
	__MMBarrier.subresourceRange.baseMipLevel = __TextureData.mipcount-1;
	__MMBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	__MMBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	__MMBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	__MMBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 0,0,nullptr,0,nullptr,1,&__MMBarrier);

	// cleanup staging memory
	__TextureData.gpu_upload();  // TODO this is only to trigger the memfree, this will be removed later.

	// image view
	VkImageViewCreateInfo __ImageViewInfo = {  };
	__ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	__ImageViewInfo.image = m_Texture;
	__ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	__ImageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;  // TODO check
	__ImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__ImageViewInfo.subresourceRange.baseMipLevel = 0;
	__ImageViewInfo.subresourceRange.levelCount = __TextureData.mipcount;
	__ImageViewInfo.subresourceRange.baseArrayLayer = 0;
	__ImageViewInfo.subresourceRange.layerCount = 1;
	__ImageViewInfo.components = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY,
	};
	__Result = vkCreateImageView(g_GPU.gpu,&__ImageViewInfo,nullptr,&image_view);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"image view creation failed");
	// FIXME code repitition here, see blitter.cpp. abstract and allow for multiple images by pointer

	// texture sampler
	// decided against custom border colour extensions, due to missing reasons for higher support complexity
	VkSamplerCreateInfo __SamplerInfo = {  };
	__SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	__SamplerInfo.magFilter = VK_FILTER_LINEAR;
	__SamplerInfo.minFilter = VK_FILTER_LINEAR;
	__SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	__SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	__SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	__SamplerInfo.anisotropyEnable = !!(g_GPU.device_info->supported&GPU_FEATURE_SUPPORT_ANISOTROPY);
	__SamplerInfo.maxAnisotropy = g_GPU.device_info->properties.limits.maxSamplerAnisotropy;
	__SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	__SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	// TODO research, this is an interesting feature. unfortunately only works with nearest
	__SamplerInfo.compareEnable = VK_FALSE;
	__SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	__SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	__SamplerInfo.mipLodBias = .0f;
	__SamplerInfo.minLod = 0;
	__SamplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	__Result = vkCreateSampler(g_GPU.gpu,&__SamplerInfo,nullptr,&sampler);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"texture sampler creation failed");
	// TODO this will be the texture settings++ from ogl version
}
// TODO put this back into the multithreading texture load system & also use vulcanous advantages
// TODO choose how texture streaming will be done in the future utilizing vulkan (prealloc like in ogl?)

/**
 *	TODO
 */
void GPUPixelBuffer::vanish()
{
	g_GPU.free(m_StagingBuffer);
	g_GPU.free(m_StagingMemory);
	// TODO do the staging removal right after upload?
	g_GPU.free(sampler);
	g_GPU.free(image_view);
	g_GPU.free(m_Texture);
	g_GPU.free(m_TextureMemory);
}

#endif


/**
 *	load texture from path and finally upload to gpu memory
 *	\param gpb: target pixel buffer
 *	\param pbc: pointer to atlas component information, this will be overwritten
 *	\param path: path to texture file
 *	NOTE this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
void GPUPixelBuffer::load_texture(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,const char* path)
{
	TextureData __TextureData;
	__TextureData.load(path);
	gpb->signal.proceed();
	_load(gpb,pbc,&__TextureData);
}

/**
 *	load font data and finally upload to gpu memory
 *	\param gpb: target pixel buffer
 *	\param font: pointer to target font memory
 *	\param path: path to font file
 *	\param size: target rasterization size of vector font
 *	NOTE this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
void GPUPixelBuffer::load_font(GPUPixelBuffer* gpb,Font* font,const char* path,u16 size)
{
	font->size = size;

	// load ttf file
	FT_Face __Face;
	bool _failed = FT_New_Face(g_FreetypeLibrary,path,0,&__Face);
	COMM_ERR_COND(_failed,"font loading unsuccessful");
	FT_Set_Pixel_Sizes(__Face,0,size);

	// iterate font glyphs
	for (u8 i=0;i<96;i++)
	{
		// rasterize glyph
		_failed = FT_Load_Char(__Face,i+32,FT_LOAD_RENDER);
		COMM_ERR_COND(_failed,"rasterization of character %c failed",(char)i+32);

		// subtexture attributes
		TextureData __TextureData = TextureData(TEXTURE_FORMAT_MONOCHROME);
		__TextureData.width = __Face->glyph->bitmap.width;
		__TextureData.height = __Face->glyph->bitmap.rows;

		// glyph attributes
		font->glyphs[i] = {
			.scale = vec2(__Face->glyph->bitmap.width,__Face->glyph->bitmap.rows),
			.bearing = vec2(__Face->glyph->bitmap_left,__Face->glyph->bitmap_top),
			.advance = (__Face->glyph->advance.x>>6)
		};

		// upload glyph as texture buffer
		size_t __Mem = __Face->glyph->bitmap.pitch*__Face->glyph->bitmap.rows;
		__TextureData.data = (u8*)malloc(__Mem);
		memcpy(__TextureData.data,__Face->glyph->bitmap.buffer,__Mem);
		_load(gpb,&font->tex[i],&__TextureData);
	}
	gpb->signal.proceed();

	// store & clear
	FT_Done_Face(__Face);
}

/**
 *	write texture buffer to preallocated gpu memory
 *	\param gpb: target pixel buffer
 *	\param pbc: pointer to atlas component information, this will be overwritten
 *	\param data: texture data
 *	NOTE this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
void GPUPixelBuffer::_load(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,TextureData* data)
{
	// locate best position for texture on free memory space
	f32 __BestDifference = 0x7f800000;
	u32 __MemoryIndex = -1;
	for (u32 i=0;i<gpb->memory_segments.size();i++)
	{
		PixelBufferComponent* p_FreeComponent = &gpb->memory_segments[i];
		if (data->width>p_FreeComponent->dimensions.x
			||data->height>p_FreeComponent->dimensions.y) continue;

		// find closest fit
		f32 __AreaDifference = p_FreeComponent->dimensions.x*p_FreeComponent->dimensions.y
				- data->width*data->height;
		if (__AreaDifference<__BestDifference)
		{
			__MemoryIndex = i;
			__BestDifference = __AreaDifference;
		}
	}

	// get memory segment pointer
	COMM_ERR_COND(__MemoryIndex==-1,"sprite texture memory is populated or segmented. texture upload failed!");
	COMM_MSG_COND(__MemoryIndex==-1,LOG_CYAN,"attempted load dimensions -> (%i,%i)",data->width,data->height);
	PixelBufferComponent* p_CloseFitComponent = &gpb->memory_segments[__MemoryIndex];

	// write atlas information
	data->x = p_CloseFitComponent->offset.x, data->y = p_CloseFitComponent->offset.y;
	pbc->offset = p_CloseFitComponent->offset*gpb->dimensions_inv;
	pbc->dimensions = vec2(data->width,data->height)*gpb->dimensions_inv;

	// segment free memory to reserve pixel space for upload
	s32 __PaddedWidth = data->width+BUFFER_ATLAS_BORDER_PADDING;
	s32 __PaddedHeight = data->height+BUFFER_ATLAS_BORDER_PADDING;
	PixelBufferComponent __Side = {
		.offset = p_CloseFitComponent->offset+vec2(__PaddedWidth,0),
		.dimensions = vec2(p_CloseFitComponent->dimensions.x-__PaddedWidth,__PaddedHeight)
	};
	PixelBufferComponent __Below = {
		.offset = p_CloseFitComponent->offset+vec2(0,__PaddedHeight),
		.dimensions = p_CloseFitComponent->dimensions-vec2(0,__PaddedHeight)
	};
	// FIXME this is segmenting falsely, it's not possible to insert into texture space that has the correct
	//		dimensions in only one segment but crosses over into a different free rect.
	//		alternatively this can be done by assigning cross segment in both subsequent segments, but
	//		this will mess with memory information, due to multiple free states per pixel. geez louize
	//		solve this with a consistent merger algorithm after every segment?

	// update memory information data
	gpb->mutex_memory_segments.lock();
	gpb->memory_segments.erase(gpb->memory_segments.begin()+__MemoryIndex);
	if (__Side.dimensions.x>0) gpb->memory_segments.push_back(__Side);
	if (__Below.dimensions.y>0) gpb->memory_segments.push_back(__Below);
	gpb->mutex_memory_segments.unlock();
	// TODO when deleting and segmenting, check if free subspaces can be merged back into each other

	// write buffer
	gpb->mutex_texture_requests.lock();
	gpb->load_requests.push(*data);
	gpb->mutex_texture_requests.unlock();
}

/**
 *	automatically uploads the loaded subtextures to the gpu
 *	\param channel: texture channel
 *	NOTE this has to be run in main thread due to the gpu upload being context sensitive
 */
void GPUPixelBuffer::gpu_upload(u8 channel)
{
	atlas.bind(channel);
	mutex_texture_requests.lock();

	// iterate waiting requests
	while (load_requests.size()&&calculate_delta_time_ms(g_Frame.fstart)<FRAME_TIME_BUDGET_MS)
	{
		TextureData& p_Data = load_requests.front();
		p_Data.gpu_upload_subtexture();
		load_requests.pop();
	}
	COMM_LOG_COND(load_requests.size(),"stalling upload in pixel buffer");

	// controversial pixel buffer lod creation
	mutex_texture_requests.unlock();
	Texture::generate_mipmap();
}
// FIXME performance will suffer when generating mipmap every time the loop condition breaks


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

#ifdef VKBUILD

/**
 *	TODO
 */
UniformBuffer::UniformBuffer(u32 binding_count)
{
	m_PSizes.reserve(binding_count);
	m_Bindings.reserve(binding_count);
	m_Writes.reserve(binding_count);
	m_DescriptorInfos.reserve(binding_count);
	// TODO those can be free'd after setup has finished
}

/**
 *	TODO
 */
void UniformBuffer::define(u32 location,size_t size)
{
	COMM_MSG_COND(m_Bindings.capacity()<=m_Bindings.size(),LOG_YELLOW,
				  "uniform buffer binding malloc not sufficient, resizing (capacity>%ld)...",m_Bindings.size());

	// descriptor pool size
	VkDescriptorPoolSize __PSize = {  };
	__PSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__PSize.descriptorCount = GPU_BUFFER_COUNT;
	m_PSizes.push_back(__PSize);

	// bindings
	VkDescriptorSetLayoutBinding __Binding = {  };
	__Binding.binding = location;
	__Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__Binding.descriptorCount = 1;  // TODO allow for multiple definitions at the same time? careful! sizeing!
	__Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // TODO dynamics. what for? just to be sure i guess.
	__Binding.pImmutableSamplers = nullptr;  // TODO research
	m_Bindings.push_back(__Binding);
	// TODO debug level map if location is a duplicate to easily check development time mismatch!
	// TODO research if there is more to binding index than reference? maybe performance downsides to splits?

	// buffer info
	DescriptorInfo __Desc = {  };
	__Desc.type = DESCRIPTOR_TYPE_BUFFER;
	__Desc.info.buffer = {  };
	__Desc.info.buffer.offset = m_Size;
	__Desc.info.buffer.range = size;
	m_DescriptorInfos.push_back(__Desc);
	m_Size += size;

	// write descriptors
	VkWriteDescriptorSet __WriteDescriptor = {  };
	__WriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	__WriteDescriptor.dstBinding = location;
	__WriteDescriptor.dstArrayElement = 0;  // TODO research
	__WriteDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__WriteDescriptor.descriptorCount = 1;
	m_Writes.push_back(__WriteDescriptor);
}

/**
 *	TODO
 */
void UniformBuffer::define(u32 location,GPUPixelBuffer& texture)
{
	COMM_MSG_COND(m_Bindings.capacity()<=m_Bindings.size(),LOG_YELLOW,
				  "sampler binding malloc not sufficient, resizing (capacity>%ld)...",m_Bindings.size());

	// descriptor pool size
	VkDescriptorPoolSize __PSize = {  };
	__PSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	__PSize.descriptorCount = GPU_BUFFER_COUNT;
	m_PSizes.push_back(__PSize);

	// bindings
	VkDescriptorSetLayoutBinding __Binding = {  };
	__Binding.binding = location;
	__Binding.descriptorCount = 1;
	__Binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	__Binding.pImmutableSamplers = nullptr;
	__Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_Bindings.push_back(__Binding);
	// TODO solve the same things as in other definition implementation (also fragment bit e.g. height manip)

	// image info
	DescriptorInfo __Desc = {  };
	__Desc.type = DESCRIPTOR_TYPE_IMAGE;
	__Desc.info.image = {  };
	__Desc.info.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	__Desc.info.image.imageView = texture.image_view;
	__Desc.info.image.sampler = texture.sampler;
	m_DescriptorInfos.push_back(__Desc);

	// write descriptors
	VkWriteDescriptorSet __WriteDescriptor = {  };
	__WriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	__WriteDescriptor.dstBinding = location;
	__WriteDescriptor.dstArrayElement = 0;
	__WriteDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	__WriteDescriptor.descriptorCount = 1;
	m_Writes.push_back(__WriteDescriptor);
}

/**
 *	TODO
 */
void UniformBuffer::define(u32 location,VkImageView buffer)
{
	COMM_MSG_COND(m_Bindings.capacity()<=m_Bindings.size(),LOG_YELLOW,
				  "subpass result binding malloc not sufficient, resizing (capacity>%ld)...",m_Bindings.size());

	// descriptor pool size
	VkDescriptorPoolSize __PSize = {  };
	__PSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	__PSize.descriptorCount = GPU_BUFFER_COUNT;
	m_PSizes.push_back(__PSize);

	// bindings
	VkDescriptorSetLayoutBinding __Binding = {  };
	__Binding.binding = location;
	__Binding.descriptorCount = 1;
	__Binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	__Binding.pImmutableSamplers = nullptr;
	__Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_Bindings.push_back(__Binding);
	// TODO solve the same things as in other definition implementation (also fragment bit e.g. height manip)

	// image info
	DescriptorInfo __Desc = {  };
	__Desc.type = DESCRIPTOR_TYPE_IMAGE;
	__Desc.info.image = {  };
	__Desc.info.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	__Desc.info.image.imageView = buffer;
	__Desc.info.image.sampler = VK_NULL_HANDLE;
	m_DescriptorInfos.push_back(__Desc);

	// write descriptors
	VkWriteDescriptorSet __WriteDescriptor = {  };
	__WriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	__WriteDescriptor.dstBinding = location;
	__WriteDescriptor.dstArrayElement = 0;
	__WriteDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	__WriteDescriptor.descriptorCount = 1;
	m_Writes.push_back(__WriteDescriptor);
}

/**
 *	TODO
 */
void UniformBuffer::assemble()
{
	COMM_AWT("allocating the uniform buffer");

	// generate buffer
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		GPU::generate_buffer(m_UBO[i],m_UBOMemory[i],m_Size,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkMapMemory(g_GPU.gpu,m_UBOMemory[i],0,m_Size,0,&m_UBOMapped[i]);
	}
	// TODO stage this too? host_visible? i don't think so bröther

	// descriptor pool creation
	VkDescriptorPoolCreateInfo __DPoolInfo = {  };
	__DPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	__DPoolInfo.poolSizeCount = m_PSizes.size();
	__DPoolInfo.pPoolSizes = &m_PSizes[0];
	__DPoolInfo.maxSets = GPU_BUFFER_COUNT;
	__DPoolInfo.flags = 0;
	VkResult __Result = vkCreateDescriptorPool(g_GPU.gpu,&__DPoolInfo,nullptr,&m_DescriptorPool);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate driver descriptor pool");

	// uniform layout
	VkDescriptorSetLayoutCreateInfo __LayoutInfo = {  };
	__LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	__LayoutInfo.bindingCount = m_Bindings.size();
	__LayoutInfo.pBindings = &m_Bindings[0];
	__Result = vkCreateDescriptorSetLayout(g_GPU.gpu,&__LayoutInfo,nullptr,&m_DSetLayout);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"uniform layout definition failed");

	// descriptor sets
	vector<VkDescriptorSetLayout> __DSetLayouts(GPU_BUFFER_COUNT,m_DSetLayout);
	VkDescriptorSetAllocateInfo __DSetAllocInfo = {  };
	__DSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	__DSetAllocInfo.descriptorPool = m_DescriptorPool;
	__DSetAllocInfo.descriptorSetCount = GPU_BUFFER_COUNT;
	__DSetAllocInfo.pSetLayouts = &__DSetLayouts[0];
	__Result = vkAllocateDescriptorSets(g_GPU.gpu,&__DSetAllocInfo,m_DSets);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate descriptor set memory");

	// descriptors
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		for (size_t j=0;j<m_Writes.size();j++)
		{
			m_Writes[j].dstSet = m_DSets[i];
			switch (m_DescriptorInfos[j].type)
			{
			case DESCRIPTOR_TYPE_BUFFER:
				m_DescriptorInfos[j].info.buffer.buffer = m_UBO[i];
				m_Writes[j].pBufferInfo = &m_DescriptorInfos[j].info.buffer;
				break;
			case DESCRIPTOR_TYPE_IMAGE: m_Writes[j].pImageInfo = &m_DescriptorInfos[j].info.image;
				break;
			}
			// FIXME this buffer shenanigans is very funny, but let's not actually do this in the final solution
		}
		vkUpdateDescriptorSets(g_GPU.gpu,m_Writes.size(),&m_Writes[0],0,nullptr);
	}

	COMM_CNF();
}

/**
 *	TODO
 *	TODO add an offset to allow for bundling later (or maybe just push constants? research!)
 */
void UniformBuffer::update(void* data,size_t size)
{
	memcpy(m_UBOMapped[g_GPU.active_buffer],data,size);
}
// FIXME isn't g_GPU.active_buffer the next buffer from the currently selected one (referencing in hardware.h)

/**
 *	TODO
 */
void UniformBuffer::vanish()
{
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		g_GPU.free(m_UBO[i]);
		g_GPU.free(m_UBOMemory[i]);
	}
	g_GPU.free(m_DescriptorPool);
	g_GPU.free(m_DSetLayout);
}
// TODO maybe this buffer needs to be moved to shader.h instead, being closely related to it's features

#endif
