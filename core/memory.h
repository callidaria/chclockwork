#ifndef CORE_MEMORY_HEADER
#define CORE_MEMORY_HEADER


#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// Geometry Buffers

class VertexBufferHeap
{
public:
	VertexBufferHeap();
	void upload(size_t offset,size_t size);
	void gpu_upload();
	void vanish();

public:
	VkBuffer buffer;
	MemoryChunkResource mcr = MemoryChunkResource(MEMORY_VBO_HEAP_MEMSIZE);
	void* mem;

private:
	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_Memory,m_StagingMemory;
	vector<VkBufferCopy> m_CopyRegions;
	size_t m_CopyCursor = 0;
};

inline VertexBufferHeap g_VertexBufferHeap = VertexBufferHeap();


class VertexBuffer
{
public:
	void allocate(size_t size);
	void upload(void* vertices,size_t vsize,size_t ofs=0);
	void upload(void* vertices,size_t vsize,void* indices,size_t isize);

#ifndef VKBUILD
	void bind();
#endif

public:
#ifdef VKBUILD
	MemoryRange segment;
	size_t index_offset;
#endif

private:
#ifdef VKBUILD
	u8* m_Data;
#else
	u32 m_VAO,m_VBO;
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
	/*
	void transfer_ownership_read();
	void transfer_ownership_write();
	*/
	void bind();
	void bind_indexed();

private:
	vector<VkBuffer> m_Buffers;
	vector<size_t> m_Offsets;
	size_t m_IndexOffset = 0;
	s16 m_IndexSource = -1;
};
// TODO this is a testing solution, think about dedicated command buffer per target and bind once at creation
//		right now i'm not sure if this is performant or even possible in this case, so i'll leave this todo here
#endif


// ----------------------------------------------------------------------------------------------------
// Texture

enum TextureFormat : u8
{
	TEXTURE_FORMAT_RGBA,
	TEXTURE_FORMAT_SRGB,
	TEXTURE_FORMAT_MONOCHROME,
	TEXTURE_FORMAT_COUNT
};

struct TextureData
{
	// utility
	void load_colour(vec3 colour);
	void load(const char* path);
	void vanish();

	// data
	u8* data;
	u32 x = 0,y = 0;
	s32 width = 0,height = 0;
	bool texture_flag = false;
};

class Texture
{
public:
	// data
	void allocate(u32 wdt,u32 hgt,TextureFormat format);
	void gpu_upload(TextureData& data);
	void gpu_upload_subtexture(TextureData& data,size_t& ofs);

	// utility
	void generate_mipmap();

#ifdef VKBUILD
	void vanish();
	void transition_texture_transfer();
	static void transition_depth_texture(VkImage buffer);
#else
	static void set_channel(u8 i);
	void bind(u8 i);
	static void unbind();
#endif

	static void set_texture_parameter_linear_mipmap();
	static void set_texture_parameter_nearest_mipmap();
	static void set_texture_parameter_linear_unfiltered();
	static void set_texture_parameter_nearest_unfiltered();
	static void set_texture_parameter_clamp_to_edge();
	static void set_texture_parameter_clamp_to_border();
	static void set_texture_parameter_repeat();
	static void set_texture_parameter_filter_bias(float bias=.0f);
	static void set_texture_parameter_border_colour(vec4 colour);

private:
#ifdef VKBUILD
	void _copy_buffer(TextureData& data,size_t& ofs);
#endif

public:
	// handlers
	VkImageView image_view;
	VkSampler sampler;
	// TODO turn private or make a struct?

	// properties
	u32 width,height;
	u16 mipcount;
	bool allocated = false;

private:
#ifdef VKBUILD
	VkImage m_Texture;
	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_TextureMemory,m_StagingMemory;
	TextureFormat m_Format;
#else
	u32 m_Memory;
#endif
};


// ----------------------------------------------------------------------------------------------------
// Font

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
	Rect tex[96];
	Glyph glyphs[96];
	u16 size;
};


// ----------------------------------------------------------------------------------------------------
// Dynamic Texture Atlas

struct GPUPixelBuffer
{
	// setup
	void allocate(u32 wdt,u32 hgt,TextureFormat format,u32 padding=0);

	// utilty
	static void load_texture(GPUPixelBuffer* gpb,Rect* pbc,const char* path);
	static void load_font(GPUPixelBuffer* gpb,Font* font,const char* path,u16 size);
	void gpu_upload(u8 channel=0);
	// TODO allocate & write for statically written texture atlas
	// TODO when allocating, rotation boolean can be stored in alpha by signing the float
	// TODO allow to merge deleted rects when using a dynamic texture atlas

	// texture
	Texture atlas;
	// TODO turn private?

	// subtexture segmentation
	vector<Rect> memory_segments;  // FIXME rects are rational, this shouldn't be more than natural
	InPlaceArray<Rect> textures = InPlaceArray<Rect>(BUFFER_MAXIMUM_TEXTURE_COUNT);
	queue<TextureData> load_requests;
	std::mutex mutex_memory_segments;
	std::mutex mutex_texture_requests;
	ThreadSignal signal;

	// precalculation
	u32 subtex_padding = 0;
	vec2 dimensions_inv;
};


#endif
