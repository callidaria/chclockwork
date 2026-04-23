#ifndef CORE_MEMORY_HEADER
#define CORE_MEMORY_HEADER


#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// Geometry Buffers

class VertexBuffer
{
public:
	void allocate(size_t size,bool indexed=false);
	void upload(void* vertices,size_t vsize);
	void upload(void* vertices,size_t vsize,void* indices,size_t isize);

#ifdef VKBUILD
	void update();
	void free();
	void vanish();
#else
	void bind();
#endif

public:

#ifdef VKBUILD
	VkBuffer vbo;
	size_t index_offset;
#endif

private:

#ifdef VKBUILD
	VkBuffer m_StagingVBO;
	VkDeviceMemory m_Memory;
	VkDeviceMemory m_StagingMemory;
	void* m_Data;
	VkBufferCopy m_BufferCopy = {  };
	CommandBufferTRF* m_CMDBuffer;  // TODO evaluate if this is good and use it for both vb and fb
#else
	u32 m_VAO;
	u32 m_VBO;
#endif
};


#ifdef VKBUILD
class VertexArray
{
public:

	// setup
	void allocate(u8 size);
	void register_buffer(const VertexBuffer& vb);
	void register_buffer_dynamic(const VertexBuffer& vb);
	void register_buffer_indexed(const VertexBuffer& vb);

	// update
	void transfer_ownership();
	void bind();
	void bind_indexed();

private:
	vector<VkBuffer> m_Buffers;
	vector<VkBufferMemoryBarrier> m_Barriers;
	vector<size_t> m_Offsets;
	size_t m_IndexOffset = 0;
	s16 m_IndexSource = -1;
};
// TODO this is a testing solution, think about dedicated command buffer per target and bind once at creation
//		right now i'm not sure if this is performant or even possible in this case, so i'll leave this todo here
#endif


// ----------------------------------------------------------------------------------------------------
// Colour Buffers

enum TextureFormat : u8
{
	TEXTURE_FORMAT_RGBA,
	TEXTURE_FORMAT_SRGB,
	TEXTURE_FORMAT_MONOCHROME,
	TEXTURE_FORMAT_COUNT
};

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
	u32 x = 0,y = 0;
	s32 width = 0,height = 0;
#ifdef VKBUILD
	u16 mipcount;
#endif
	u8* data;

private:
	TextureFormat m_Format = TEXTURE_FORMAT_RGBA;
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
#ifdef VKBUILD
	void load_texture(const char* path);
	void load_font(const char* path,u16 size);
	void vanish();
#endif

	// utilty
	void allocate(u32 width,u32 height,TextureFormat format);
	static void load_texture(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,const char* path);
	static void load_font(GPUPixelBuffer* gpb,Font* font,const char* path,u16 size);
	static void _load(GPUPixelBuffer* gpb,PixelBufferComponent* pbc,TextureData* data);
	void gpu_upload(u8 channel);
	// TODO allocate & write for statically written texture atlas
	// TODO when allocating, rotation boolean can be stored in alpha by signing the float
	// TODO allow to merge deleted rects when using a dynamic texture atlas
	// FIXME format can be assigned when allocating but load instructions are format dependent

	// data
#ifdef VKBUILD
	VkBuffer m_StagingBuffer;
	VkImage m_Texture;
	VkDeviceMemory m_StagingMemory;  // TODO remove staging memory & buffer from here
	VkDeviceMemory m_TextureMemory;
	VkImageView image_view;
	VkSampler sampler;
#endif
	// FIXME wait just a second GPUPixelBuffer is a struct and i act as if it's a class. obey the coding laws!

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


#endif
