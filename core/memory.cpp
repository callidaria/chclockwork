#include "memory.h"


// ----------------------------------------------------------------------------------------------------
// Memory Barriers

/**
 *	generate memory barrier for texture, image or other pixel buffer
 *	\param tex: target image
 *	\param mip: count of generated mipmap levels
 *	\returns assembled barrier info
 */
inline static VkImageMemoryBarrier _generate_memory_barrier_tex(VkImage tex,u32 layers,u32 mip)
{
	VkImageMemoryBarrier __Barrier = {  };
	__Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	__Barrier.image = tex;
	__Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__Barrier.subresourceRange.baseArrayLayer = 0;
	__Barrier.subresourceRange.baseMipLevel = 0;
	__Barrier.subresourceRange.layerCount = layers;
	__Barrier.subresourceRange.levelCount = mip;
	return __Barrier;
}


// ----------------------------------------------------------------------------------------------------
// Vertex Buffer Heap

/**
 *	allocate vram heap for future vbo, ibo and instance data memory uploads
 *	\param buffer: target buffer handle
 *	\param size: size of preallocated heap in bytesize
 */
VertexBufferHeap::VertexBufferHeap(VkBuffer* buffer,size_t size)
	: m_Buffer(buffer)
{
	COMM_AWT("allocating vram heap for vertex buffers");
	g_GPU.generate_buffer(*m_Buffer,m_Memory,size,VK_BUFFER_USAGE_TRANSFER_DST_BIT
						  |VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	g_GPU.generate_buffer(m_StagingBuffer,m_StagingMemory,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkMapMemory(g_GPU.gpu,m_StagingMemory,0,size,0,&mem);
	COMM_CNF();
}
// TODO optimize, do not rely on coherent bit and flush explicitly later
// TODO do not stage in case the engine runs on a system with integrated graphics, it should be detremental

/**
 *	queue an upload for vertex buffer heap memory
 *	\param offset: start of the memory segment
 *	\param size: memsize of memory segment (also range in MemoryRange) in bytes
 */
void VertexBufferHeap::upload(size_t offset,size_t size)
{
	m_CopyRegions.push_back({
			.srcOffset = offset,
			.dstOffset = offset,
			.size = size
		});
}

/**
 *	upload all registered buffer regions from staging buffer to vram
 */
void VertexBufferHeap::gpu_upload()
{
	if (!m_CopyRegions.size()) return;
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_transfer_command_buffer()->buffer;

	// memory barrier access after last transfer
	VkBufferMemoryBarrier __MemoryBarrier = {  };
	__MemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	__MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	__MemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	__MemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MemoryBarrier.buffer = *m_Buffer;
	__MemoryBarrier.offset = 0;
	__MemoryBarrier.size = VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,
						 0,nullptr,1,&__MemoryBarrier,0,nullptr);

	// copy buffer
	vkCmdCopyBuffer(__CMDBuffer,m_StagingBuffer,*m_Buffer,m_CopyRegions.size(),&m_CopyRegions[0]);

	// memory barrier to access from graphics queue
	__MemoryBarrier.dstAccessMask = 0;
	__MemoryBarrier.srcQueueFamilyIndex = g_GPU.device_info->transfer_queue;
	__MemoryBarrier.dstQueueFamilyIndex = g_GPU.device_info->graphical_queue;
	vkCmdPipelineBarrier(__CMDBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,
						 0,nullptr,1,&__MemoryBarrier,0,nullptr);

	// reset
	m_CopyRegions.clear();
}
// TODO consider using inheritance info to work with secondary command buffers

/**
 *	free vbo memory heap & destruct buffers
 */
void VertexBufferHeap::vanish()
{
	vkUnmapMemory(g_GPU.gpu,m_StagingMemory);
	vkWaitForFences(g_GPU.gpu,1,&g_GPU.acquire_transfer_command_buffer()->processing,VK_TRUE,UINT64_MAX);
	g_GPU.free(m_StagingBuffer);
	g_GPU.free(m_StagingMemory);
	g_GPU.free(*m_Buffer);
	g_GPU.free(m_Memory);
}

/**
 *	bind the vertex buffer heap
 *	\param bfrs: buffer count. =1 if only vertex data is needed, =2 if binding for instance draw
 *	\param vofs: vertex buffer segment offset in heap
 *	\param iofs: (default 0) instance buffer segment offset in heap
 */
void VertexBufferHeap::bind(u8 bfrs,size_t vofs,size_t iofs)
{
	VkDeviceSize __Offsets[2] = { vofs,iofs };
	vkCmdBindVertexBuffers(g_GPU.acquire_graphical_command_buffer()->buffer,0,bfrs,
						   g_VertexBuffers,__Offsets);
	// TODO find a solution for indices, maybe allocate from the back?
}
// TODO investigate vertex pulling for possible improvements, to reduce the amount of rebinds.
//		although in my initial research i've found, that it is used to increase dev dynamics, not performance


// ----------------------------------------------------------------------------------------------------
// Vertex Buffer

/**
 *	vertex buffer allocation
 *	\param size: buffer size in bytes
 *	\param type: static if this holds vertices, dynamic if it holds constantly updating ibo or similar
 *	\returns pointer to allocated segment in staging buffer memory
 */
void* VertexBuffer::allocate(size_t size,VertexBufferType type)
{
	m_Type = type;
#ifdef VKBUILD
	segment = {
		.offset = g_VertexBufferHeap[type].mcr.acquire_memory_segment(size),
		.range = size
	};
	m_Data = (u8*)g_VertexBufferHeap[type].mem+segment.offset;
#else
	glGenVertexArrays(1,&m_VAO);
	glGenBuffers(1,&m_VBO);
#endif
	return (void*)m_Data;
}
// TODO consider a ring buffer system for multi-frame processing for instances

/**
 *	vertex buffer upload, vertices only. copies memory
 *	\param vertices: pointer to vertex memory
 *	\param vsize: size of vertex memory copy range in bytes
 *	\param ofs: (default 0) offset in vertex memory to start the copy from in bytes
 *	\note should only be utilized for one-time geometry upload, it is preferred to write directly via pointer
 */
void VertexBuffer::upload(void* vertices,size_t vsize,size_t ofs)
{
	COMM_AWT("copying vertex buffer data");
#ifdef VKBUILD
	memcpy((void*)(m_Data+ofs),vertices,vsize);
	register_upload(vsize,ofs);
#else
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
	glBufferData(GL_ARRAY_BUFFER,vsize,vertices,GL_STATIC_DRAW);
#endif
	COMM_CNF();
}

/**
 *	vertex buffer upload, vertices alongside indices. copies memory
 *	\param vertices: pointer to vertex memory
 *	\param vsize: size of vertex memory copy range in bytes
 *	\param indices: pointer to index memory
 *	\param isize: size of index memory copy range in bytes
 *	\note should only be utilized for one-time geometry upload, it is preferred to write directly via pointer
 */
void VertexBuffer::upload(void* vertices,size_t vsize,void* indices,size_t isize)
{
	COMM_AWT("copying vertex & index buffer data");
	COMM_ERR("do not use index uploads, this feature is not appropriately supported yet");
#ifdef VKBUILD
	index_offset = vsize;
	memcpy((void*)(m_Data),vertices,vsize);
	memcpy((void*)(m_Data+vsize),indices,isize);
	register_upload(vsize+isize);
#else
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
	glBufferData(GL_ARRAY_BUFFER,vsize,vertices,GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER,isize,indices,GL_STATIC_DRAW);
#endif
	COMM_CNF();
}

/**
 *	register buffer memory update
 *	\param size: memory size of updated range in bytes
 *	\param ofs: (default 0) start of memory range in bytes
 */
void VertexBuffer::register_upload(size_t size,size_t ofs)
{
	g_VertexBufferHeap[m_Type].upload(segment.offset+ofs,size);
}

/**
 *	free memory segment assigned to this vbo, should it not be needed anymore
 */
void VertexBuffer::free()
{
	g_VertexBufferHeap[m_Type].mcr.free_memory_segment(segment);
}

#ifndef VKBUILD

/**
 *	bind vertex array?
 */
void VertexBuffer::bind()
{
	glBindVertexArray(m_VAO);
}

#endif


// ----------------------------------------------------------------------------------------------------
// Vertex Array

#ifdef VKBUILD

/**
 *	add vertex buffer to array
 *	\param vb: vertex buffer to register
 */
void VertexArray::register_buffer(const VertexBuffer& vb)
{
	m_VOfs = vb.segment.offset;
}

/**
 *	add dynamic vertex buffer, for constantly changing data. e.g. for an instance buffer, holding positions
 *	\param vb: instance buffer to register
 */
void VertexArray::register_buffer_dynamic(const VertexBuffer& vb)
{
	m_IOfs = vb.segment.offset;
	m_Instanced = true;
}

/**
 *	add indexed vertex buffer to array. this vertex buffer must have been created with indexed=true
 *	\param vb: vertex/instance buffer to register
 */
void VertexArray::register_buffer_indexed(const VertexBuffer& vb)
{
	COMM_MSG_COND(m_IndexSource>-1,LOG_YELLOW,"WARNING: a previous buffer has already set the index component");
	COMM_ERR("unfortunately the indexing feature is currently broken & must be fixed before usage");
	m_IndexOffset = vb.index_offset;
	register_buffer(vb);
}

/**
 *	bind vertex array
 */
void VertexArray::bind()
{
	VertexBufferHeap::bind(1+m_Instanced,m_VOfs,m_IOfs);
}

/**
 *	bind indexed vertex array
 */
/*
void VertexArray::bind_indexed()
{
	COMM_ERR_COND(m_IndexSource<0,"an indexed bind is requested, but no source was ever defined");
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	vkCmdBindVertexBuffers(__CMDBuffer,0,m_Buffers.size(),&m_Buffers[0],&m_Offsets[0]);
	vkCmdBindIndexBuffer(__CMDBuffer,m_Buffers[m_IndexSource],m_IndexOffset,VK_INDEX_TYPE_UINT32);
}
*/

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
	{ VK_FORMAT_B10G11R11_UFLOAT_PACK32,4 },
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
 *	generate data for single colour texture
 *	\param colour: colour vector
 */
void TextureData::load(vec3 colour)
{
	width = 1;
	height = 1;

	// write colour data
	data.intbfr = (u8*)malloc(sizeof(u32));
	data.intbfr[0] = colour.x*255;
	data.intbfr[1] = colour.y*255;
	data.intbfr[2] = colour.z*255;
	data.intbfr[3] = 255;
}

/**
 *	make the cpu load the texture data & dimensions from file
 *	\param path: path to texture
 */
void TextureData::load(const char* path)
{
	COMM_ERR_COND(!check_file_exists(path),"texture at %s could not be found",path);
	stbi_set_flip_vertically_on_load(true);
	data.intbfr = stbi_load(path,&width,&height,0,STBI_rgb_alpha);
	type = TEX_DATA_TYPE_INTEGER;
}
// FIXME that stbi_set_flip_vertically_on_load call here is due to my build setup, but should be at init
//		the annoyance is pointing to a much bigger problem though, the entire stbi is reprocessed per tu
//		this increases compile time for files that do not necessarily need it and also leads to bad behaviour.
//		additionally there is the problem that with the current setup all userscripts are receiving renderer
//		and by extension must also process the stbi header. this cannot be the right solution!

/**
 *	TODO
 */
void TextureData::load_hdr(const char* path)
{
	COMM_ERR_COND(!check_file_exists(path),"hdr cubemap at %s could not be found",path);

	// load texture
	stbi_set_flip_vertically_on_load(true);
	f32* __ImageBfr = stbi_loadf(path,&width,&height,0,STBI_rgb_alpha);

	// package into 4-byte unsigned floatbuffer
	data.floatbfr = (u32*)malloc(width*height*sizeof(u32));
	for (size_t i=0;i<width*height;i++)
	{
		vec3 __Pixel = vec3(__ImageBfr[i*4+0],__ImageBfr[i*4+1],__ImageBfr[i*4+2]);
		data.floatbfr[i] = glm::packF2x11_1x10(__Pixel);
	}

	// free original texture & signal type
	stbi_image_free(__ImageBfr);
	type = TEX_DATA_TYPE_FLOAT;
}

/**
 *	free buffer memory
 */
void TextureData::vanish()
{
	switch (type)
	{
	case TEX_DATA_TYPE_OVERWRITTEN: free(data.intbfr);
		break;
	case TEX_DATA_TYPE_INTEGER: stbi_image_free(data.intbfr);
		break;
	case TEX_DATA_TYPE_FLOAT: free(data.floatbfr);
		break;
	};
}

/**
 *	setup texture buffer
 *	\param width: texture width
 *	\param height: texture height
 *	\param format: colourspace format of texture
 *	\param feat: (default TEXTURE_FEATURE_STANDARD) texture features
 */
void Texture::allocate(u32 wdt,u32 hgt,TextureFormat format,TextureFeatures feat)
{
	width = wdt;
	height = hgt;
	mipcount = (feat&(TEXTURE_FEATURE_SUPPRESS_MIPMAP_BIT|TEXTURE_FEATURE_CUBEMAP_BIT))
				? 1 : std::floor(std::log2(std::max(width,height)))+1;
	m_Format = format;
	// TODO use mip definition in allocation for ogl version as well

	// cubemap layer switching
	bool __Cubemap = feat&TEXTURE_FEATURE_CUBEMAP_BIT;
	layercount = 1+5*__Cubemap;

#ifdef VKBUILD
	// generate staging buffer
	g_GPU.generate_buffer(m_StagingBuffer,m_StagingMemory,
						  width*height*_texture_formats[format].size*layercount,
						  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// TODO optimize, do not rely on coherent bit and flush explicitly later
	// TODO !!!!kill staging buffer after upload for all textures, except those that are used by atlasses!!!!

	// image buffer
	VkImageCreateInfo __ImageInfo = {  };
	__ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	__ImageInfo.flags = 0|(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT*__Cubemap);
	__ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	__ImageInfo.format = _texture_formats[m_Format].format;
	__ImageInfo.extent.width = width;
	__ImageInfo.extent.height = height;
	__ImageInfo.extent.depth = 1;
	__ImageInfo.mipLevels = mipcount;
	__ImageInfo.arrayLayers = layercount;
	__ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	__ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	__ImageInfo.usage
			= VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	__ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	__ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkResult __Result = vkCreateImage(g_GPU.gpu,&__ImageInfo,nullptr,&m_Texture);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"could not create texture image handle");
	// TODO as a possible late-stage optimization look into image unionization for multiple image destinations
	//		utilizing the same memory by setting an initialLayout and an alias
	//		this will probably just be useful for rendertarget results that do not overlap in timing

	// memory
	g_GPU.allocate_vram(m_TextureMemory,m_Texture);
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
	VkImageMemoryBarrier __Barrier = _generate_memory_barrier_tex(m_Texture,layercount,mipcount);
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
	__ImageViewInfo.viewType = (__Cubemap) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	__ImageViewInfo.format = _texture_formats[m_Format].format;
	__ImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__ImageViewInfo.subresourceRange.baseMipLevel = 0;
	__ImageViewInfo.subresourceRange.levelCount = mipcount;
	__ImageViewInfo.subresourceRange.baseArrayLayer = 0;
	__ImageViewInfo.subresourceRange.layerCount = layercount;
	__ImageViewInfo.components = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY,
	};
	__Result = vkCreateImageView(g_GPU.gpu,&__ImageViewInfo,nullptr,&image_view);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"image view creation failed");

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
	// FIXME many of this does not need to happen over and over again for each gpupixelbuffer
	// TODO maybe no 1:1 relationship for samplers. not every texture needs its own sampler
#else
	glGenTextures(1,&m_Memory);
	glTexImage2D(GL_TEXTURE_2D,0,_texture_formats[format].format,width,height,0,
				 _texture_format_internal[format],GL_UNSIGNED_BYTE,0);
#endif

	// memory correlation & sign as ready
	vkMapMemory(g_GPU.gpu,m_StagingMemory,0,width*height*_texture_formats[m_Format].size,0,&m_Data);
	allocated = true;
}

/**
 *	register subtexture upload data to be uploaded at a later point through gpu_upload()
 *	\param data: texture image data
 *	\note OGL: has to be uploaded in main thread
 *	\note OGL: target texture has to be bound and allocated before uploading
 *	\note upload requires the manual calling of gpu_upload()
 */
void Texture::register_subtexture(TextureData& data)
{
#ifdef VKBUILD
	_setup_buffer_copy(m_UploadCursor,data);
#else
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,data.width,data.height,
					_texture_formats[m_Format].format,GL_UNSIGNED_BYTE,data.data);
#endif
	data.vanish();
}

/**
 *	upload all registered subtexture uploads, this should be run for texture atlasses
 */
void Texture::gpu_upload()
{
	// upload image data
	vkCmdCopyBufferToImage(g_GPU.acquire_graphical_command_buffer()->buffer,
						   m_StagingBuffer,m_Texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   m_TextureCopies.size(),&m_TextureCopies[0]);

	// reset
	m_TextureCopies.clear();
	m_UploadCursor = 0;
}

/**
 *	upload texture image data directly to gpu, this should be called for singular textures
 *	\param data: texture image data
 *	\note OGL: has to be uploaded in main thread
 *	\note OGL: target texture has to be bound before uploading
 */
void Texture::gpu_upload(TextureData& data)
{
#ifdef VKBUILD
	_setup_buffer_copy(m_UploadCursor,data);
	gpu_upload();
	vkUnmapMemory(g_GPU.gpu,m_StagingMemory);
	m_ActiveAtlas = false;
#else
	glTexImage2D(GL_TEXTURE_2D,0,_texture_format_internal[m_Format],data.width,data.height,0,
				 _texture_formats[m_Format].format,GL_UNSIGNED_BYTE,data.data);
#endif
	data.vanish();
}

/**
 *	TODO
 */
void Texture::gpu_upload(vector<TextureData>& data)
{
#ifdef VKBUILD
	for (size_t i=0;i<data.size();i++)
	{
		_setup_buffer_copy(m_UploadCursor,data[i],i);
		data[i].vanish();
	}
	gpu_upload();
	vkUnmapMemory(g_GPU.gpu,m_StagingMemory);
	m_ActiveAtlas = false;
#else
	// TODO
#endif
}

/**
 *	generate mipmap for texture
 */
void Texture::generate_mipmap()
{
#ifdef VKBUILD
	VkCommandBuffer& __CMDBuffer = g_GPU.acquire_graphical_command_buffer()->buffer;
	VkImageMemoryBarrier __MMBarrier = _generate_memory_barrier_tex(m_Texture,layercount,1);

	// mipmap blit
	VkImageBlit __MMBlit = {  };
	__MMBlit.srcOffsets[0] = { 0,0,0 };
	__MMBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__MMBlit.srcSubresource.baseArrayLayer = 0;
	__MMBlit.srcSubresource.layerCount = 1;
	__MMBlit.dstOffsets[0] = { 0,0,0 };
	__MMBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__MMBlit.dstSubresource.baseArrayLayer = 0;
	__MMBlit.dstSubresource.layerCount = layercount;
	// TODO maybe move this to texture preprocessing and skip the blitting at load time

	// generate mipmaps
	s32 __MMWidth = width;
	s32 __MMHeight = height;
	for (u16 i=1;i<mipcount;i++)
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
	__MMBarrier.subresourceRange.baseMipLevel = mipcount-1;
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
// TODO in vk when this step is skipped there needs to be a barrier, that achieves the same state
//		as if after the mipmap generation
// FIXME also skipping mipmap is forbidden, when miplevels get calculated automatically by dimensions at alloc

#ifdef VKBUILD

/**
 *	destruct texture
 */
void Texture::vanish()
{
	if (m_ActiveAtlas) vkUnmapMemory(g_GPU.gpu,m_StagingMemory);

	// staging
	g_GPU.free(m_StagingMemory);
	g_GPU.free(m_StagingBuffer);

	// buffer
	g_GPU.free(image_view);
	g_GPU.free(m_Texture);
	g_GPU.free(m_TextureMemory);

	// sampler
	g_GPU.free(sampler);
}

/**
 *	transition texture to transfer state after allocation
 */
void Texture::transition_texture_transfer()
{
	VkImageMemoryBarrier __Barrier = _generate_memory_barrier_tex(m_Texture,layercount,mipcount);
	__Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	__Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	__Barrier.srcAccessMask = 0;
	__Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(g_GPU.acquire_graphical_command_buffer()->buffer,
						 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0,0,nullptr,0,nullptr,1,&__Barrier);
}

/**
 *	transition depth texture from framebuffer, so that it can be used as input texture in following shading steps
 *	\param buffer: depth buffer result, sourced from framebuffer component image
 */
void Texture::transition_depth_texture(VkImage buffer)
{
	VkImageMemoryBarrier __MemoryBarrier = {  };
	__MemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	__MemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	__MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	__MemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	__MemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	__MemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	__MemoryBarrier.image = buffer;
	__MemoryBarrier.subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
	vkCmdPipelineBarrier(g_GPU.acquire_graphical_command_buffer()->buffer,
						 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 0,0,nullptr,0,nullptr,1,&__MemoryBarrier);
}

#else

/**
 *	set texture channel
 *	\param i: channel index, correlating to sampler2D integer upload
 */
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

/**
 *	define trilinear texture filter for mipmap generation
 *	\note texture should be bound
 *	\note this allows generate_mipmap() to be executed if no manual approach is chosen
 */
void Texture::set_texture_parameter_linear_mipmap()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

/**
 *	define bilinear texture filter for mipmap generation
 *	\note texture should be bound
 *	\note this allows generate_mipmap() to be executed if no manual approach is chosen
 */
void Texture::set_texture_parameter_nearest_mipmap()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

/**
 *	define linear filter without mipmapping
 *	\note texture should be bound
 */
void Texture::set_texture_parameter_linear_unfiltered()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

/**
 *	define filterless pixelraster
 *	\note texture should be bound
 */
void Texture::set_texture_parameter_nearest_unfiltered()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

/**
 *	define extension behaviour to stretch to maximum size
 *	\note texture should be bound
 */
void Texture::set_texture_parameter_clamp_to_edge()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}

/**
 *	define extension behaviour to be scaled towards custom borders, avoiding ratio manipulation
 *	\note texture should be bound
 */
void Texture::set_texture_parameter_clamp_to_border()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
}

/**
 *	repeat texture over vertices without scaling
 *	\note texture should be bound
 */
void Texture::set_texture_parameter_repeat()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

/**
 *	define level of detail through filter bias (will additively shift the perceived lod value)
 *	\param bias: (default .0f) filter bias, addition bias means that .0f is no bias
 *	\note texture should be bound
 */
void Texture::set_texture_parameter_filter_bias(float bias)
{
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_LOD_BIAS,bias);
}

/**
 *	define border colour for texture when set_texture_parameter_clamp_to_border() is defined
 *	\param colour: RGBA border colour as vector
 *	\note texture should be bound
 *	\note pointer trick not field tested, should border colour fail to work this is the most likely culprit
 */
void Texture::set_texture_parameter_border_colour(vec4 colour)
{
	glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,&colour.r);
}

#endif

#ifdef VKBUILD

/**
 *	setup buffer to texture/atlas memory copy, by copying to stage. then register upload region over this memory
 *	\param ofs: offset in staging buffer, will automatically be progressing as memory write cursor
 *	\param data: texture dimensions & data
 *	\param arr_index: (default 0) array upload position index
 */
void Texture::_setup_buffer_copy(size_t& ofs,TextureData& data,size_t arr_index)
{
	if (!data.width||!data.height)
	{
		COMM_ERR("texture data without width or height has been submitted");
		return;
	}

	// copy to staging memory
	size_t __ImageSize = data.width*data.height*_texture_formats[m_Format].size;
	if (data.type==TEX_DATA_TYPE_FLOAT) memcpy((void*)((f32*)m_Data+ofs),data.data.floatbfr,__ImageSize);
	else memcpy((void*)((u8*)m_Data+ofs),data.data.intbfr,__ImageSize);

	// insert copy region
	VkBufferImageCopy __BufferCopy = {  };
	__BufferCopy.bufferOffset = ofs;
	__BufferCopy.bufferRowLength = 0;
	__BufferCopy.bufferImageHeight = 0;
	__BufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	__BufferCopy.imageSubresource.mipLevel = 0;
	__BufferCopy.imageSubresource.baseArrayLayer = arr_index;
	__BufferCopy.imageSubresource.layerCount = 1;
	__BufferCopy.imageOffset = { (s32)data.x,(s32)data.y,0 };
	__BufferCopy.imageExtent = { (u32)data.width,(u32)data.height,1 };
	m_TextureCopies.push_back(__BufferCopy);

	ofs += __ImageSize;
}

#endif


// ----------------------------------------------------------------------------------------------------
// Font

/**
 *	calculate estimated word length in given font
 *	\param word: given word for length estimation
 *	\param offset: (default 0) wordlength character offset to exclude buffer tail
 *	\returns returns wordlength (on x-axis for unrotated text)
 */
f32 Font::estimate_wordlength(string& word,u32 offset)
{
	f32 out = .0f;
	for (u32 i=0;i<word.size()-offset;i++) out += glyphs[word[i]-32].advance;
	return out;
}


// ----------------------------------------------------------------------------------------------------
// Pixel Buffer Feature

/**
 *	TODO There should be two different texture store types available.
 *	Singular texture (GPUTexture) and a texture atlas (GPUPixelBuffer).
 *	Whenever a single draw call has a predictable amount of textures, a GPUTexture should be used.
 *	An example for this is the draw call to a mesh, using a fixed amount of material channels
 *	(albedo, normals, materials & emission).
 *	Should a drawcall be utilizing a dynamically determined amount of different textures,
 *	then the GPUPixelBuffer may be used.
 *	The engine provides a precedent for this utilization, through the combination of all UI sprite
 *	and animation elements into one draw call.
 *	This call therefore also needs a predictable amount of textures to be read from
 *	(no sparseless is allowed for the default use case, to
 *	provide support for older systems).
 *	All sprite and animation textures are stored into the texture atlas of the GPUPixelBuffer and indexed
 *	by texture coordinate.
 *
 *	TODO Due to big differences in driver- & hardware-side modernization
 *	there must be two different texture handling systems, depending on the device capabilities.
 *	Such is the curse of the PC developer.
 *	1) The modern way of handling textures is through sparse binding, although not available on all devices.
 *	2) Binding pre-updated descriptor sets as materials, for each draw call (OpenGL-esque)
 *
 *	TODO sparse binding is not implemented yet, everything is managed by the fallback system for now.
 */

/**
 *	allocate video memory to use as we, the programmers please
 *	\param width: buffer width
 *	\param height: buffer height
 *	\param format: colourspace format of pixels
 *	\param feat: (default TEXTURE_FEATURE_STANDARD) bitwise texture feature combination
 *	\param padding: (default 0) pixel space padding in between allocated subtextures
 *	\note cannot be executed in subthread, uses context bound to main thread in ogl version
 */
void GPUPixelBuffer::allocate(u32 wdt,u32 hgt,TextureFormat format,TextureFeatures feat,u32 padding)
{
	dimensions_inv = vec2(1.f/wdt,1.f/hgt);
	subtex_padding = padding;

	// mark initial, untouched allocated memory segment
	memory_segments.push_back({ .extent = vec2(wdt+padding,hgt+padding) });

	// allocate texture memory & handles
	atlas.allocate(wdt,hgt,format,feat);
}

/**
 *	static helper to allow a subtexture segment of an atlas to merge when obsolete
 *	\param ums: quantity of memory segments in atlas
 *	\param seg: segment to check coverage for in both directions (contained or containing)
 *	\param len: amount of relevant rects in ums to check
 */
inline void _merge_segment(vector<Rect>& ums,Rect& seg,u32& len)
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
 *	\note this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
inline void _load(GPUPixelBuffer* gpb,Rect* pbc,TextureData* data)
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
 *	load texture from path and finally upload to gpu memory
 *	\param gpb: target pixel buffer
 *	\param pbc: pointer to atlas component information, this will be overwritten
 *	\param path: path to texture file
 *	\note this is supposed to run as a subthread, hence the mutex and load request queue pointer
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
 *	\note this is supposed to run as a subthread, hence the mutex and load request queue pointer
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
		TextureData __TextureData;
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
		if (__TextureData.type==TEX_DATA_TYPE_FLOAT)
		{
			__TextureData.data.intbfr = (u8*)malloc(__Mem);
			memcpy(__TextureData.data.intbfr,__Face->glyph->bitmap.buffer,__Mem);
		}
		else
		{
			__TextureData.data.intbfr = (u8*)malloc(__Mem);
			memcpy(__TextureData.data.intbfr,__Face->glyph->bitmap.buffer,__Mem);
		}
		_load(gpb,&font->tex[i],&__TextureData);
	}
	gpb->signal.proceed();

	// store & clear
	FT_Done_Face(__Face);
}

/**
 *	automatically uploads the loaded subtextures to the gpu
 *	\param channel: texture channel (this has to be defined in ogl version, can be ignored in vk)
 *	\note this has to be run in main thread due to the gpu upload being context sensitive in ogl
 */
void GPUPixelBuffer::gpu_upload(u8 channel)
{
	if (!load_requests.size()) return;
	mutex_texture_requests.lock();

	// iterate waiting requests
#ifdef VKBUILD
	atlas.transition_texture_transfer();
	while (load_requests.size())
#else
	atlas.bind(channel);
	while (load_requests.size()&&calculate_delta_time_ms(g_Frame.fstart)<FRAME_TIME_BUDGET_MS)
#endif
	{
		TextureData& p_Data = load_requests.front();
		atlas.register_subtexture(p_Data);
		load_requests.pop();
	}
	atlas.gpu_upload();
	COMM_LOG_COND(load_requests.size(),"stalling upload in pixel buffer");

	// controversial pixel buffer lod creation
	mutex_texture_requests.unlock();
	atlas.generate_mipmap();
}
// FIXME performance will suffer when generating mipmap every time the loop condition breaks


// ----------------------------------------------------------------------------------------------------
// Cubemap

/**
 *	TODO
 */
void Cubemap::load(const char* path,TextureFormat fmt)
{
	format = fmt;
	std::filesystem::path __CmDir(path);
	data.resize(CUBEMAP_COMPONENT_CHANNEL_COUNT);
	data[CUBEMAP_COMPONENT_CHANNEL_POSX].load((__CmDir/"posx.jpg").c_str());
	data[CUBEMAP_COMPONENT_CHANNEL_NEGX].load((__CmDir/"negx.jpg").c_str());
	data[CUBEMAP_COMPONENT_CHANNEL_NEGY].load((__CmDir/"posy.jpg").c_str());
	data[CUBEMAP_COMPONENT_CHANNEL_POSY].load((__CmDir/"negy.jpg").c_str());
	data[CUBEMAP_COMPONENT_CHANNEL_POSZ].load((__CmDir/"posz.jpg").c_str());
	data[CUBEMAP_COMPONENT_CHANNEL_NEGZ].load((__CmDir/"negz.jpg").c_str());
}

/**
 *	TODO
 */
void Cubemap::gpu_upload()
{
	cubemap.allocate(data[0].width,data[0].height,format,TEXTURE_FEATURE_CUBEMAP_BIT);
	cubemap.transition_texture_transfer();
	cubemap.gpu_upload(data);
	cubemap.generate_mipmap();  // TODO just include into gpu_upload at this point
}

/**
 *	TODO
 */
void Cubemap::vanish()
{
	cubemap.vanish();
}
