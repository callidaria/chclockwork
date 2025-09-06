#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER


#include "base.h"
#include "blitter.h"

enum MemoryFormat : u8
{
	MEMORY_FORMAT_STATIC,
	MEMORY_FORMAT_QUICK,
	MEMORY_FORMAT_COUNT
};

enum TextureFormat : u8
{
	TEXTURE_FORMAT_RGBA,
	TEXTURE_FORMAT_SRGB,
	TEXTURE_FORMAT_MONOCHROME,
	TEXTURE_FORMAT_COUNT
};


// ----------------------------------------------------------------------------------------------------
// Geometry Buffers

class VertexArray
{
public:
	VertexArray();

	void bind();
	static void unbind();

private:
	u32 m_VAO;
};
// TODO this should be used automatically when implementing the vulkan correlation,
//		the struct itself should not be used outside the vertex buffer utility setup for the ogl version!

#ifndef VKBUILD
static inline GLenum _memory_formats[MEMORY_FORMAT_COUNT] = {
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW
};
#endif
// TODO this does not belong in the header, this will be moved to implementation later
// TODO add more types & correlate with vulkan setup

class VertexBuffer
{
public:
	VertexBuffer();

	void bind();
	void bind_elements();
	static void unbind();
	static void unbind_elements();

	/**
	 *	template inline for dynamic vertex struct uploads
	 *	\param vertices: vertex array/vector holding geometry
	 *	\param size: array size, not necessary when using a vector
	 *	\param memtype: GL_(STREAM+STATIC+DYNAMIC)_(DRAW+READ+COPY)
	 *	NOTE vertex buffer has to be bound beforehand
	 *	NOTE do not use in combination with upload_elements(...)
	 */
	template<typename T> inline void upload_vertices(T* vertices,
													 size_t size,MemoryFormat memtype=MEMORY_FORMAT_STATIC)
	{
#ifdef VKBUILD
		// TODO
#else
		glBufferData(GL_ARRAY_BUFFER,size*sizeof(T),vertices,_memory_formats[memtype]);
#endif
	}
	template<typename T> inline void upload_vertices(vector<T> vertices,
													 MemoryFormat memtype=MEMORY_FORMAT_STATIC)
	{
#ifdef VKBUILD
		// TODO
#else
		glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(T),&vertices[0],_memory_formats[memtype]);
#endif
	}
	// TODO move this out of the header & find a different upload approach

	void upload_elements(u32* elements,size_t size);
	void upload_elements(vector<u32> elements);

private:
	u32 m_VBO;
};


// ----------------------------------------------------------------------------------------------------
// Colour Buffers


struct TextureData
{
public:
	TextureData(TextureFormat format=TEXTURE_FORMAT_RGBA);

	void load(const char* path);
	void gpu_upload();
	void gpu_upload_subtexture();

private:
	void _free();

public:
	u32 x,y;
	s32 width,height;
	u8* data;

private:
	TextureFormat m_Format;
	bool m_TextureFlag = false;
};

class Texture
{
public:
	Texture();

	static void set_channel(u8 i);
	void bind(u8 i);
	static void unbind();

	static void set_texture_parameter_linear_mipmap();
	static void set_texture_parameter_nearest_mipmap();
	static void set_texture_parameter_linear_unfiltered();
	static void set_texture_parameter_nearest_unfiltered();
	static void set_texture_parameter_clamp_to_edge();
	static void set_texture_parameter_clamp_to_border();
	static void set_texture_parameter_repeat();
	static void set_texture_parameter_filter_bias(float bias=.0f);
	static void set_texture_parameter_border_colour(vec4 colour);
	static void generate_mipmap();

private:
	u32 m_Memory;
};


struct PixelBufferComponent
{
	vec2 offset = vec2(0,0);
	vec2 dimensions = vec2(0,0);
};

struct Glyph
{
	vec2 scale;
	vec2 bearing;
	s64 advance;
};

struct Font
{
	// utility
	f32 estimate_wordlength(string& word,u32 offset=0);

	// data
	PixelBufferComponent tex[96];
	Glyph glyphs[96];
	u16 size;
};

struct GPUPixelBuffer
{
	// utilty
	void allocate(u32 width,u32 height,TextureFormat format);
	static void load_texture(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,const char* path);
	static void load_font(GPUPixelBuffer* gpb,Font* font,const char* path,u16 size);
	static void _load(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,TextureData* data);
	void gpu_upload(u8 channel,std::chrono::steady_clock::time_point& fstart);
	// TODO allocate & write for statically written texture atlas
	// TODO when allocating, rotation boolean can be stored in alpha by signing the float
	// TODO allow to merge deleted rects when using a dynamic texture atlas
	// FIXME format can be assigned when allocating but load instructions are format dependent

	// data
	Texture atlas;
	vec2 dimensions_inv;
	vector<PixelBufferComponent> memory_segments;
	std::mutex mutex_memory_segments;
	InPlaceArray<PixelBufferComponent> textures
			= InPlaceArray<PixelBufferComponent>(BUFFER_MAXIMUM_TEXTURE_COUNT);
	std::mutex mutex_texture_requests;
	queue<TextureData> load_requests;
	ThreadSignal signal;
};


// ----------------------------------------------------------------------------------------------------
// Rendertarget Colour Buffers

typedef
#ifdef VKBUILD
VkAttachmentDescription
#else
u32
#endif
__fbuffer_component;

class Framebuffer
{
public:
	Framebuffer(u8 count);
	void define_colour_component(u8 index,f32 width,f32 height,bool fbuffer=false);
	void define_depth_component(f32 width,f32 height);
	void finalize();

	// usage
	void start();
	static void stop();
	void bind_colour_component(u8 channel,u8 i);
	void bind_depth_component(u8 channel);

private:
#ifdef VKBUILD
	VkRenderPass m_RenderPass;
#else
	u32 m_Buffer
#endif
	__fbuffer_component* m_ColourComponents;
	__fbuffer_component* m_DepthComponent;
	// TODO make this a pointer
};


#endif
