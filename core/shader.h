#ifndef CORE_SHADER_HEADER
#define CORE_SHADER_HEADER


#include "gpu_interface.h"
#include "memory.h"


constexpr u32 SHADER_ERROR_LOGGING_LENGTH = 512;
// constexpr size_t SHADER_UPLOAD_VALUE_SIZE = sizeof(f32);


#ifdef VKBUILD

// ----------------------------------------------------------------------------------------------------
// Tuples & States

enum DescriptorType : u8
{
	DESCRIPTOR_TYPE_BUFFER,
	DESCRIPTOR_TYPE_IMAGE
};

enum UniformDimension : u8
{
	SHADER_UNIFORM_UNDEFINED,
	SHADER_UNIFORM_UINT,
	SHADER_UNIFORM_INT,
	SHADER_UNIFORM_FLOAT,
	SHADER_UNIFORM_VEC2,
	SHADER_UNIFORM_VEC3,
	SHADER_UNIFORM_VEC4,
	SHADER_UNIFORM_MAT44,
	SHADER_UNIFORM_TEXTURE,
	SHADER_UNIFORM_FORMAT_COUNT
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

struct ShaderAttribute
{
#ifdef VKBUILD
	u32
#else
	string
#endif
	location;
	size_t offset;
	UniformDimension dim;
};

struct ShaderUniformValue
{
	string name;
	u32 uloc;
	UniformDimension udim;
	f32* data;
};

struct UBOAttribute
{
	VkDescriptorType type;
	VkShaderStageFlags stage = 0;
	size_t offset = 0,memsize = 0;
};

struct ShaderInterface
{
	vector<ShaderAttribute> vbo_attribs;
	vector<ShaderAttribute> ibo_attribs;
	vector<map<u32,UBOAttribute>> ubo_attribs;
	size_t vbo_width = 0;
	size_t ibo_width = 0;
	size_t ubo_width = 0;
	size_t pc_count = 0,pc_memsize = 0;
};


// ----------------------------------------------------------------------------------------------------
// Descriptor Memory

struct DescriptorSetMemory
{
	// interaction
	void define(u32 location,UBOAttribute& attr);
	void link_result(size_t location,size_t offset,size_t size);
	void link_result(size_t location,GPUPixelBuffer& texture);
	void link_result(size_t location,VkImageView buffer);

	// state
	void allocate(u8 set,size_t size,vector<VkDescriptorSetLayout>& layouts);
	void bind(VkPipelineLayout& layout);
	void update();
	void update_frame();

private:
	VkDescriptorSet m_DSets[GPU_BUFFER_COUNT];
	vector<VkWriteDescriptorSet> m_Writes;
	vector<DescriptorInfo> m_DescriptorInfos;
	map<size_t,size_t> m_LocationIndexCorrelation;
	u8 m_Set;
};


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer Memory

class UniformBuffer
{
public:
	UniformBuffer();
	void update(void* data,size_t size);
	void vanish();

public:
	GPUPixelBuffer default_texture;
	VkSampler default_sampler;
	VkDescriptorPool descriptor_pool;
	VkBuffer ubo[GPU_BUFFER_COUNT];

private:
	VkDeviceMemory m_UBOMemory[GPU_BUFFER_COUNT];
	void* m_UBOMapped[GPU_BUFFER_COUNT];
};
inline UniformBuffer g_UniformBuffer = UniformBuffer();
#endif


// ----------------------------------------------------------------------------------------------------
// Shader Pipeline

#ifdef GLBUILD
class Shader
{
public:
	static u32 compile(const char* path,GLenum type);
};

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
#endif


class ShaderPipeline
{
public:
	ShaderPipeline(u8 bfr_count,bool depth=false);

	// definition
	u8 out_define_colour_buffer(bool floatbuffer=false);
	u8 out_define_result_buffer();
	// TODO somehow autodefine those by shader analysis? but there is a problem with result specification!

	// assembly
	void assemble(const char* vs,const char* fs,bool flipped=false);
#ifdef GLBUILD
	void assemble(VertexShader vs,FragmentShader fs);
#endif
	//void map(u16 channel);
	void vanish();

	// usage
	void enable();
	static void disable();
	u32 get_uniform_location(const char* uname);

	// ubo & pcm
	void generate_ubo(vector<DescriptorSetMemory>& sets);
	void generate_pcm(void* pcm,u32 repeat=1);
	void upload_pcm(void* pcm,u32 ofs=0);

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
	ShaderInterface m_Interface;
	vector<VkDescriptorSetLayout> m_DSetLayouts;
	VkAttachmentReference* m_References;
	u8 m_Cursor = 0;
#else
	VertexShader m_VertexShader;
	FragmentShader m_FragmentShader;
	u32 m_ShaderProgram;
#endif

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
