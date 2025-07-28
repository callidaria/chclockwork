#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Shaders

/**
 *	compile given shader program
 *	\param path: path to shader program (can be vertex, fragment or geometry)
 *	\param type: shader type GL_(VERTEX+GEOMETRY+FRAGMENT)
 *	\returns compiled shader pipeline fragment
 */
u32 Shader::compile(const char* path,GLenum type)
{
	COMM_AWT("compiling shader: %s",path);

	// open shader source
	std::ifstream __File(path);
	if (!__File)
	{
		COMM_ERR("no shader found at path: %s",path);
		return 0;
	}

	// read shader source
	string __SourceRaw;
	string __Line;
	while (!__File.eof())
	{
		std::getline(__File,__Line);
		__SourceRaw.append(__Line+'\n');
	}
	const char* __SourceCompile = __SourceRaw.c_str();
	__File.close();

	// compile shader
	u32 shader = glCreateShader(type);
	glShaderSource(shader,1,&__SourceCompile,NULL);
	glCompileShader(shader);

	// compile error log
#ifdef DEBUG
	int __Status;
	glGetShaderiv(shader,GL_COMPILE_STATUS,&__Status);
	if (!__Status)
	{
		char log[SHADER_ERROR_LOGGING_LENGTH];
		glGetShaderInfoLog(shader,SHADER_ERROR_LOGGING_LENGTH,NULL,log);
		COMM_ERR("[SHADER] %s -> %s",path,log);
	}
#endif

	COMM_CNF();
	return shader;
}

/**
 *	create a vertex shader from source
 *	\param path: path to GLSL vertex source file
 */
VertexShader::VertexShader(const char* path)
{
	shader = Shader::compile(path,GL_VERTEX_SHADER);
	if (!shader)
	{
		COMM_ERR("[SHADER] skipping input parser, vertex shader is corrupted");
		return;
	}

	// assess input pattern for vbo/ibo automapping
	std::ifstream __File(path);
	string __Line;
	while (!__File.eof())
	{
		std::getline(__File,__Line);
		if (__Line.find("// engine: ibo")==0)
		{
			write_head = &ibo_attribs;
			width_head = &ibo_width;
			continue;
		}
		else if (__Line.find("in")!=0) continue;
		else if (__Line.find("void main()")==0) break;

		// extract input information
		vector<string> tokens;
		split_words(tokens,__Line);
		tokens[2].pop_back();

		// interpret input definition line
		u8 dim = (tokens[1]=="float") ? 1 : tokens[1][3]-0x30;
		write_head->push_back({ dim,tokens[2] });
		(*width_head) += dim;
	}

	// convert widths to byte format
	vbo_width *= SHADER_UPLOAD_VALUE_SIZE;
	ibo_width *= SHADER_UPLOAD_VALUE_SIZE;
}

/**
 *	create a fragment shader from source
 *	\param path: path to GLSL fragment source file
 */
FragmentShader::FragmentShader(const char* path)
{
	shader = Shader::compile(path,GL_FRAGMENT_SHADER);
	if (!shader)
	{
		COMM_ERR("[SHADER] skipping sample mapping, fragment shader is corrupted");
		return;
	}

	// grind fragment shader for texture
	std::ifstream __File(path);
	string __Line;
	while(!__File.eof())
	{
		std::getline(__File,__Line);
		if (__Line.find("uniform sampler2D")!=0) continue;
		else if (__Line.find("void main()")==0) break;

		// extract sampler variables
		vector<string> tokens;
		split_words(tokens,__Line);
		tokens[2].pop_back();
		sampler_attribs.push_back(tokens[2]);
	}
}


// ----------------------------------------------------------------------------------------------------
// Pipelines

/**
 *	assemble shader pipeline from compiled shaders
 *	pipeline flow: vertex shader -> (geometry shader) -> fragment shader
 *	\param vs: compiled vertex shader
 *	\param fs: compiled fragment shader
 */
void ShaderPipeline::assemble(VertexShader vs,FragmentShader fs)
{
	m_VertexShader = vs;
	m_FragmentShader = fs;
	// FIXME this CAN and SHOULD be critisized! awful memory management through heavy copy!

	// assemble program
	m_ShaderProgram = glCreateProgram();
	glAttachShader(m_ShaderProgram,vs.shader);
	glAttachShader(m_ShaderProgram,fs.shader);
	glLinkProgram(m_ShaderProgram);
}

/**
 *	automatically map vertex and index buffer object to vertex shader input
 *	\param channel: starting texture channel
 *	\param vbo: vertex buffer object
 *	\param ibo: (default nullptr) index buffer object
 *	NOTE vertex buffer needs to be active
 */
void ShaderPipeline::map(u16 channel,VertexBuffer* vbo,VertexBuffer* ibo)
{
	// vertex buffer
	COMM_LOG("mapping shader (vbo = %lu:%lu,ibo = %lu:%lu) utilizing %lu texture channels",
			 m_VertexShader.vbo_attribs.size(),m_VertexShader.vbo_width,
			 m_VertexShader.ibo_attribs.size(),m_VertexShader.ibo_width,
			 m_FragmentShader.sampler_attribs.size()
		);
	enable();
	for (ShaderAttribute& attrib : m_VertexShader.vbo_attribs) _define_attribute(attrib);
	m_VertexCursor = 0;

	// texture mapping
	for (u16 i=0;i<m_FragmentShader.sampler_attribs.size();i++)
		upload(m_FragmentShader.sampler_attribs[i].c_str(),channel+i);

	// index buffer
	if (ibo==nullptr||!m_VertexShader.ibo_attribs.size()) return;
	ibo->bind();
	for (ShaderAttribute& attrib : m_VertexShader.ibo_attribs) _define_index_attribute(attrib);
	m_IndexCursor = 0;
}

/**
 *	enable/disable shader pipeline
 */
void ShaderPipeline::enable() { glUseProgram(m_ShaderProgram); }
void ShaderPipeline::disable() { glUseProgram(0); }

/**
 *	extract uniform location from shader program
 *	\param uname: literal uniform variable name in shader program
 *	\returns uniform location
 */
u32 ShaderPipeline::get_uniform_location(const char* uname)
{
	return glGetUniformLocation(m_ShaderProgram,uname);
}

// uniform variable upload function correlation map
typedef void (*uniform_upload)(u16,f32*);
void _upload1f(u16 uloc,f32* data) { glUniform1f(uloc,data[0]); }
void _upload2f(u16 uloc,f32* data) { glUniform2f(uloc,data[0],data[1]); }
void _upload3f(u16 uloc,f32* data) { glUniform3f(uloc,data[0],data[1],data[2]); }
void _upload4f(u16 uloc,f32* data) { glUniform4f(uloc,data[0],data[1],data[2],data[3]); }
void _upload4m(u16 uloc,f32* data) { glUniformMatrix4fv(uloc,1,GL_FALSE,data); }
uniform_upload uploadf[] = { _upload1f,_upload2f,_upload3f,_upload4f,_upload4m };

/**
 *	upload float uniform variable to shader by variable name
 *	\param varname: uniform variable name
 *	\param dim: uniform dimension
 *	\param data: pointer to data, that will be uploaded to uniform variable
 *	NOTE shader pipeline needs to be active to upload values to uniform variables
 */
void ShaderPipeline::upload(const char* varname,UniformDimension dim,f32* data)
{
	uploadf[dim](get_uniform_location(varname),data);
}

/**
 *	upload float uniform variable to shader
 *	\param uniform: uniform value reference, location & dimension
 *	NOTE shader pipeline needs to be active to upload values to uniform variables
 */
void ShaderPipeline::upload(ShaderUniformValue& uniform)
{
	uploadf[uniform.udim](uniform.uloc,uniform.data);
}

/**
 *	upload uniform variable to shader
 *	\param varname: variable name as defined as "uniform" in shader (must be part of the pipeline)
 *	\param value: value to upload to specified variable
 *	NOTE shader pipeline needs to be active to upload values to uniform variables
 */
void ShaderPipeline::upload(const char* varname,s32 value)
	{ glUniform1i(get_uniform_location(varname),value); }
void ShaderPipeline::upload(const char* varname,f32 value)
	{ glUniform1f(get_uniform_location(varname),value); }
void ShaderPipeline::upload(const char* varname,vec2 value)
	{ glUniform2f(get_uniform_location(varname),value.x,value.y); }
void ShaderPipeline::upload(const char* varname,vec3 value)
	{ glUniform3f(get_uniform_location(varname),value.x,value.y,value.z); }
void ShaderPipeline::upload(const char* varname,vec4 value)
	{ glUniform4f(get_uniform_location(varname),value.x,value.y,value.z,value.w); }
void ShaderPipeline::upload(const char* varname,mat4 value)
	{ glUniformMatrix4fv(get_uniform_location(varname),1,GL_FALSE,glm::value_ptr(value)); }

/**
 *	automatically upload the global 2D coordinate system to the shader
 *	the coordinate system is uploaded to uniforms view = "view", proj = "proj"
 */
void ShaderPipeline::upload_coordinate_system()
{
	upload("view",SHADER_UNIFORM_MAT44,glm::value_ptr(g_CoordinateSystem.view));
	upload("proj",SHADER_UNIFORM_MAT44,glm::value_ptr(g_CoordinateSystem.proj));
}

/**
 *	automatically upload the global 3D camera to the shader
 *	the camera is uploaded to uniforms view = "view", proj = "proj"
 */
void ShaderPipeline::upload_camera()
{
	upload("view",SHADER_UNIFORM_MAT44,glm::value_ptr(g_Camera.view));
	upload("proj",SHADER_UNIFORM_MAT44,glm::value_ptr(g_Camera.proj));
}

/**
 *	upload the given 3D camera to the shader
 *	\param c: camera to upload
 */
void ShaderPipeline::upload_camera(Camera3D& c)
{
	upload("view",SHADER_UNIFORM_MAT44,glm::value_ptr(c.view));
	upload("proj",SHADER_UNIFORM_MAT44,glm::value_ptr(c.proj));
}

/**
 *	point to attribute in vertex buffer raster
 *	\param attrib: shader attribute structure, holding attribute name and dimension
 *	NOTE shader pipeline, vertex array & vertex buffer need to be active to point to attribute
 */
void ShaderPipeline::_define_attribute(ShaderAttribute attrib)
{
	COMM_ERR_COND(m_VertexCursor+attrib.dim*SHADER_UPLOAD_VALUE_SIZE>m_VertexShader.vbo_width,
				  "attribute dimension violates upload width");

	s32 __Attribute = _handle_attribute_location_by_name(attrib.name.c_str());
	glVertexAttribPointer(__Attribute,attrib.dim,GL_FLOAT,GL_FALSE,
						  m_VertexShader.vbo_width,(void*)m_VertexCursor);
	m_VertexCursor += attrib.dim*SHADER_UPLOAD_VALUE_SIZE;
}

/**
 *	point to attribute in index buffer raster
 *	\param attrib: shader attribute structure, holding attribute name and dimension
 *	NOTE shader pipeline, vertex array & index buffer need to be active to point to attribute
 */
void ShaderPipeline::_define_index_attribute(ShaderAttribute attrib)
{
	COMM_ERR_COND(m_IndexCursor+attrib.dim*SHADER_UPLOAD_VALUE_SIZE>m_VertexShader.ibo_width,
				  "index dimension violates upload width");

	s32 __Attribute = _handle_attribute_location_by_name(attrib.name.c_str());
	glVertexAttribPointer(__Attribute,attrib.dim,GL_FLOAT,GL_FALSE,
						  m_VertexShader.ibo_width,(void*)m_IndexCursor);
	glVertexAttribDivisor(__Attribute,1);
	m_IndexCursor += attrib.dim*SHADER_UPLOAD_VALUE_SIZE;
}

/**
 *	input attribute name and receive the attribute id
 *	\param name of the vertex/index attribute
 */
s32 ShaderPipeline::_handle_attribute_location_by_name(const char* varname)
{
	s32 attribute = glGetAttribLocation(m_ShaderProgram,varname);
	glEnableVertexAttribArray(attribute);
	return attribute;
}

/**
 *	upload all attached uniform variables
 */
void ShaderUniformUpload::upload()
{
	for (ShaderUniformValue& p_Upload : uploads)
		shader->upload(p_Upload);
}

/**
 *	cross-shader uniform variable correlation
 *	\param uniform: source uniform variable structure
 */
void ShaderUniformUpload::correlate(ShaderUniformUpload& uniform)
{
	for (ShaderUniformValue& p_Upload : uniform.uploads)
		attach_uniform(p_Upload.name.c_str(),p_Upload.udim,p_Upload.data);
}

/**
 *	attach variable in ram to auto update uniform in vram
 *	\param name: uniform name in shader
 *	\param var: pointer to variable in memory, the uniform state will be updated accordingly
 */
void ShaderUniformUpload::attach_uniform(const char* name,f32* var)
{
	ShaderUniformValue& u = _attach_variable(name);
	u.udim = SHADER_UNIFORM_FLOAT;
	u.data = var;
}

void ShaderUniformUpload::attach_uniform(const char* name,vec2* var)
{
	ShaderUniformValue& u = _attach_variable(name);
	u.udim = SHADER_UNIFORM_VEC2;
	u.data = &var->x;
}

void ShaderUniformUpload::attach_uniform(const char* name,vec3* var)
{
	ShaderUniformValue& u = _attach_variable(name);
	u.udim = SHADER_UNIFORM_VEC3;
	u.data = &var->x;
}

void ShaderUniformUpload::attach_uniform(const char* name,vec4* var)
{
	ShaderUniformValue& u = _attach_variable(name);
	u.udim = SHADER_UNIFORM_VEC4;
	u.data = &var->x;
}

void ShaderUniformUpload::attach_uniform(const char* name,mat4* var)
{
	ShaderUniformValue& u = _attach_variable(name);
	u.udim = SHADER_UNIFORM_MAT44;
	u.data = glm::value_ptr(*var);
}

void ShaderUniformUpload::attach_uniform(const char* name,UniformDimension dim,f32* var)
{
	ShaderUniformValue& u = _attach_variable(name);
	u.udim = dim;
	u.data = var;
}

/**
 *	streamlined method to correlate variable name with location and to add uniform to the bunch
 *	\param name: variable name
 */
ShaderUniformValue& ShaderUniformUpload::_attach_variable(const char* name)
{
	uploads.push_back({
			.name = name,
			.uloc = shader->get_uniform_location(name),
		});
	return uploads.back();
}
