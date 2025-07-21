#include "buffer.h"


// ----------------------------------------------------------------------------------------------------
// Geometry Buffers


/**
 *	create vertex array
 */
VertexArray::VertexArray()
{
	glGenVertexArrays(1,&m_VAO);
}

/**
 *	bind vertex array
 */
void VertexArray::bind()
{
	glBindVertexArray(m_VAO);
}

/**
 *	unbind all vertex arrays
 */
void VertexArray::unbind()
{
	glBindVertexArray(0);
}


/**
 *	create vertex buffer
 */
VertexBuffer::VertexBuffer()
{
	glGenBuffers(1,&m_VBO);
}

/**
 *	bind vertex buffer
 */
void VertexBuffer::bind()
{
	glBindBuffer(GL_ARRAY_BUFFER,m_VBO);
}

/**
 *	unbind all vertex buffers
 */
void VertexBuffer::unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER,0);
}

/**
 *	upload elements from array into buffer
 *	\param elements: array of optional element indices, mapping the vertex order
 *	\param size: size of array, holding the element indices
 *	NOTE vertex buffer has to be bound beforehand
 */
void VertexBuffer::upload_elements(u32* elements,size_t size)
{
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,size,elements,GL_STATIC_DRAW);
}

/**
 *	upload elements from vector into buffer
 *	\param elements: vector list of optional element indices to upload, mapping the vertex order
 *	NOTE vertex buffer has to be bound beforehand
 */
void VertexBuffer::upload_elements(vector<u32> elements)
{
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,elements.size()*sizeof(u32),&elements[0],GL_STATIC_DRAW);
}


// ----------------------------------------------------------------------------------------------------
// Colour Buffers

s32 _texture_format_channels[] = {
	GL_RGBA,
	GL_RGBA,
	GL_RED
};

s32 _texture_format_internal[] = {
	GL_RGBA,
	GL_SRGB8_ALPHA8,
	GL_RED
};

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
	glTexImage2D(GL_TEXTURE_2D,0,_texture_format_internal[m_Format],width,height,0,
				 _texture_format_channels[m_Format],GL_UNSIGNED_BYTE,data);
	_free();
}

/**
 *	upload data as subtexture to atlas on gpu based on saved x & y axis offset
 *	NOTE has to be uploaded in main thread
 *	NOTE target texture has to be bound and allocated before uploading
 */
void TextureData::gpu_upload_subtexture()
{
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,width,height,_texture_format_channels[m_Format],GL_UNSIGNED_BYTE,data);
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
	glGenTextures(1,&m_Memory);
}

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
 *	NOTE texture should be bound
 *	NOTE this allows generate_mipmap() to be executed if no manual approach is chosen
 */
void Texture::set_texture_parameter_linear_mipmap()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

/**
 *	define bilinear texture filter for mipmap generation
 *	NOTE texture should be bound
 *	NOTE this allows generate_mipmap() to be executed if no manual approach is chosen
 */
void Texture::set_texture_parameter_nearest_mipmap()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

/**
 *	define linear filter without mipmapping
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_linear_unfiltered()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

/**
 *	define filterless pixelraster
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_nearest_unfiltered()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

/**
 *	define extension behaviour to stretch to maximum size
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_clamp_to_edge()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}

/**
 *	define extension behaviour to be scaled towards custom borders, avoiding ratio manipulation
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_clamp_to_border()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
}

/**
 *	repeat texture over vertices without scaling
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_repeat()
{
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

/**
 *	define level of detail through filter bias (will additively shift the perceived lod value)
 *	\param bias: (default .0f) filter bias, addition bias means that .0f is no bias
 *	NOTE texture should be bound
 */
void Texture::set_texture_parameter_filter_bias(float bias)
{
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_LOD_BIAS,bias);
}

/**
 *	define border colour for texture when set_texture_parameter_clamp_to_border() is defined
 *	\param colour: RGBA border colour as vector
 *	NOTE texture should be bound
 *	NOTE pointer trick not field tested, should border colour fail to work this is the most likely culprit
 */
void Texture::set_texture_parameter_border_colour(vec4 colour)
{
	glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,&colour.r);
}

/**
 *	automatically generate mipmap when the appropriate texture parameters are set (_mipmap() suffix)
 *	NOTE texture should be bound
 */
void Texture::generate_mipmap()
{
	glGenerateMipmap(GL_TEXTURE_2D);
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
void GPUPixelBuffer::allocate(u32 width,u32 height,u32 format)
{
	// store info
	dimensions_inv = vec2(1.f/width,1.f/height);

	// allocate memory
	memory_segments.push_back({
			.offset = vec2(0,0),
			.dimensions = vec2(width,height)
		});
	glTexImage2D(GL_TEXTURE_2D,0,format,width,height,0,format,GL_UNSIGNED_BYTE,0);
}
// FIXME inconsistent formatting, the allocation format can easily be detached from texture data structure
// FIXME in case of sRGB colourspace for example the format in internalformat and format won't be the same
//		furthermore this is absolutely necessary to be of dynamic nature, due to monochrome buffers being a thing

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
 *	\param compcount: number of components, that will be defined for this framebuffer
 */
Framebuffer::Framebuffer(u8 compcount)
{
	glGenFramebuffers(1,&m_Buffer);
	m_ColourComponents.resize(compcount);
	glGenTextures(compcount,&m_ColourComponents[0]);
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
	glBindTexture(GL_TEXTURE_2D,m_ColourComponents[index]);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA+0x6f12*fbuffer,width,height,0,GL_RGBA,GL_UNSIGNED_INT+fbuffer,NULL);
	Texture::set_texture_parameter_nearest_unfiltered();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+index,GL_TEXTURE_2D,m_ColourComponents[index],0);
}

/**
 *	depth component definition, only a single one per framebuffer allowed for obvious reasons
 *	\param width: resolution width
 *	\param height: resolution height
 */
void Framebuffer::define_depth_component(f32 width,f32 height)
{
	glGenTextures(1,&m_DepthComponent);
	glBindTexture(GL_TEXTURE_2D,m_DepthComponent);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,width,height,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
	Texture::set_texture_parameter_nearest_unfiltered();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,m_DepthComponent,0);
}

/**
 *	combine previously defined framebuffer attachments
 *	NOTE: this has to happen after definitions of all components
 */
void Framebuffer::finalize()
{
	u32 __Attachments[m_ColourComponents.size()];
	for (u8 i=0;i<m_ColourComponents.size();i++) __Attachments[i] = GL_COLOR_ATTACHMENT0+i;
	glDrawBuffers(m_ColourComponents.size(),__Attachments);
}

/**
 *	clear buffer and start recording process
 */
void Framebuffer::start()
{
	glBindFramebuffer(GL_FRAMEBUFFER,m_Buffer);
	Frame::clear();
}

/**
 *	stop writing to the framebuffer
 */
void Framebuffer::stop()
{
	glBindFramebuffer(GL_FRAMEBUFFER,0);
}

/**
 *	bind colour component to a texture channel
 *	\param channel: texture channel
 *	\param i: colour component index of rendertarget
 */
void Framebuffer::bind_colour_component(u8 channel,u8 i)
{
	Texture::set_channel(channel);
	glBindTexture(GL_TEXTURE_2D,m_ColourComponents[i]);
}

/**
 *	bind depth component to a texture channel
 *	\param channel: texture channel
 */
void Framebuffer::bind_depth_component(u8 channel)
{
	Texture::set_channel(channel);
	glBindTexture(GL_TEXTURE_2D,m_DepthComponent);
}
