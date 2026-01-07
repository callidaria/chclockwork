#ifndef CORE_BUFFER_HEADER
#define CORE_BUFFER_HEADER


#include "blitter.h"


enum BufferType : u8
{
	BUFFER_TYPE_VERTEX,
	BUFFER_TYPE_INDEX,
	BUFFER_TYPE_COUNT
};

enum TextureFormat : u8
{
	TEXTURE_FORMAT_RGBA,
	TEXTURE_FORMAT_SRGB,
	TEXTURE_FORMAT_MONOCHROME,
	TEXTURE_FORMAT_COUNT
};


// ----------------------------------------------------------------------------------------------------
// Rendertarget Colour Buffers

typedef
#ifdef VKBUILD
u32  // TODO texture representation of components
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
	void vanish();  // §§test

	// usage
	void start();  // TODO retire start/stop
	void stop();
	void bind_colour_component(u8 channel,u8 i);
	void bind_depth_component(u8 channel);

#ifdef VKBUILD
	void link_output();
#endif

private:
#ifdef VKBUILD
public:
	VkRenderPass render_pass;
	CommandBuffer* cmd_buffer;
	// TODO switch back to private and somehow add to pipeline?
private:
	VkAttachmentDescription* m_ColourComponentSetup;
	VkAttachmentReference* m_ColourComponentReference;
	VkAttachmentDescription* m_DepthComponentSetup;
	VkAttachmentReference* m_DepthComponentReference;
#else
	u32 m_Buffer;
#endif

	// textures
	vector<__fbuffer_component> m_ColourComponents;
	__fbuffer_component m_DepthComponent;
};
// TODO create pipelines instead of framebuffers! this allows the engine to use the subpass feature
// TODO allocate depth component together with colours, not on-demand. this reduces the allocations by ~half(WC)
// TODO maybe create pipeline feature from this and implement this for ogl version with recursive fb chains


// ----------------------------------------------------------------------------------------------------
// Geometry Buffers

class VertexBuffer
{
public:
	void allocate(size_t size,BufferType type=BUFFER_TYPE_VERTEX);

	// upload
	void upload_vertices(void* vertices);
	void upload_vertices(void* vertices,size_t size);
	void upload_elements(u32* elements,size_t size);
	void upload_elements(vector<u32> elements);

#ifdef VKBUILD
	void vanish();
#else
	void bind();
	void bind_elements();
#endif

private:
	BufferType m_BufferType;
	size_t m_BufferSize;

#ifdef VKBUILD
public:
	VkBuffer vbo;
private:
	VkBuffer m_StagingVBO;
	VkDeviceMemory m_Memory;
	VkDeviceMemory m_StagingMemory;
#else
	u32 m_VBO;
#endif
};

class VertexArray
{
public:
#ifdef VKBUILD
	VertexArray(u8 size);
	void link_buffer(VertexBuffer& vb,u64 offset);
	void bind(Framebuffer& fb);
#else
	VertexArray();
	void bind();
#endif

private:
#ifdef VKBUILD
	vector<VkBuffer> m_Buffers;
	vector<u64> m_Offsets;
#else
	u32 m_VAO;
#endif
};
// TODO this should be used automatically when implementing the vulkan correlation,
//		the struct itself should not be used outside the vertex buffer utility setup for the ogl version!


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
	u32 x = 0,y = 0;
	s32 width = 0,height = 0;
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
