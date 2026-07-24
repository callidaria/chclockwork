#include "memory.h"


// ----------------------------------------------------------------------------------------------------
// Memory Barriers

/**
 *	TODO
 */
inline static VkBufferMemoryBarrier _generate_memory_barrier_bfr()
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

/**
 *	TODO
 */
inline static VkImageMemoryBarrier _generate_memory_barrier_tex(VkImage tex,u32 mip)
{
	VkImageMemoryBarrier __Barrier = {  };
	__Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	__Barrier.image = tex;
	__Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__Barrier.subresourceRange.baseArrayLayer = 0;
	__Barrier.subresourceRange.baseMipLevel = 0;
	__Barrier.subresourceRange.layerCount = 1;
	__Barrier.subresourceRange.levelCount = mip;
	return __Barrier;
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
void VertexBuffer::upload(void* vertices,size_t vsize,size_t ofs)
{
#ifdef VKBUILD
	memcpy((void*)((char*)m_Data+ofs),vertices,vsize);
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
inline void VertexArray::register_buffer(const VertexBuffer& vb)
{
	COMM_ERR_COND(m_Buffers.size()>=m_Buffers.capacity(),"insufficient vertex array allocation");
	m_Buffers.push_back(vb.vbo);
}

/**
 *	TODO
 */
void VertexArray::register_buffer_dynamic(const VertexBuffer& vb)
{
	register_buffer(vb);

	// register memory barrier for dynamic upload. this will require to transfer ownership!
	VkBufferMemoryBarrier __Barrier = _generate_memory_barrier_bfr();
	__Barrier.buffer = vb.vbo;
	m_Barriers.push_back(__Barrier);
}

/**
 *	TODO
 */
void VertexArray::register_buffer_indexed(const VertexBuffer& vb)
{
	COMM_MSG_COND(m_IndexSource>-1,LOG_YELLOW,"WARNING: a previous buffer has already set the index component");
	m_IndexSource = m_Buffers.size();
	m_IndexOffset = vb.index_offset;
	register_buffer(vb);
}

/**
 *	TODO
 */
void VertexArray::transfer_ownership_read()
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
void VertexArray::transfer_ownership_write()
{
	VkCommandBuffer cmd_buffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	vkCmdPipelineBarrier(cmd_buffer,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,0,
						 0,nullptr,m_Barriers.size(),&m_Barriers[0],0,nullptr);
}

/**
 *	TODO
 */
void VertexArray::bind()
{
	vkCmdBindVertexBuffers(g_GPU.acquire_graphical_command_buffer()->buffer,0,m_Buffers.size(),
						   &m_Buffers[0],&m_Offsets[0]);
}

/**
 *	TODO
 */
void VertexArray::bind_indexed()
{
	COMM_ERR_COND(m_IndexSource<0,"an indexed bind is requested, but no source was ever defined");
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	vkCmdBindVertexBuffers(__CMDBuffer,0,m_Buffers.size(),&m_Buffers[0],&m_Offsets[0]);
	vkCmdBindIndexBuffer(__CMDBuffer,m_Buffers[m_IndexSource],m_IndexOffset,VK_INDEX_TYPE_UINT32);
}
// TODO upload sync by semaphore

#endif


// ----------------------------------------------------------------------------------------------------
// Colour Buffers

// texture format correlation
struct TextureFormatTuple
{
	__texture_format format;
	u8 size;
};

#ifdef VKBUILD
TextureFormatTuple _texture_formats[TEXTURE_FORMAT_COUNT] = {
	{ VK_FORMAT_R8G8B8A8_UNORM,4 },
	{ VK_FORMAT_R8G8B8A8_SRGB,4 },
	{ VK_FORMAT_R8_UNORM,1 }
};

#else
TextureFormatTuple _texture_formats[TEXTURE_FORMAT_COUNT] = {
	{ GL_RGBA,4 },
	{ GL_RGBA,4 },
	{ GL_RED,1 }
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
	stbi_set_flip_vertically_on_load(true);
	data = stbi_load(path,&width,&height,0,STBI_rgb_alpha);
	m_TextureFlag = true;
}
// FIXME that stbi_set_flip_vertically_on_load call here is due to my build setup, but should be at init
//		the annoyance is pointing to a much bigger problem though, the entire stbi is reprocessed per tu
//		this increases compile time for files that do not necessarily need it and also leads to bad behaviour.
//		additionally there is the problem that with the current setup all userscripts are receiving renderer
//		and by extension must also process the stbi header. this cannot be the right solution!

/**
 *	upload data to gpu
 *	NOTE has to be uploaded in main thread
 *	NOTE target texture has to be bound before uploading
 */
void TextureData::gpu_upload(
#ifdef VKBUILD
		VkImage image,VkBuffer buf,VkDeviceMemory mem
#endif
	)
{
#ifdef VKBUILD
	_copy_buffer(image,buf,mem,0);
#else
	glTexImage2D(GL_TEXTURE_2D,0,_texture_format_internal[m_Format],width,height,0,
				 _texture_formats[m_Format].format,GL_UNSIGNED_BYTE,data);
#endif
	_free();
}

/**
 *	upload data as subtexture to atlas on gpu based on saved x & y axis offset
 *	NOTE has to be uploaded in main thread
 *	NOTE target texture has to be bound and allocated before uploading
 */
void TextureData::gpu_upload_subtexture(
#ifdef VKBUILD
		VkImage image,VkBuffer buf,VkDeviceMemory mem,size_t ofs
#endif
	)
{
#ifdef VKBUILD
	_copy_buffer(image,buf,mem,ofs);
#else
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,width,height,_texture_formats[m_Format].format,GL_UNSIGNED_BYTE,data);
#endif
	_free();
}

/**
 *	TODO
 */
#ifdef VKBUILD
void TextureData::_copy_buffer(VkImage image,VkBuffer buf,VkDeviceMemory mem,size_t ofs)
{
	if (!width||!height)
	{
		COMM_ERR("buffer without width or height has been submitted");
		return;
	}
	size_t __ImageSize = width*height*_texture_formats[m_Format].size;

	// stage memory
	void* __Data;
	vkMapMemory(g_GPU.gpu,mem,ofs,__ImageSize,0,&__Data);
	memcpy(__Data,data,__ImageSize);

	// buffer copy
	VkBufferImageCopy __BufferCopy = {  };
	__BufferCopy.bufferOffset = ofs;
	__BufferCopy.bufferRowLength = 0;
	__BufferCopy.bufferImageHeight = 0;
	__BufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__BufferCopy.imageSubresource.mipLevel = 0;
	__BufferCopy.imageSubresource.baseArrayLayer = 0;
	__BufferCopy.imageSubresource.layerCount = 1;
	__BufferCopy.imageOffset = { (s32)x,(s32)y,0 };
	__BufferCopy.imageExtent = { (u32)width,(u32)height,1 };

	// upload image
	vkCmdCopyBufferToImage(g_GPU.acquire_graphical_command_buffer()->buffer,
						   buf,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&__BufferCopy);
	vkUnmapMemory(g_GPU.gpu,mem);
}
#endif

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
#ifndef VKBUILD
void Texture::set_channel(u8 i)
{
	glActiveTexture(GL_TEXTURE0+i);
}

/**
 *	bind texture buffer for read and write procedures
 *	\param i: channel index, correlating to sampler2D integer upload
 */
void Texture::bind(u8 i)
{
	set_channel(i);
	glBindTexture(GL_TEXTURE_2D,m_Memory);
}

/**
 *	release any bound textures
 */
void Texture::unbind()
{
	glBindTexture(GL_TEXTURE_2D,0);
}
#endif

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
 *	\param padding: (default 0) pixel space padding in between allocated subtextures
 *	NOTE cannot be executed in subthread, uses context bound to main thread
 */
void GPUPixelBuffer::allocate(u32 width,u32 height,TextureFormat format,u32 padding)
{
	// store info
	m_Format = format;
	dimensions_inv = vec2(1.f/width,1.f/height);
	subtex_padding = padding;

	// mark initial, untouched allocated memory segment
	memory_segments.push_back({ .extent = vec2(width+padding,height+padding) });

#ifdef VKBUILD
	m_Width = width;
	m_Height = height;
	m_Mipcount = std::floor(std::log2(std::max(width,height)))+1;

	// generate staging buffer
	GPU::generate_buffer(m_StagingBuffer,m_StagingMemory,
						 width*height*_texture_formats[format].size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// image buffer
	VkImageCreateInfo __ImageInfo = {  };
	__ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	__ImageInfo.flags = 0;
	__ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	__ImageInfo.format = _texture_formats[m_Format].format;
	__ImageInfo.extent.width = width;
	__ImageInfo.extent.height = height;
	__ImageInfo.extent.depth = 1;
	__ImageInfo.mipLevels = m_Mipcount;
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
	// FIXME image size relies on a 4-channel format, which is not always the case (rgb, greyscale)

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
	 *		-> then when streaming data the value will be updated and modified to dynamically throttle
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

	// test for blitting support based on image format
#ifdef DEBUG
	VkFormatProperties __FormatProperties;
	vkGetPhysicalDeviceFormatProperties(g_GPU.device_info->gpu,_texture_formats[m_Format].format,
										&__FormatProperties);
	COMM_ERR_COND(!(__FormatProperties.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT),
				  "texture format does not support blitting for mipmap generation purposes");
	// TODO and then maybe do something about it outside of debug cases... we are in trouble should this happen
	//		this is not a problem should the mip levels be pre-processed in addition to improved load times
	// FIXME this will be printed over and over again. do this once, when selecting the gpu and later even
	//		use this aspect to evaluate gpu capability for automatic selection
#endif

	// setup memory barrier to create image view
	VkImageMemoryBarrier __Barrier = _generate_memory_barrier_tex(m_Texture,m_Mipcount);
	__Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	__Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	__Barrier.srcAccessMask = 0;
	__Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(g_GPU.acquire_graphical_command_buffer()->buffer,
						 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0,0,nullptr,0,nullptr,1,&__Barrier);

	// image view
	VkImageViewCreateInfo __ImageViewInfo = {  };
	__ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	__ImageViewInfo.image = m_Texture;
	__ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	__ImageViewInfo.format = _texture_formats[m_Format].format;
	__ImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__ImageViewInfo.subresourceRange.baseMipLevel = 0;
	__ImageViewInfo.subresourceRange.levelCount = m_Mipcount;
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

#else
	// generate buffer
	glTexImage2D(GL_TEXTURE_2D,0,_texture_formats[format].format,width,height,0,
				 _texture_format_internal[format],GL_UNSIGNED_BYTE,0);
#endif
}
// TODO use mip definition in allocation for ogl version as well

/**
 *	TODO
 */
void GPUPixelBuffer::vanish()
{
#ifdef VKBUILD

	// staging
	g_GPU.free(m_StagingMemory);
	g_GPU.free(m_StagingBuffer);

	// buffer
	g_GPU.free(image_view);
	g_GPU.free(m_Texture);
	g_GPU.free(m_TextureMemory);

	// sampler
	g_GPU.free(sampler);
#endif
}

/**
 *	load texture from path and finally upload to gpu memory
 *	\param gpb: target pixel buffer
 *	\param pbc: pointer to atlas component information, this will be overwritten
 *	\param path: path to texture file
 *	NOTE this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
void GPUPixelBuffer::load_texture(GPUPixelBuffer* gpb,Rect* pbc,const char* path)
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
	FT_Error _failed = FT_New_Face(g_FreetypeLibrary,path,0,&__Face);
	COMM_ERR_COND(_failed,"font loading unsuccessful. failed with code %d",_failed);
	FT_Set_Pixel_Sizes(__Face,0,size);

	// iterate font glyphs
	for (u8 i=0;i<96;i++)
	{
		// rasterize glyph
		_failed = FT_Load_Char(__Face,i+32,FT_LOAD_RENDER);
		COMM_ERR_COND(_failed,"rasterization of character %c failed. code %d",(char)i+32,_failed);

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
 *	TODO
 */
void _merge_segment(vector<Rect>& ums,Rect& seg,u32& len)
{
	u32 i = 0;
	while (i<len)
	{
		// new segment is rarely obsolete by perfect segmentation
		Rect& p_Segment = ums[i];
		if (p_Segment.contains(seg)) return;

		// swap and remove should new segment be fully enclosing existing segment
		if (seg.contains(p_Segment))
		{
			ums[i] = ums[--len];
			ums[len] = ums.back();
			ums.pop_back();
		}

		// no fully obsoleting relationship, moving on
		else i++;
	}

	// feed segment
	ums.push_back(seg);
}

/**
 *	write texture buffer to preallocated gpu memory
 *	\param gpb: target pixel buffer
 *	\param pbc: pointer to atlas component information, this will be overwritten
 *	\param data: texture data
 *	NOTE this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
void GPUPixelBuffer::_load(GPUPixelBuffer* gpb,Rect* pbc,TextureData* data)
{
	gpb->mutex_memory_segments.lock();

	// compute padded dimensions
	s32 __PaddedWidth = data->width+gpb->subtex_padding;
	s32 __PaddedHeight = data->height+gpb->subtex_padding;

	// locate best position for texture on free memory space
	f32 __BestDifference = 0x7f800000;
	u32 __MemoryIndex = -1;
	for (u32 i=0;i<gpb->memory_segments.size();i++)
	{
		Rect* p_FreeComponent = &gpb->memory_segments[i];
		if (__PaddedWidth>p_FreeComponent->extent.x||__PaddedHeight>p_FreeComponent->extent.y) continue;

		// find closest fit
		f32 __AreaDifference = p_FreeComponent->extent.x*p_FreeComponent->extent.y
				- __PaddedWidth*__PaddedHeight;
		if (__AreaDifference<__BestDifference)
		{
			__MemoryIndex = i;
			__BestDifference = __AreaDifference;
		}
	}

	// get memory segment pointer
	COMM_ERR_COND(__MemoryIndex==-1,"sprite texture memory is populated or segmented. texture upload failed!");
	COMM_MSG_COND(__MemoryIndex==-1,LOG_CYAN,"attempted load dimensions -> (%i,%i)",data->width,data->height);
	Rect* p_CloseFitComponent = &gpb->memory_segments[__MemoryIndex];

	// write atlas information unpadded, this will be used as atlas coordinates in shader
	data->x = p_CloseFitComponent->position.x, data->y = p_CloseFitComponent->position.y;
	pbc->position = p_CloseFitComponent->position*gpb->dimensions_inv;
	pbc->extent = vec2(data->width,data->height)*gpb->dimensions_inv;

	// store unnormalized overwritten segment for memory splicing
	Rect __OverwrittenArea = {
		.position = p_CloseFitComponent->position,
		.extent = vec2(__PaddedWidth,__PaddedHeight)
	};

	// iterate free memory segments as maintenance update
	u32 i=0;
	vector<Rect> __UpdatedSegments;
	__UpdatedSegments.reserve(4);  // FIXME not vectorized, place this on stack instead
	while (i<gpb->memory_segments.size())
	{
		// test segment intersections with texture rect
		Rect& p_Segment = gpb->memory_segments[i];
		if (!p_Segment.intersects(__OverwrittenArea))
		{
			i++;
			continue;
		}
		u32 __MomentaryLength = __UpdatedSegments.size();

		// overwritten area intersects with segment on x-axis
		if (__OverwrittenArea.position.x<(p_Segment.position.x+p_Segment.extent.x)
			&&(__OverwrittenArea.position.x+__OverwrittenArea.extent.x>p_Segment.position.x))
		{
			// top segment
			if (__OverwrittenArea.position.y>p_Segment.position.y
				&&__OverwrittenArea.position.y<(p_Segment.position.y+p_Segment.extent.y))
			{
				Rect __TopSegment = p_Segment;
				__TopSegment.extent.y = __OverwrittenArea.position.y-__TopSegment.position.y;
				_merge_segment(__UpdatedSegments,__TopSegment,__MomentaryLength);
			}

			// bottom segment
			if ((__OverwrittenArea.position.y+__OverwrittenArea.extent.y)
				<(p_Segment.position.y+p_Segment.extent.y))
			{
				Rect __BottomSegment = p_Segment;
				__BottomSegment.position.y = __OverwrittenArea.position.y+__OverwrittenArea.extent.y;
				__BottomSegment.extent.y = p_Segment.position.y+p_Segment.extent.y
					-(__OverwrittenArea.position.y+__OverwrittenArea.extent.y);
				_merge_segment(__UpdatedSegments,__BottomSegment,__MomentaryLength);
			}
		}

		// overwritten area intersects with segment on y-axis
		if (__OverwrittenArea.position.y<(p_Segment.position.y+p_Segment.extent.y)
			&&(__OverwrittenArea.position.y+__OverwrittenArea.extent.y>p_Segment.position.y))
		{
			// left segment
			if (__OverwrittenArea.position.x>p_Segment.position.x
				&&__OverwrittenArea.position.x<(p_Segment.position.x+p_Segment.extent.x))
			{
				Rect __LeftSegment = p_Segment;
				__LeftSegment.extent.x = __OverwrittenArea.position.x-__LeftSegment.position.x;
				_merge_segment(__UpdatedSegments,__LeftSegment,__MomentaryLength);
			}

			// right segment
			if ((__OverwrittenArea.position.x+__OverwrittenArea.extent.x)
				<(p_Segment.position.x+p_Segment.extent.x))
			{
				Rect __RightSegment = p_Segment;
				__RightSegment.position.x = __OverwrittenArea.position.x+__OverwrittenArea.extent.x;
				__RightSegment.extent.x = p_Segment.position.x+p_Segment.extent.x
					-(__OverwrittenArea.position.x+__OverwrittenArea.extent.x);
				_merge_segment(__UpdatedSegments,__RightSegment,__MomentaryLength);
			}
		}

		// remove segment when split
		p_Segment = gpb->memory_segments.back();
		gpb->memory_segments.pop_back();
	}

	// iterate segment lists for obsoletion
	for (u32 i=0;i<gpb->memory_segments.size();i++)
	{
		u32 j = 0;
		while (j<__UpdatedSegments.size())
		{
			// test if segment is obsolete through previous memory segment
			if (gpb->memory_segments[i].contains(__UpdatedSegments[j]))
			{
				__UpdatedSegments[j] = __UpdatedSegments.back();
				__UpdatedSegments.pop_back();
			}

			// new segment must be merged into segment vector
			else j++;
		}
	}
	// FIXME this is still a memory access rc hazard, is it not?

	// merge segment list
	gpb->memory_segments.insert(gpb->memory_segments.end(),__UpdatedSegments.begin(),__UpdatedSegments.end());
	gpb->mutex_memory_segments.unlock();

	// write buffer
	gpb->mutex_texture_requests.lock();
	gpb->load_requests.push(*data);
	gpb->mutex_texture_requests.unlock();
}
// TODO prohibit creating pixel buffers that violate the device limit, to reduce silent memory stalling
// TODO allow to merge freed memory segments with atlas segment vector

/**
 *	automatically uploads the loaded subtextures to the gpu
 *	\param channel: texture channel (this has to be defined in ogl version, can be ignored in vk)
 *	NOTE this has to be run in main thread due to the gpu upload being context sensitive
 */
void GPUPixelBuffer::gpu_upload(u8 channel)
{
	if (!load_requests.size()) return;

	mutex_texture_requests.lock();

#ifdef VKBUILD
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_graphical_command_buffer()->buffer;

	// setup memory barrier
	VkImageMemoryBarrier __Barrier = _generate_memory_barrier_tex(m_Texture,m_Mipcount);
	__Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	__Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	__Barrier.srcAccessMask = 0;
	__Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0,0,nullptr,0,nullptr,1,&__Barrier);

	// iterate waiting requests
	size_t __MemoryOffset = 0;
	while (load_requests.size())
#else
	atlas.bind(channel);
	while (load_requests.size()&&calculate_delta_time_ms(g_Frame.fstart)<FRAME_TIME_BUDGET_MS)
#endif
	{
		TextureData& p_Data = load_requests.front();
#ifdef VKBUILD
		p_Data.gpu_upload_subtexture(m_Texture,m_StagingBuffer,m_StagingMemory,__MemoryOffset);
		__MemoryOffset += p_Data.width*p_Data.height*_texture_formats[m_Format].size;
#else
		p_Data.gpu_upload_subtexture();
#endif
		load_requests.pop();
		// TODO join those subtexture uploads into one, by offsetting the data in staging buffer here
	}
	COMM_LOG_COND(load_requests.size(),"stalling upload in pixel buffer");
	// TODO transition this naive, unstalled implementation to an actually good implementation

	// controversial pixel buffer lod creation
	mutex_texture_requests.unlock();
#ifdef VKBUILD

	// memory barrier mipmapping
	VkImageMemoryBarrier __MMBarrier = _generate_memory_barrier_tex(m_Texture,1);

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

	// generate mipmaps
	s32 __MMWidth = m_Width;
	s32 __MMHeight = m_Height;
	for (u16 i=1;i<m_Mipcount;i++)
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
	__MMBarrier.subresourceRange.baseMipLevel = m_Mipcount-1;
	__MMBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	__MMBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	__MMBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	__MMBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 0,0,nullptr,0,nullptr,1,&__MMBarrier);

#else
	glGenerateMipmap(GL_TEXTURE_2D);
#endif
}
// TODO sort into appropriate utility
// FIXME performance will suffer when generating mipmap every time the loop condition breaks
