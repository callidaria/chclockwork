#ifndef CORE_MEMORY_HEADER
#define CORE_MEMORY_HEADER


#include "blitter.h"


// ----------------------------------------------------------------------------------------------------
// Geometry Buffers

enum VertexBufferType
{
	VERTEX_BUFFER_TYPE_STATIC,
	VERTEX_BUFFER_TYPE_DYNAMIC,
};


class VertexBufferHeap
{
public:
	VertexBufferHeap(VkBuffer* buffer,size_t size);
	void upload(size_t offset,size_t size);
	void gpu_upload();
	void vanish();
	static void bind(u8 bfrs,size_t vofs,size_t iofs=0);

public:
	MemoryChunkResource mcr = MemoryChunkResource(MEMORY_VBO_HEAP_MEMSIZE);
	void* mem;

private:
	VkBuffer* m_Buffer;
	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_Memory,m_StagingMemory;
	vector<VkBufferCopy> m_CopyRegions;
};

inline VkBuffer g_VertexBuffers[2];
inline VertexBufferHeap g_VertexBufferHeap[2] = {
	VertexBufferHeap(&g_VertexBuffers[0],MEMORY_VBO_HEAP_MEMSIZE),
	VertexBufferHeap(&g_VertexBuffers[1],MEMORY_IBO_HEAP_MEMSIZE)
};


class VertexBuffer
{
public:
	void* allocate(size_t size,VertexBufferType type);
	void upload(void* vertices,size_t vsize,size_t ofs=0);
	void upload(void* vertices,size_t vsize,void* indices,size_t isize);
	void register_upload(size_t size,size_t ofs=0);
	void free();

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
	VertexBufferType m_Type;
#else
	u32 m_VAO,m_VBO;
#endif
};


#ifdef VKBUILD
class VertexArray
{
public:

	// setup
	void register_buffer(const VertexBuffer& vb);
	void register_buffer_dynamic(const VertexBuffer& vb);
	void register_buffer_indexed(const VertexBuffer& vb);

	// update
	void bind();
	//void bind_indexed();

private:
	bool m_Instanced = false;
	size_t m_VOfs = 0,m_IOfs = 0;
	size_t m_IndexOffset = 0;
	s16 m_IndexSource = -1;
};
// TODO this is a testing solution, think about dedicated command buffer per target and bind once at creation
//		right now i'm not sure if this is performant or even possible in this case, so i'll leave this todo here
#endif


// ----------------------------------------------------------------------------------------------------
// Texture

enum TextureSourceType : u8
{
	TEXTURE_SOURCE_TYPE_TEXTURE,
	TEXTURE_SOURCE_TYPE_CUBEMAP,
	TEXTURE_SOURCE_TYPE_COUNT
};

enum TextureFormat : u8
{
	TEXTURE_FORMAT_RGBA,
	TEXTURE_FORMAT_SRGB,
	TEXTURE_FORMAT_RGBu11,
	TEXTURE_FORMAT_MONOCHROME,
	TEXTURE_FORMAT_COUNT
};

enum TextureFeatures : u8
{
	TEXTURE_FEATURE_STANDARD,
	TEXTURE_FEATURE_SUPPRESS_MIPMAP_BIT = 1,
	TEXTURE_FEATURE_CUBEMAP_BIT = 1<<1,
};

enum TexDataType : u8
{
	TEX_DATA_TYPE_OVERWRITTEN,
	TEX_DATA_TYPE_INTEGER,
	TEX_DATA_TYPE_FLOAT,
	TEX_DATA_TYPE_COUNT
};

union TexDataArr
{
	u8* intbfr;
	u32* floatbfr;
};

struct TextureData
{
	// utility
	void load(vec3 colour);
	void load(const char* path);
	void load_hdr(const char* path);
	void vanish();

	// data
	TexDataArr data;
	TexDataType type = TEX_DATA_TYPE_OVERWRITTEN;
	u32 x = 0,y = 0;
	s32 width = 0,height = 0;
};

class Texture;

struct TextureDataTuple
{
	TextureData data;
	Texture* texture;
	TextureFormat format;
	TextureFeatures features;
};

class Texture
{
public:
	// data
	void allocate(u32 wdt,u32 hgt,TextureFormat format,TextureFeatures feat=TEXTURE_FEATURE_STANDARD);
	void register_subtexture(TextureData& data);

	// load
	static void load_colour(Texture* texture,vec3 colour,TextureFormat format,
							ThreadedQueue<TextureDataTuple>* q);
	static void load_texture(Texture* texture,const char* path,TextureFormat format,TextureFeatures feat,
							 ThreadedQueue<TextureDataTuple>* q);
	void gpu_upload();
	void gpu_upload(TextureData& data);
	void gpu_upload(TextureData* data,u8 size);

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

	// gl parameters
	static void set_texture_parameter_linear_mipmap();
	static void set_texture_parameter_nearest_mipmap();
	static void set_texture_parameter_linear_unfiltered();
	static void set_texture_parameter_nearest_unfiltered();
	static void set_texture_parameter_clamp_to_edge();
	static void set_texture_parameter_clamp_to_border();
	static void set_texture_parameter_repeat();
	static void set_texture_parameter_filter_bias(float bias=.0f);
	static void set_texture_parameter_border_colour(vec4 colour);
#endif

#ifdef VKBUILD
private:
	void _setup_buffer_copy(size_t& ofs,TextureData& data,size_t arr_index=0);
#endif

public:
	// handlers
	VkImageView image_view;
	VkSampler sampler;
	// TODO turn private or make a struct?

	// properties
	u32 width,height;
	u16 layercount,mipcount;
	bool allocated = false;

private:
#ifdef VKBUILD
	VkImage m_Texture;
	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_TextureMemory,m_StagingMemory;
	TextureFormat m_Format;
	vector<VkBufferImageCopy> m_TextureCopies;
	size_t m_UploadCursor = 0;
	void* m_Data;
	bool m_ActiveAtlas = true;
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
	void allocate(u32 wdt,u32 hgt,TextureFormat format,
				  TextureFeatures feat=TEXTURE_FEATURE_STANDARD,u32 padding=0);

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


// ----------------------------------------------------------------------------------------------------
// Cubemap

// obviously component order for classical cubemap sampler upload matters in visualization. this is the order.
enum CubemapComponentChannel : u8
{
	CUBEMAP_COMPONENT_CHANNEL_POSX,
	CUBEMAP_COMPONENT_CHANNEL_NEGX,
	CUBEMAP_COMPONENT_CHANNEL_POSY,
	CUBEMAP_COMPONENT_CHANNEL_NEGY,
	CUBEMAP_COMPONENT_CHANNEL_POSZ,
	CUBEMAP_COMPONENT_CHANNEL_NEGZ,
	CUBEMAP_COMPONENT_CHANNEL_COUNT
};

// classical cubemap, texture to upload as samplerCube in shader
struct Cubemap
{
	// utility
	static void load(Cubemap* cubemap,const char* path,TextureFormat format,ThreadedQueue<Cubemap*>* q);
	void load_colour(vec3 colour);
	void load_YUpLH(const char* path,TextureFormat fmt);
	void gpu_upload();
	void vanish();

	// data
	Texture cubemap;
	TextureData data[CUBEMAP_COMPONENT_CHANNEL_COUNT];
	TextureFormat format;
};
