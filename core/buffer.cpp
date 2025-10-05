#include "buffer.h"


// ----------------------------------------------------------------------------------------------------
// Vertex Array

/**
 *	create vertex array
 */
VertexArray::VertexArray()
{
#ifdef VKBUILD
	// TODO

#else
	glGenVertexArrays(1,&m_VAO);
#endif
}

/**
 *	bind vertex array
 */
void VertexArray::bind()
{
#ifdef VKBUILD
	// TODO

#else
	glBindVertexArray(m_VAO);
#endif
}


// ----------------------------------------------------------------------------------------------------
// Vertex Buffer

#ifdef VKBUILD
VkBufferUsageFlagBits _buffer_formats[BUFFER_TYPE_COUNT] = {
	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	VK_BUFFER_USAGE_INDEX_BUFFER_BIT
};
#else
GLenum _memory_formats[BUFFER_TYPE_COUNT] = {
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW
};
#endif

/**
 *	TODO
 */
void VertexBuffer::allocate(size_t size,BufferType type)
{
	m_BufferSize = size;
	m_BufferType = type;

#ifdef VKBUILD
	// vertex buffer
	VkBufferCreateInfo __BufferInfo = {  };
	__BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	__BufferInfo.size = size;
	__BufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	__BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult __Result = vkCreateBuffer(g_GPU.gpu,&__BufferInfo,nullptr,&m_VBO);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create vertex buffer");

	// analyze memory type
	VkMemoryRequirements __MemoryRequirements;
	vkGetBufferMemoryRequirements(g_GPU.gpu,m_VBO,&__MemoryRequirements);

	// iterate memory
	u32 __MemoryIndex = 0;
	while (__MemoryIndex<g_GPU.device_info->memory_properties.memoryTypeCount)
	{
		if ((__MemoryRequirements.memoryTypeBits&(1<<__MemoryIndex))
			&&(g_GPU.device_info->memory_properties.memoryTypes[__MemoryIndex].propertyFlags
			   &(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) break;
		__MemoryIndex++;
	}
	// TODO well this just has to be completely reworked before fully including this
	// TODO optimize, do not rely on coherent bit and flush explicitly later

	// buffer memory allocation
	VkMemoryAllocateInfo __MallocInfo = {  };
	__MallocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	__MallocInfo.allocationSize = __MemoryRequirements.size;
	__MallocInfo.memoryTypeIndex = __MemoryIndex;
	__Result = vkAllocateMemory(g_GPU.gpu,&__MallocInfo,nullptr,&m_Memory);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate VRAM for geometry for some reason");

	// bind memory to vbo
	vkBindBufferMemory(g_GPU.gpu,m_VBO,m_Memory,0);

#else
	glGenBuffers(1,&m_VBO);
#endif
}

#ifdef VKBUILD
/**
 *	TODO
 */
void VertexBuffer::vanish()
{
	g_GPU.free(m_VBO);
	g_GPU.free(m_Memory);
}
#endif

/**
 *	TODO
 */
void VertexBuffer::bind()
{
#ifdef VKBUILD
	// TODO
#else
	glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
#endif
}
// FIXME there is no bind/unbind in vulkan. how do i replicate this phenomenon?

/**
 *	TODO
 */
void VertexBuffer::bind_elements()
{
#ifdef VKBUILD
	// TODO bind vulkan elements
#else
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_VBO);
#endif
}

/**
 *	TODO
 */
void VertexBuffer::upload_vertices(void* verts)
{
#ifdef VKBUILD
	void* __Data;
	vkMapMemory(g_GPU.gpu,m_Memory,0,m_BufferSize,0,&__Data);
	memcpy(__Data,verts,m_BufferSize);
	vkUnmapMemory(g_GPU.gpu,m_Memory);
#else
	glBufferData(GL_ARRAY_BUFFER,m_BufferSize,verts,_memory_formats[m_BufferType]);
#endif
}

/**
 *	upload elements from array into buffer
 *	\param elements: array of optional element indices, mapping the vertex order
 *	\param size: size of array, holding the element indices
 *	NOTE vertex buffer has to be bound beforehand
 */
void VertexBuffer::upload_elements(u32* elements,size_t size)
{
#ifdef VKBUILD
	// TODO not sure if this will prevail, because the vulkan version will use element draw from the start

#else
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,size,elements,GL_STATIC_DRAW);
#endif
}

/**
 *	upload elements from vector into buffer
 *	\param elements: vector list of optional element indices to upload, mapping the vertex order
 *	NOTE vertex buffer has to be bound beforehand
 */
void VertexBuffer::upload_elements(vector<u32> elements)
{
#ifdef VKBUILD
	// TODO

#else
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,elements.size()*sizeof(u32),&elements[0],GL_STATIC_DRAW);
#endif
}


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
	stbi_set_flip_vertically_on_load(true);
	data = stbi_load(path,&width,&height,0,STBI_rgb_alpha);
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

#ifdef VKBUILD
	// TODO

#else
	glTexImage2D(GL_TEXTURE_2D,0,_texture_format_channels[format],width,height,0,
				 _texture_format_internal[format],GL_UNSIGNED_BYTE,0);
#endif
}
// TODO i don't believe all this schnickschnack is necessary for the vulkan version at all.
//		vulkan allows for a way more direct malloc procedure, this might be the biggest discrepancy in the vers

/**
 *	load texture from path and finally upload to gpu memory
 *	\param gpb: target pixel buffer
 *	\param pbc: pointer to atlas component information, this will be overwritten
 *	\param path: path to texture file
 *	NOTE this is supposed to run as a subthread, hence the mutex and load request queue pointer
 */
void GPUPixelBuffer::load_texture(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,const char* path)
{
	// load information from texture file
	TextureData __TextureData;
	__TextureData.load(path);
	gpb->signal.proceed();

	// upload to gpu memory & signal data safety
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
 *	\param fstart: time the current frame started
 *	NOTE this has to be run in main thread due to the gpu upload being context sensitive
 */
void GPUPixelBuffer::gpu_upload(u8 channel,std::chrono::steady_clock::time_point& fstart)
{
	atlas.bind(channel);
	mutex_texture_requests.lock();

	// iterate waiting requests
	while (load_requests.size()&&calculate_delta_time(fstart)<FRAME_TIME_BUDGET_MS)
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
// Rendertarget Colour Buffers

/**
 *	allocate memory for framebuffer
 *	\param count: number of components, that will be defined for this framebuffer
 */
Framebuffer::Framebuffer(u8 count)
{
	m_ColourComponents.resize(count);
#ifdef VKBUILD
	m_ColourComponentSetup = (VkAttachmentDescription*)malloc(count*sizeof(VkAttachmentDescription));
	m_ColourComponentReference = (VkAttachmentReference*)malloc(count*sizeof(VkAttachmentReference));
#else
	glGenFramebuffers(1,&m_Buffer);
	glGenTextures(count,&m_ColourComponents[0]);
#endif
}

/**
 *	colour component definition, allowed as many as the constructor has allocated
 *	\param index: frambuffer component index
 *	\param width: resolution width
 *	\param height: resolution height
 *	\param fbuffer: (default false) true if floatbuffer when extra precision is needed
 */
void Framebuffer::define_colour_component(u8 index,f32 width,f32 height,bool fbuffer)
{
#ifdef VKBUILD
	// specify colour component
	m_ColourComponentSetup[index] = {};
	m_ColourComponentSetup[index].format = g_Frame.swapchain.format.format;
	m_ColourComponentSetup[index].samples = VK_SAMPLE_COUNT_1_BIT;
	m_ColourComponentSetup[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_ColourComponentSetup[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_ColourComponentSetup[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	m_ColourComponentSetup[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_ColourComponentSetup[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_ColourComponentSetup[index].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// specify fragment output location
	m_ColourComponentReference[index] = {};
	m_ColourComponentReference[index].attachment = index;
	m_ColourComponentReference[index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// TODO load format based on given width & height, not based on the global format
	// TODO configure initialLayout in unison with clear op
	// TODO setup display texture in this case, because subpass is not yet working
	// TODO research if different component resolutions are even viable and check for standard setup impl.

#else
	glBindTexture(GL_TEXTURE_2D,m_ColourComponents[index]);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA+0x6f12*fbuffer,width,height,0,GL_RGBA,GL_UNSIGNED_INT+fbuffer,NULL);
	Texture::set_texture_parameter_nearest_unfiltered();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+index,GL_TEXTURE_2D,m_ColourComponents[index],0);
#endif
}
// TODO maybe define index inside the framebuffer struct as a cursor counter variable

/**
 *	depth component definition, only a single one per framebuffer allowed for obvious reasons
 *	\param width: resolution width
 *	\param height: resolution height
 */
void Framebuffer::define_depth_component(f32 width,f32 height)
{
#ifdef VKBUILD
	m_DepthComponentSetup = (VkAttachmentDescription*)malloc(sizeof(VkAttachmentDescription));
	m_DepthComponentReference = (VkAttachmentReference*)malloc(sizeof(VkAttachmentReference));
	m_DepthComponentSetup = {};
	m_DepthComponentReference = {};
	// TODO

#else
	glGenTextures(1,&m_DepthComponent);
	glBindTexture(GL_TEXTURE_2D,m_DepthComponent);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,width,height,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
	Texture::set_texture_parameter_nearest_unfiltered();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,m_DepthComponent,0);
#endif
}
// TODO this way the depth component can't be used when finalizing, the depth belongs into constructor malloc

/**
 *	combine previously defined framebuffer attachments
 *	NOTE: this has to happen after definitions of all components
 */
void Framebuffer::finalize()
{
#ifdef VKBUILD
	// specify graphical subpass
	VkSubpassDescription __SubpassDesc = {  };
	__SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	__SubpassDesc.colorAttachmentCount = m_ColourComponents.size();
	__SubpassDesc.pColorAttachments = m_ColourComponentReference;

	// subpass dependency
	VkSubpassDependency __SubpassDependency = {  };
	__SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	__SubpassDependency.dstSubpass = 0;
	__SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	__SubpassDependency.srcAccessMask = 0;
	__SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	__SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// TODO implement feature according to the todo placed in header file

	// render pass
	VkRenderPassCreateInfo __RPInfo = {  };
	__RPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	__RPInfo.attachmentCount = m_ColourComponents.size();
	__RPInfo.pAttachments = m_ColourComponentSetup;
	__RPInfo.subpassCount = 1;
	__RPInfo.pSubpasses = &__SubpassDesc;
	__RPInfo.dependencyCount = 1;
	__RPInfo.pDependencies = &__SubpassDependency;
	VkResult __Result = vkCreateRenderPass(g_GPU.gpu,&__RPInfo,nullptr,&render_pass);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create render pass");

	// clear setup memory
	free(m_ColourComponentSetup);
	free(m_ColourComponentReference);
	free(m_DepthComponentSetup);
	free(m_DepthComponentReference);

#else
	u32 __Attachments[m_ColourComponents.size()];
	for (u8 i=0;i<m_ColourComponents.size();i++) __Attachments[i] = GL_COLOR_ATTACHMENT0+i;
	glDrawBuffers(m_ColourComponents.size(),__Attachments);
#endif
}

/**
 *	TODO
 */
void Framebuffer::vanish()
{
#ifdef VKBUILD
	g_GPU.free(render_pass);
#endif
}

/**
 *	clear buffer and start recording process
 */
void Framebuffer::start()
{
#ifdef VKBUILD
	// aquire next command buffer & reset
	cmd_buffer = g_GPU.aquire_command_buffer();
	vkResetCommandBuffer(cmd_buffer->buffer,0);

	// get next swapchain image
	VkResult __Result = vkAcquireNextImageKHR(g_GPU.gpu,g_Frame.swapchain.swapchain,UINT64_MAX,cmd_buffer->ready,
											  VK_NULL_HANDLE,&g_Frame.frame_id);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"available target frame could not be aquired");
	// TODO never write to frame directly in and buffer method

	// start command buffer
	VkCommandBufferBeginInfo __CMDInfo = {  };
	__CMDInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	__CMDInfo.flags = 0;
	__CMDInfo.pInheritanceInfo = nullptr;
	__Result = vkBeginCommandBuffer(cmd_buffer->buffer,&__CMDInfo);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"issue while registering a command");
	// TODO the creation info can be pre-cached instead and then just used based on registration type later

	// setup begin draw
	VkRenderPassBeginInfo __RPBeginInfo = {  };
	__RPBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	__RPBeginInfo.renderPass = render_pass;
	__RPBeginInfo.framebuffer = g_Frame.framebuffers[g_Frame.frame_id];  // TODO aquisition call
	__RPBeginInfo.renderArea.offset = { 0,0 };
	__RPBeginInfo.renderArea.extent = g_Frame.swapchain.extent;
	__RPBeginInfo.clearValueCount = 1;
	__RPBeginInfo.pClearValues = &g_Frame.clear_colour;
	vkCmdBeginRenderPass(cmd_buffer->buffer,&__RPBeginInfo,VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd_buffer->buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,g_Frame.ref_pipeline);
	// TODO very rigid. this expects graphical output, which is kindergarten

	// viewport setup
	vkCmdSetViewport(cmd_buffer->buffer,0,1,&g_Frame.viewport);
	vkCmdSetScissor(cmd_buffer->buffer,0,1,&g_Frame.scissor);
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
	// finish buffer registration
	vkCmdEndRenderPass(cmd_buffer->buffer);
	VkResult __Result = vkEndCommandBuffer(cmd_buffer->buffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to successfully write command buffer");
	// TODO outsource appropriately to pipeline probably

	// submit buffer
	VkPipelineStageFlags __StageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo __SubmitInfo = {  };
	__SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	__SubmitInfo.waitSemaphoreCount = 1;
	__SubmitInfo.pWaitSemaphores = &cmd_buffer->ready;
	__SubmitInfo.pWaitDstStageMask = __StageFlags;
	__SubmitInfo.commandBufferCount = 1;
	__SubmitInfo.pCommandBuffers = &cmd_buffer->buffer;
	__SubmitInfo.signalSemaphoreCount = 1;
	__SubmitInfo.pSignalSemaphores = &g_Frame.render_done[g_Frame.frame_id];
	__Result = vkQueueSubmit(g_GPU.graphical_queue,1,&__SubmitInfo,cmd_buffer->processing);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to submit command buffer");
	// TODO again, using all framebuffers like final render targets does not hold up

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

/**
 *	TODO
 */
void Framebuffer::link_output()
{
	g_Frame.link_result(render_pass);
}
