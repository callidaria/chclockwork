#ifndef CORE_SHADER_HEADER
#define CORE_SHADER_HEADER


#include "gpu_interface.h"
#include "memory.h"


constexpr u32 SHADER_ERROR_LOGGING_LENGTH = 512;
constexpr size_t SHADER_UPLOAD_VALUE_SIZE = sizeof(f32);


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

#ifdef VKBUILD
enum DescriptorType : u8
{
	DESCRIPTOR_TYPE_BUFFER,
	DESCRIPTOR_TYPE_IMAGE
};

struct DescriptorInfo
{
	DescriptorType type;
	union
	{
		VkDescriptorBufferInfo buffer;
		VkDescriptorImageInfo image;
	} info;
};

struct DescriptorSets
{
	VkDescriptorSet data;
	VkDescriptorSet textures;
};

constexpr u8 UNIFORM_DESCRIPTOR_SET_COUNT = sizeof(DescriptorSets)/sizeof(VkDescriptorSet);

class UniformBuffer
{
public:
	UniformBuffer(u32 binding_count);

	// setup
	void define_geometry_buffer(u32 location,size_t size);
	size_t define_pixel_buffer(u32 location,VkDescriptorType type);
	void link_result(size_t i,GPUPixelBuffer& texture);
	void link_result(size_t i,VkImageView buffer);
	void link_texture(size_t i,GPUPixelBuffer* texture);
	void assemble();
	void finalize();

	// action
	void update(void* data,size_t size);

	// final
	void vanish();

public:
	VkDescriptorSetLayout dset_layout,dset_layout_textures;
	DescriptorSets m_DSets[GPU_BUFFER_COUNT];  // TODO move this out of public

private:
	VkBuffer m_UBO[GPU_BUFFER_COUNT];
	VkDeviceMemory m_UBOMemory[GPU_BUFFER_COUNT];
	void* m_UBOMapped[GPU_BUFFER_COUNT];
	VkDescriptorPool m_DescriptorPool;
	vector<VkDescriptorPoolSize> m_PSizes;
	vector<VkDescriptorSetLayoutBinding> m_Bindings;
	vector<VkWriteDescriptorSet> m_Writes;
	vector<DescriptorInfo> m_DescriptorInfos;
	VkImage m_PlaceholderImage;
	VkDeviceMemory m_PlaceholderMemory;
	VkImageView m_PlaceholderTexture;
	VkSampler m_DefaultSampler;
	size_t m_Size = 0;

	// texture set
	VkDescriptorImageInfo m_MeshTextureInfo[RENDERER_MAXIMUM_TEXTURE_COUNT];
	VkWriteDescriptorSet m_MeshTextureSet = {  };
};
inline UniformBuffer g_UniformBuffer = UniformBuffer(5);
// TODO definition through config or something else, that the developer is capable to easily find & change
#endif


// ----------------------------------------------------------------------------------------------------
// Shader Pipeline

struct ShaderAttribute
{
#ifdef VKBUILD
	u32
#else
	string
#endif
	location;
	size_t offset;
	u8 dim;
};

struct ShaderInterface
{
	vector<ShaderAttribute> vbo_attribs;
	vector<ShaderAttribute> ibo_attribs;
	size_t vbo_width = 0;
	size_t ibo_width = 0;
};

#ifndef VKBUILD
class Shader
{
public:
	static u32 compile(const char* path,GLenum type);
};
#endif

class VertexShader
{
public:
	VertexShader() {  }  // TODO remove this after pointing to the correct shader instead of copy
	VertexShader(const char* path);

public:
	u32 shader;
	ShaderInterface interface;
};

class FragmentShader
{
public:
	FragmentShader() {  }  // TODO remove this after pointing to the correct shader instead of copy
	FragmentShader(const char* path);

public:
	u32 shader;
	vector<string> sampler_attribs;
};


enum UniformDimension : u8
{
	SHADER_UNIFORM_FLOAT,
	SHADER_UNIFORM_VEC2,
	SHADER_UNIFORM_VEC3,
	SHADER_UNIFORM_VEC4,
	SHADER_UNIFORM_MAT44
};

struct ShaderUniformValue
{
	string name;
	u32 uloc;
	UniformDimension udim;
	f32* data;
};

class ShaderPipeline
{
public:
	ShaderPipeline(u8 bfr_count,bool depth=false);

	// definition
	u8 out_define_colour_buffer(bool floatbuffer=false);
	u8 out_define_result_buffer();
	// TODO somehow autodefine those by shader analysis? but there is a problem with result specification!

	// assembly
	void assemble(const char* vs,const char* fs,bool flipped=false,bool pconstants=false);
	void assemble(VertexShader vs,FragmentShader fs);
	//void map(u16 channel);
	void vanish();

	// usage
	void enable();
	static void disable();
	u32 get_uniform_location(const char* uname);

	// upload
	void upload(const char* varname,s32 value);
	void upload(const char* varname,f32 value);
	void upload(const char* varname,vec2 value);
	void upload(const char* varname,vec3 value);
	void upload(const char* varname,vec4 value);
	void upload(const char* varname,mat4 value);
	void upload(const char* varname,UniformDimension dim,f32* data);
	void upload(ShaderUniformValue& uniform);
	void upload_coordinate_system();
	void upload_camera();
	void upload_camera(Camera3D& c);

private:
#ifdef VKBUILD
	void _define_colour_component(u8 index,VkFormat format,bool result=false);
#else
	void _define_attribute(ShaderAttribute attrib);
	void _define_index_attribute(ShaderAttribute attrib);
	s32 _handle_attribute_location_by_name(const char* varname);
	// TODO change back to references
#endif

public:
#ifdef VKBUILD
	VkPipeline pipeline;
	VkPipelineLayout pipeline_layout;
	VkRenderPass render_pass;
	VkAttachmentDescription* descriptions;
	BitwiseWords result_attachment;
#else
#endif
	u8 depth_channel;
	bool has_depth;

private:
#ifdef VKBUILD
	VkAttachmentReference* m_References;
	u8 m_Cursor = 0;
#else
	u32 m_ShaderProgram;
#endif

	// shader components
	VertexShader m_VertexShader;
	FragmentShader m_FragmentShader;

	// working iteration
	size_t m_VertexCursor = 0;
	size_t m_IndexCursor = 0;
};


struct ShaderUniformUpload
{
	void correlate(ShaderUniformUpload& uniform);
	void upload();

	// unform attachments
	void attach_uniform(const char* name,f32* var);
	void attach_uniform(const char* name,vec2* var);
	void attach_uniform(const char* name,vec3* var);
	void attach_uniform(const char* name,vec4* var);
	void attach_uniform(const char* name,mat4* var);
	void attach_uniform(const char* name,UniformDimension dim,f32* var);
	// TODO templating?

private:
	ShaderUniformValue& _attach_variable(const char* name);

public:
	lptr<ShaderPipeline> shader;
	vector<ShaderUniformValue> uploads;
};
// TODO deprecated this is not the way to go anymore since vk port


#endif
