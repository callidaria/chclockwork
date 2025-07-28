#ifndef CORE_SHADER_HEADER
#define CORE_SHADER_HEADER


#include "base.h"
#include "buffer.h"


constexpr u32 SHADER_ERROR_LOGGING_LENGTH = 512;
constexpr size_t SHADER_UPLOAD_VALUE_SIZE = sizeof(float);


struct ShaderAttribute
{
	u8 dim;
	string name;
};

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
	vector<ShaderAttribute> vbo_attribs;
	vector<ShaderAttribute> ibo_attribs;
	size_t vbo_width = 0;
	size_t ibo_width = 0;

private:
	vector<ShaderAttribute>* write_head = &vbo_attribs;
	size_t* width_head = &vbo_width;
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
	u32 uloc;
	UniformDimension udim;
	f32* data;
};

class ShaderPipeline
{
public:
	ShaderPipeline() {  }
	void assemble(VertexShader vs,FragmentShader fs);
	void map(u16 channel,VertexBuffer* vbo,VertexBuffer* ibo=nullptr);

	// usage
	void enable();
	static void disable();
	u32 get_uniform_location(const char* uname);

	// upload
	void upload(const char* varname,UniformDimension dim,f32* data);
	void upload(ShaderUniformValue& uniform);
	void upload(const char* varname,s32 value);
	void upload(const char* varname,f32 value);
	void upload(const char* varname,vec2 value);
	void upload(const char* varname,vec3 value);
	void upload(const char* varname,vec4 value);
	void upload(const char* varname,mat4 value);
	void upload_coordinate_system();
	void upload_camera();
	void upload_camera(Camera3D& c);

	void _define_attribute(ShaderAttribute attrib);
	void _define_index_attribute(ShaderAttribute attrib);
	// TODO change back to references
private:
	s32 _handle_attribute_location_by_name(const char* varname);

private:

	// program
	u32 m_ShaderProgram;
	VertexShader m_VertexShader;
	FragmentShader m_FragmentShader;

	// working iteration
	size_t m_VertexCursor = 0;
	size_t m_IndexCursor = 0;
};




class ShaderUniformUpload
{
public:
	ShaderUniformUpload(lptr<ShaderPipeline> shader);
	void upload();

	// unform attachments
	void attach_uniform(const char* name,f32* var);
	void attach_uniform(const char* name,vec2* var);
	void attach_uniform(const char* name,vec3* var);
	void attach_uniform(const char* name,vec4* var);
	void attach_uniform(const char* name,mat4* var);
	// TODO templating?

private:
	lptr<ShaderPipeline> m_Shader;
	vector<ShaderUniformValue> m_Uploads;
};


#endif
