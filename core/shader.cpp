#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Constants

#ifdef VKBUILD

// vertex shader input format correlation
const VkFormat _vertex_shader_input_formats[SHADER_UNIFORM_FORMAT_COUNT] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R32_SFLOAT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_UNDEFINED,
};

// dynamic state
constexpr u32 _dynamic_state_count = 2;
VkDynamicState _dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };

// shader uniform name to size correlation tuples
struct ShaderType
{
	const char* name;
	size_t memsize;
};
inline const ShaderType SHADER_TYPES[SHADER_UNIFORM_FORMAT_COUNT] = {
	{ "",0 },
	{ "uint",sizeof(u32) },
	{ "int",sizeof(s32) },
	{ "float",sizeof(f32) },
	{ "vec2",sizeof(vec2) },
	{ "vec3",sizeof(vec3) },
	{ "vec4",sizeof(vec4) },
	{ "mat4",sizeof(mat4) },
	{ "sampler2D",0 }
};

#endif


// ----------------------------------------------------------------------------------------------------
// Tools

/**
 *	TODO
 */
static inline void _process_data_block_text(std::ifstream& file,size_t& varcount,size_t& blocksize)
{
	// continue reading passed file
	string __Line;
	while (!file.eof())
	{
		std::getline(file,__Line);

		// interrupt scan at end of data block, jump over possible bracket newline
		if (__Line[0]=='}') break;
		else if (__Line.find("{")==0) continue;

		// gather requested values
		std::istringstream __LineStream(__Line);
		string __Typename,__Varname;
		__LineStream >> __Typename;
		__LineStream >> __Varname;

		// correlate typename with memory size
		for (u8 i=0;i<SHADER_UNIFORM_FORMAT_COUNT;i++)
		{
			if (!strcmp(SHADER_TYPES[i].name,__Typename.c_str()))
			{
				blocksize += SHADER_TYPES[i].memsize;
				break;
			}
		}
		varcount++;
	}
}
// FIXME implementation too rigid
// FIXME this unfortunately does not work for the current implementation, that combines vertex
//		& fragment interface on the fly

/**
 *	TODO
 */
static inline void _shader_interface_automap(const char* path,ShaderInterface& interface,
											 VkShaderStageFlags stage)
{
	bool vshader = stage&VK_SHADER_STAGE_VERTEX_BIT;

	// setup attribute write head for vertex components until engine annotation overwrites to instance
	vector<ShaderAttribute>* __WriteHead = &interface.vbo_attribs;
	size_t* __WidthHead = &interface.vbo_width;

	// assess input pattern for vbo/ibo automapping
	std::ifstream __File(path);
	string __Line;
	while (!__File.eof())
	{
		std::getline(__File,__Line);
		s32 __Location = -1;
		s16 __Set = -1,__Binding = -1;
		bool __PCD = false;

		// shader head line identification for interface extraction
		// read shader engine annotation for start of ibo input variable definiton
		if (!__Line.find("// engine: ibo"))
		{
			__WriteHead = &interface.ibo_attribs;
			__WidthHead = &interface.ibo_width;
			continue;
		}

		// preprocessing for vulkan glsl 450 dialect, which requires layout prefix before variable definition
#ifdef VKBUILD
		// any layout definition, of relevance are in and uniform, because the user interfaces with them
		// out definitions shall be ignored, they are not relevant. neither the engine nor the user care at all
		else if (!__Line.find("layout"))
		{
			// in or output variable
			if (__Line[7]=='l')
			{
				size_t __LocationDef = __Line.find('=')+1;
				size_t __Until = __Line.find(')',__LocationDef);
				__Location = stoi(__Line.substr(__LocationDef,__Until));
				COMM_ERR_COND(__Location<0,
							  "no location extracted, this will lead to faulty data reads in shader: \"%s\"",
							  __Line.c_str());
				__Line = __Line.substr(__Until+2);
				// FIXME this will break when there is no whitespace between the location and in signifier
			}

			// uniform variable
			else if (__Line[7]=='s')
			{
				size_t __SetDef = __Line.find('=')+1;
				size_t __BindingDef = __Line.find('=',__SetDef)+1;
				size_t __SetUntil = __Line.find(',',__SetDef);
				size_t __BindingUntil = __Line.find(')',__BindingDef);
				__Set = stoi(__Line.substr(__SetDef,__SetUntil));
				__Binding = stoi(__Line.substr(__BindingDef,__BindingUntil));
				COMM_ERR_COND(__Set<0,
							  "no set extracted, this will lead to faulty data reads in shader: \"%s\"",
							  __Line.c_str());
				COMM_ERR_COND(__Binding<0,
							  "no binding extracted, this will lead to faulty data reads in shader: \"%s\"",
							  __Line.c_str());
				__Line = __Line.substr(__BindingUntil+2);
			}

			// push constant
			else if (__Line[7]=='p')
			{
				size_t __End = __Line.find(')');
				__Line = __Line.substr(__End+2);
				__PCD = true;
			}

			// error in case none of the upper patterns match, which should be a grave mistake in shader code
			COMM_ERR_FALLBACK("layout was found in shader, but parameters are violating expectations");
		}
#endif

		// sensibly, end head interpretation when shader function implementation starts
		else if (__Line.find("void")==0) break;
		else continue;

		// interpret the actual definition by its source (in,uniform,pcs) and its type
		// extract input information
		vector<string> tokens;
		split_words(tokens,__Line);

		// trim ';' from location, due to ogl version using a name based uloc as opposed to the int uloc in vk
#ifdef GLBUILD
		tokens[2].pop_back();
#endif

		// check for in definition
		// this automatically ignores out variable definitions
		if (tokens[0][0]=='i'&&vshader)
		{
			UniformDimension __Dim = (tokens[1]=="float")
					? SHADER_UNIFORM_FLOAT : (UniformDimension)(SHADER_UNIFORM_INT+(tokens[1][3]-0x30));
			__WriteHead->push_back({
#ifdef VKBUILD
					.location = (u32)__Location,
#else
					.location = tokens[2],
#endif
					.offset = *__WidthHead,
					.dim = __Dim
				});
			(*__WidthHead) += SHADER_TYPES[__Dim].memsize;
		}

		// check for push constant structure definition
		else if (__PCD) _process_data_block_text(__File,interface.pc_count,interface.pc_memsize);

		// check for uniform definition
		else if (tokens[0][0]=='u')
		{
			// maximum used set number implicates usage of all sets up until this number
			// resize imitates this behaviour
			if (__Set>=interface.ubo_attribs.size()) interface.ubo_attribs.resize(__Set+1);

			// determine type and check for disagreements with colliding definitions from prev. stages
			// the check is only reliable, because the read does not allow write of sampler type due to the
			// default value being 0. that value describes VK_DESCRIPTOR_TYPE_SAMPLER. the precheck would fail!
			VkDescriptorType __Type = (__Line.find("sampler2D")==string::npos)
					? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			UBOAttribute& p_Attrib = interface.ubo_attribs[__Set][__Binding];
			COMM_ERR_COND(p_Attrib.type!=0&&p_Attrib.type!=__Type,
						  "uniform variable def. disagrees across two shader stages at set: %i, binding: %i",
						  __Set,__Binding);

			// insert binding information
			p_Attrib.type = __Type;
			p_Attrib.stage |= stage;

			// read data block for size estimation & save offset
			if (__Line.find("sampler2D")==string::npos)
			{
				size_t __Count;
				_process_data_block_text(__File,__Count,p_Attrib.memsize);
				p_Attrib.offset = interface.ubo_width;
				interface.ubo_width += p_Attrib.memsize;
			}
			// FIXME dont run this when repeating read from vertex in fragment source
		}
	}
}
// TODO significant distictions between vertex and fragment shader significance for the interface like the
//		enable/disable of in variable processing & replacement probibition for double stage uniforms
//		need to be implemented in order for this to even work
// TODO this way push constants, used by both shaders are implemented twice, signify per shader!
//		this also makes it possible to exactly assign uploads either vertex or fragment shader stage bit
// FIXME this requires the push constants to be defined line by line, without empty lines in between
//		why is this not implemented using istringstream for the whole process?
// TODO read in one go from file, then store and iterate after file itself is closed


// ----------------------------------------------------------------------------------------------------
// Texture Set

/**
 *	TODO
 */
void DescriptorSetMemory::define(u32 location,UBOAttribute& attr)
{
	COMM_MSG_COND(m_DescriptorInfos.capacity()<=m_DescriptorInfos.size(),LOG_YELLOW,
				  "uniform buffer binding malloc not sufficient, resizing (capacity>%ld)...",
				  m_DescriptorInfos.size());

	// store memory index for shader location id
	m_LocationIndexCorrelation[location] = m_Writes.size();

	// image info
	DescriptorType __Type = (attr.type==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			? DESCRIPTOR_TYPE_BUFFER : DESCRIPTOR_TYPE_IMAGE;
	DescriptorInfo __Desc = {  };
	__Desc.type = __Type;

	// info initialization based on attribute type
	UBOMemoryRange __MemRange;
	switch (__Type)
	{
	case DESCRIPTOR_TYPE_BUFFER:
		__MemRange = g_UniformBuffer.memory_range_lut[m_Set][location];
		__Desc.info.buffer = {  };
		__Desc.info.buffer.offset = __MemRange.offset;
		__Desc.info.buffer.range = __MemRange.range;
		break;
	case DESCRIPTOR_TYPE_IMAGE:
		__Desc.info.image = {  };
		__Desc.info.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		__Desc.info.image.imageView = g_UniformBuffer.default_texture.image_view;
		__Desc.info.image.sampler = g_UniformBuffer.default_texture.sampler;
		break;
	};

	// store permanently for later reference by VkWriteDescriptorSet
	m_DescriptorInfos.push_back(__Desc);

	// write descriptors
	VkWriteDescriptorSet __WriteDescriptor = {  };
	__WriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	__WriteDescriptor.dstBinding = location;
	__WriteDescriptor.dstArrayElement = 0;
	__WriteDescriptor.descriptorType = attr.type;
	__WriteDescriptor.descriptorCount = 1;

	// link info to VkWriteDescriptorSet
	switch (__Type)
	{
	case DESCRIPTOR_TYPE_BUFFER: __WriteDescriptor.pBufferInfo = &m_DescriptorInfos.back().info.buffer;
		break;
	case DESCRIPTOR_TYPE_IMAGE: __WriteDescriptor.pImageInfo = &m_DescriptorInfos.back().info.image;
		break;
	};
	// FIXME not the most beautiful code

	m_Writes.push_back(__WriteDescriptor);
}

/**
 *	TODO
 */
void DescriptorSetMemory::link_result(size_t location,GPUPixelBuffer& texture)
{
	size_t i = m_LocationIndexCorrelation[location];
	m_DescriptorInfos[i].info.image = {  };
	m_DescriptorInfos[i].info.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_DescriptorInfos[i].info.image.imageView = texture.image_view;
	m_DescriptorInfos[i].info.image.sampler = texture.sampler;
}

/**
 *	TODO
 */
void DescriptorSetMemory::link_result(size_t location,VkImageView buffer)
{
	size_t i = m_LocationIndexCorrelation[location];
	m_DescriptorInfos[i].info.image = {  };
	m_DescriptorInfos[i].info.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_DescriptorInfos[i].info.image.imageView = buffer;
	m_DescriptorInfos[i].info.image.sampler = g_UniformBuffer.default_sampler;
}
// TODO not sure where this fallback sampler stuff belongs really, cannot be predefined. needs device
//		probably in renderer somewhere, alongside other possible features like placeholder textures & shapes

/**
 *	TODO
 */
void DescriptorSetMemory::allocate(u8 set,size_t size,vector<VkDescriptorSetLayout>& layouts)
{
	COMM_AWT("allocating descriptor set memory");
	m_Set = set;

	// allocate ram for write & descriptor info
	m_Writes.reserve(size);
	m_DescriptorInfos.reserve(size);

	// populate layouts for each frame in flight
	VkDescriptorSetLayout __Layouts[GPU_BUFFER_COUNT];
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++) __Layouts[i] = layouts[set];

	// allocate correlated memory for linked descriptor set layout
	VkDescriptorSetAllocateInfo __DSetAllocInfo = {  };
	__DSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	__DSetAllocInfo.descriptorPool = g_UniformBuffer.descriptor_pool;
	__DSetAllocInfo.descriptorSetCount = GPU_BUFFER_COUNT;
	__DSetAllocInfo.pSetLayouts = __Layouts;
	VkResult __Result = vkAllocateDescriptorSets(g_GPU.gpu,&__DSetAllocInfo,&m_DSets[0]);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate descriptor set memory");

	COMM_CNF();
}

/**
 *	TODO
 */
void DescriptorSetMemory::bind(VkPipelineLayout& layout)
{
	vkCmdBindDescriptorSets(g_GPU.acquire_graphical_command_buffer()->buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,layout,m_Set,1,
							(VkDescriptorSet*)&m_DSets[g_GPU.active_buffer],0,nullptr);
}

/**
 *	TODO
 *	\note it is advisable to use update(), when set is updated rarely or on condition
 */
void DescriptorSetMemory::update()
{
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		for (size_t j=0;j<m_Writes.size();j++)
		{
			if (m_DescriptorInfos[j].type==DESCRIPTOR_TYPE_BUFFER)
				m_DescriptorInfos[j].info.buffer.buffer = g_UniformBuffer.ubo[i];
			m_Writes[j].dstSet = m_DSets[i];
		}
		vkUpdateDescriptorSets(g_GPU.gpu,m_Writes.size(),&m_Writes[0],0,nullptr);
	}
}

/**
 *	TODO
 *	\note it is advisable to use update_frame(), when set is updated every frame, regardless of change
 */
void DescriptorSetMemory::update_frame()
{
	for (size_t i=0;i<m_Writes.size();i++)
	{
		if (m_DescriptorInfos[i].type==DESCRIPTOR_TYPE_BUFFER)
			m_DescriptorInfos[i].info.buffer.buffer = g_UniformBuffer.ubo[g_GPU.active_buffer];
		m_Writes[i].dstSet = m_DSets[g_GPU.active_buffer];
	}
	vkUpdateDescriptorSets(g_GPU.gpu,m_Writes.size(),&m_Writes[0],0,nullptr);
}
// TODO the writes should not be duplicated per result buffer right? they are bound, then updated?
// TODO remove the typecheck for buffer! the ubo has to be transferred ONCE, then the copy will suffice


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

#ifdef VKBUILD

/**
 *	TODO
 */
UniformBuffer::UniformBuffer()
{
	// setup default sampler
	COMM_LOG("creating default sampler");
	VkSamplerCreateInfo __SamplerInfo = {  };
	__SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	__SamplerInfo.magFilter = VK_FILTER_NEAREST;
	__SamplerInfo.minFilter = VK_FILTER_NEAREST;
	__SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	__SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	__SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	__SamplerInfo.anisotropyEnable = VK_FALSE;
	__SamplerInfo.maxAnisotropy = 0;
	__SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	__SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	__SamplerInfo.compareEnable = VK_FALSE;
	__SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	__SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	__SamplerInfo.mipLodBias = .0f;
	__SamplerInfo.minLod = 0;
	__SamplerInfo.maxLod = 0;
	VkResult __Result = vkCreateSampler(g_GPU.gpu,&__SamplerInfo,nullptr,&default_sampler);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"default sampler creation failed");
	// TODO move to renderer instead

	// generate buffer for previously defined geometry ranges
	COMM_AWT("allocating the uniform buffer");
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		GPU::generate_buffer(ubo[i],m_UBOMemory[i],
							 INTERFACE_COMBINED_GLOBAL_MEMSIZE,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkMapMemory(g_GPU.gpu,m_UBOMemory[i],0,INTERFACE_COMBINED_GLOBAL_MEMSIZE,0,&m_UBOMapped[i]);
	}
	// TODO stage this too? host_visible? i don't think so bröther

	// entry descriptor pool sizes into configuration
	// starting with uniform buffer type allocation, then combined image sampler
	VkDescriptorPoolSize __DescriptorPoolSize[2];
	__DescriptorPoolSize[0] = {  };
	__DescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__DescriptorPoolSize[0].descriptorCount = GPU_BUFFER_COUNT*SHADER_DESCRIPTOR_UNIFORM_COUNT;
	__DescriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	__DescriptorPoolSize[1].descriptorCount = GPU_BUFFER_COUNT*SHADER_DESCRIPTOR_SAMPLER_COUNT;

	// configurably generous descriptor pool allocation
	VkDescriptorPoolCreateInfo __DPoolInfo = {  };
	__DPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	__DPoolInfo.poolSizeCount = 2;
	__DPoolInfo.pPoolSizes = __DescriptorPoolSize;
	__DPoolInfo.maxSets = GPU_BUFFER_COUNT*SHADER_MAXIMUM_SET_ALLOCATION;
	__DPoolInfo.flags = 0;
	__Result = vkCreateDescriptorPool(g_GPU.gpu,&__DPoolInfo,nullptr,&descriptor_pool);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate driver descriptor pool");

	COMM_CNF();
}

/**
 *	TODO
 *	TODO add an offset to allow for bundling later (or maybe just push constants? research!)
 */
void UniformBuffer::update(void* data,size_t size)
{
	memcpy(m_UBOMapped[g_GPU.active_buffer],data,size);
}
// FIXME isn't g_GPU.active_buffer the next buffer from the currently selected one (referencing in hardware.h)
// TODO for performance reasons, maybe it would be faster to not update the whole set,
//		but instead only updated segments. then again this could also quickly become hazardous when segmentation
//		is high and many updates occur at the same time?
// TODO problem is: this will update the entire ubo memory, no matter what & where.
//		copy to specific ranges at change for independent information updates? is the memcpy for all as fast?

/**
 *	TODO
 */
void UniformBuffer::vanish()
{
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		g_GPU.free(ubo[i]);
		g_GPU.free(m_UBOMemory[i]);
	}
	g_GPU.free(descriptor_pool);
	g_GPU.free(default_sampler);
	default_texture.vanish();
}
// TODO maybe this buffer needs to be moved to shader.h instead, being closely related to it's features

/**
 *	TODO
 */
void UniformBuffer::define_data_segment(u8 set,u16 location,size_t offset,size_t range)
{
	memory_range_lut[set][location] = {
		.offset = offset,
		.range = range,
	};
}

#endif


// ----------------------------------------------------------------------------------------------------
// Shader Pipeline

#ifdef GLBUILD

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
	// compile shader
	shader = Shader::compile(path,GL_VERTEX_SHADER);

	// map interface
	if (!shader)
	{
		COMM_ERR("[SHADER] skipping input parser, vertex shader is corrupted");
		return;
	}
	_shader_interface_automap(path,interface);

	// convert widths to byte format
	interface.vbo_width *= SHADER_UPLOAD_VALUE_SIZE;
	interface.ibo_width *= SHADER_UPLOAD_VALUE_SIZE;
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

#endif


// ----------------------------------------------------------------------------------------------------
// Pipelines

/**
 *	construction & allocation for render pass description
 *	\param bfr_count: amount of colour components in result
 *	\param depth: (default false) true if depth information will be stored
 */
ShaderPipeline::ShaderPipeline(u8 bfr_count,bool depth)
	: depth_channel(bfr_count),has_depth(depth),result_attachment(bfr_count+depth)
{
	u8 __ComponentCount = bfr_count+depth;
	descriptions = (VkAttachmentDescription*)malloc(__ComponentCount*sizeof(VkAttachmentDescription));
	m_References = (VkAttachmentReference*)malloc(__ComponentCount*sizeof(VkAttachmentReference));
}

/**
 *	define a colour component
 *	\param floatbuffer: (default false) true if component stores information as floats instead of integers
 *	\returns index of defined component
 */
u8 ShaderPipeline::out_define_colour_buffer(bool floatbuffer)
{
	COMM_ERR_COND(!(m_Cursor<depth_channel),
				  "colour component definition exceeds allocated range of definable components");
	_define_colour_component(m_Cursor,(floatbuffer) ? g_Formats.floatbuffer : g_Formats.colourbuffer);
	return m_Cursor++;
	// TODO overwrite framebuffer component default resolution given by construction
}

/**
 *	define a component as result of final presentation, utilizing the destination buffers
 *	\returns index of defined component
 */
u8 ShaderPipeline::out_define_result_buffer()
{
	COMM_ERR_COND(!(m_Cursor<depth_channel),
				  "result component definition exceeds allocated range of definable components");
	_define_colour_component(m_Cursor,g_Frame.swapchain.format.format,true);
	result_attachment.set(m_Cursor);
	return m_Cursor++;
}

/**
 *	TODO
 */
inline void _compile_descriptor_uniform_attributes(ShaderInterface& interface,
												   vector<vector<VkDescriptorSetLayoutBinding>>& binds)
{
	// iterate active sets
	for (map<u32,UBOAttribute>& p_Set : interface.ubo_attribs)
	{
		vector<VkDescriptorSetLayoutBinding> __Bindings;

		// iterate bindings map
		for (auto p_Binding = p_Set.begin();p_Binding != p_Set.end();p_Binding++)
		{
			VkDescriptorSetLayoutBinding __Binding = {  };
			__Binding.binding = p_Binding->first;
			__Binding.descriptorCount = 1;
			__Binding.descriptorType = p_Binding->second.type;
			__Binding.pImmutableSamplers = nullptr;  // TODO research, only relevant for texture upload
			__Binding.stageFlags = p_Binding->second.stage;
			__Bindings.push_back(__Binding);
		}

		binds.push_back(__Bindings);
	}
}

/**
 *	TODO
 *	TODO remove sl after moving uniform buffer definition
 */
void ShaderPipeline::assemble(const char* vs,const char* fs,bool flipped)
{
#ifdef VKBUILD
	COMM_MSG_COND(m_Cursor!=depth_channel,LOG_YELLOW,
				  "render pass definition is called for finalization, but not all components were defined")
	COMM_LOG_FALLBACK("assembly of shaders vertex: %s & fragment: %s",vs,fs);

	// check for active depth store
	if (has_depth)
	{
		// depth component
		descriptions[depth_channel] = {};
		descriptions[depth_channel].format = g_Formats.depthbuffer;
		descriptions[depth_channel].samples = VK_SAMPLE_COUNT_1_BIT;
		descriptions[depth_channel].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		descriptions[depth_channel].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		descriptions[depth_channel].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		descriptions[depth_channel].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		descriptions[depth_channel].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		descriptions[depth_channel].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// define as depth stencil component
		m_References[depth_channel] = {};
		m_References[depth_channel].attachment = depth_channel;
		m_References[depth_channel].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// specify graphical subpass
	VkSubpassDescription __SubpassDesc = {  };
	__SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	__SubpassDesc.colorAttachmentCount = depth_channel;
	__SubpassDesc.pColorAttachments = m_References;
	__SubpassDesc.pDepthStencilAttachment = (has_depth) ? &m_References[depth_channel] : nullptr;

	// subpass dependency
	VkSubpassDependency __SubpassDependency = {  };
	__SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	__SubpassDependency.dstSubpass = 0;
	__SubpassDependency.srcStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	__SubpassDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	__SubpassDependency.dstStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	__SubpassDependency.dstAccessMask
			= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// TODO feature selection based on component setup

	// render pass
	VkRenderPassCreateInfo __RPInfo = {  };
	__RPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	__RPInfo.attachmentCount = depth_channel+has_depth;
	__RPInfo.pAttachments = descriptions;
	__RPInfo.subpassCount = 1;
	__RPInfo.pSubpasses = &__SubpassDesc;
	__RPInfo.dependencyCount = 1;
	__RPInfo.pDependencies = &__SubpassDependency;
	VkResult __Result = vkCreateRenderPass(g_GPU.gpu,&__RPInfo,nullptr,&render_pass);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create render pass");

	// cleanup setup component for render pass
	free(m_References);

	// read precompiled shader binaries
	u32 __ShaderSizeVS,__ShaderSizeFS;
	u8* __ShaderVS = read_file_binary(vs,__ShaderSizeVS);
	u8* __ShaderFS = read_file_binary(fs,__ShaderSizeFS);

	// setup shader info
	VkShaderModule __VertexShader,__FragmentShader;
	VkShaderModuleCreateInfo __ModuleInfo = {  };
	__ModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	// vertex shader
	__ModuleInfo.codeSize = __ShaderSizeVS;
	__ModuleInfo.pCode = (u32*)__ShaderVS;
	__Result = vkCreateShaderModule(g_GPU.gpu,&__ModuleInfo,nullptr,&__VertexShader);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"vertex shader %s could not be loaded",vs);

	// fragment shader
	__ModuleInfo.codeSize = __ShaderSizeFS;
	__ModuleInfo.pCode = (u32*)__ShaderFS;
	__Result = vkCreateShaderModule(g_GPU.gpu,&__ModuleInfo,nullptr,&__FragmentShader);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"fragment shader %s could not be loaded",fs);

	// define vertex shader stage
	VkPipelineShaderStageCreateInfo __VertexStageInfo = {  };
	__VertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	__VertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	__VertexStageInfo.module = __VertexShader;
	__VertexStageInfo.pName = "main";  // TODO holy hell this is a godsend. i love & will abuse that heavily
	__VertexStageInfo.pSpecializationInfo = nullptr;
	// TODO also pSpecializationInfo this is also great. no text combination for optional features anymore

	// define fragment shader stage
	VkPipelineShaderStageCreateInfo __FragmentStageInfo = {  };
	__FragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	__FragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	__FragmentStageInfo.module = __FragmentShader;
	__FragmentStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo __ShaderStages[] = { __VertexStageInfo,__FragmentStageInfo };
	// TODO outsource those shader specific creations to their correlating shader structs

	// shader interface automapping for input definition
	std::filesystem::path __VertexSource(vs),__FragmentSource(fs);
	_shader_interface_automap((__VertexSource.parent_path().parent_path()/__VertexSource.filename()).c_str(),
							  m_Interface,VK_SHADER_STAGE_VERTEX_BIT);
	_shader_interface_automap((__FragmentSource.parent_path().parent_path()/__FragmentSource.filename()).c_str(),
							  m_Interface,VK_SHADER_STAGE_FRAGMENT_BIT);
	// TODO split definitions into two different for each shader, to allow for some independence
	// FIXME also LIES! only vertex interface relevant for upload size will break soon

	// vertex binding setup
	VkVertexInputBindingDescription __InputBindings[] = { {},{} };
	__InputBindings[0].binding = 0;
	__InputBindings[0].stride = m_Interface.vbo_width;
	__InputBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	__InputBindings[1].binding = 1;
	__InputBindings[1].stride = m_Interface.ibo_width;
	__InputBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	// TODO find out if this has performance implications

	// vertex attribute setup
	u32 __Location = 0;
	u32 __AttributeCount = m_Interface.vbo_attribs.size()+m_Interface.ibo_attribs.size();
	vector<VkVertexInputAttributeDescription> __AttributeDesc(__AttributeCount);
	for (ShaderAttribute& __Attrib : m_Interface.vbo_attribs)
	{
		__AttributeDesc[__Location] = {  };
		__AttributeDesc[__Location].binding = 0;
		__AttributeDesc[__Location].location = __Attrib.location;
		__AttributeDesc[__Location].format = _vertex_shader_input_formats[__Attrib.dim];
		__AttributeDesc[__Location].offset = __Attrib.offset;
		__Location++;
	}

	// instance attribute setup
	for (ShaderAttribute& __Attrib : m_Interface.ibo_attribs)
	{
		__AttributeDesc[__Location] = {  };
		__AttributeDesc[__Location].binding = 1;
		__AttributeDesc[__Location].location = __Attrib.location;
		__AttributeDesc[__Location].format = _vertex_shader_input_formats[__Attrib.dim];
		__AttributeDesc[__Location].offset = __Attrib.offset;
		__Location++;
	}
	// TODO debug level map if location is a duplicate to easily check development time mismatch!

	// fixed function vertex input state
	VkPipelineVertexInputStateCreateInfo __InputInfo = {  };
	__InputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	__InputInfo.vertexBindingDescriptionCount = 2;
	__InputInfo.pVertexBindingDescriptions = __InputBindings;
	__InputInfo.vertexAttributeDescriptionCount = __AttributeCount;
	__InputInfo.pVertexAttributeDescriptions = &__AttributeDesc[0];
	// TODO implement instancing switch here later!

	// fixed function input assembly
	VkPipelineInputAssemblyStateCreateInfo __AssemblyInfo = {  };
	__AssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	__AssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	__AssemblyInfo.primitiveRestartEnable = VK_FALSE;
	// TODO how would i even dynamically select this
	// TODO this is a big discrepancy to the ogl implementation, that allows e.g. wireframe on the fly
	//		cross correlate topology interpretation by definition between vulkan and ogl
	// TODO i don't yet understand the full capabilities of primitiveRestartEnable. investigate further.

	// fixed function dynamic state
	VkPipelineDynamicStateCreateInfo __DynamicInfo = {  };
	__DynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	__DynamicInfo.dynamicStateCount = _dynamic_state_count;
	__DynamicInfo.pDynamicStates = _dynamic_states;

	// fixed function viewport
	VkPipelineViewportStateCreateInfo __ViewportInfo = {  };
	__ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	__ViewportInfo.viewportCount = 1;
	__ViewportInfo.pViewports = &g_Frame.viewport;
	__ViewportInfo.scissorCount = 1;
	__ViewportInfo.pScissors = &g_Frame.scissor;
	// TODO investigate why this setting even exists? what is this multiple viewport setup for?
	// TODO this should not always depend on standard frame viewport

	// fixed function rasterization
	VkPipelineRasterizationStateCreateInfo __RasterInfo = {  };
	__RasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	__RasterInfo.depthClampEnable = VK_FALSE;  // TODO utilize this instead of depth clear + border colour
	__RasterInfo.rasterizerDiscardEnable = VK_FALSE;
	__RasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	__RasterInfo.lineWidth = 1.f;
	__RasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	__RasterInfo.frontFace = (flipped) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	__RasterInfo.depthBiasEnable = VK_FALSE;
	__RasterInfo.depthBiasConstantFactor = .0f;
	__RasterInfo.depthBiasClamp = .0f;
	__RasterInfo.depthBiasSlopeFactor = .0f;
	// TODO wait, this basically does what i do for sm in ogl dynamic sloping for depth maps?? thats crazy!

	// colour blending attachment
	vector<VkPipelineColorBlendAttachmentState> __CBlendAttachment(depth_channel);
	for (u8 i=0;i<depth_channel;i++)
	{
		__CBlendAttachment[i] = {  };
		__CBlendAttachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT
				|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
		__CBlendAttachment[i].blendEnable = VK_TRUE;
		__CBlendAttachment[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		__CBlendAttachment[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		__CBlendAttachment[i].colorBlendOp = VK_BLEND_OP_ADD;
		__CBlendAttachment[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		__CBlendAttachment[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		__CBlendAttachment[i].alphaBlendOp = VK_BLEND_OP_ADD;
	}

	// fixed function colour blending
	VkPipelineColorBlendStateCreateInfo __BlendingInfo = {  };
	__BlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	__BlendingInfo.logicOpEnable = VK_FALSE;
	__BlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	__BlendingInfo.attachmentCount = depth_channel;
	__BlendingInfo.pAttachments = &__CBlendAttachment[0];
	__BlendingInfo.blendConstants[0] = .0f;
	__BlendingInfo.blendConstants[1] = .0f;
	__BlendingInfo.blendConstants[2] = .0f;
	__BlendingInfo.blendConstants[3] = .0f;

	// hardware based multisampling anti-aliasing
	VkPipelineMultisampleStateCreateInfo __MSAAInfo = {  };
	__MSAAInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	__MSAAInfo.sampleShadingEnable = VK_FALSE;
	__MSAAInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	__MSAAInfo.minSampleShading = 1.f;
	__MSAAInfo.pSampleMask = nullptr;
	__MSAAInfo.alphaToCoverageEnable = VK_FALSE;
	__MSAAInfo.alphaToOneEnable = VK_FALSE;

	// depth stencil
	VkPipelineDepthStencilStateCreateInfo __DepthStencilInfo = {  };
	__DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	__DepthStencilInfo.depthTestEnable = VK_TRUE;
	__DepthStencilInfo.depthWriteEnable = VK_TRUE;
	__DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	__DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	__DepthStencilInfo.stencilTestEnable = VK_FALSE;  // TODO enable this later

	// uniform variables vertex shader
	vector<vector<VkDescriptorSetLayoutBinding>> __Sets;
	_compile_descriptor_uniform_attributes(m_Interface,__Sets);

	// descriptor set layout
	VkDescriptorSetLayoutCreateInfo __DescriptorLayoutInfo = {  };
	__DescriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	m_DSetLayouts.resize(__Sets.size());
	for (size_t i=0;i<__Sets.size();i++)
	{
		vector<VkDescriptorSetLayoutBinding>& p_Set = __Sets[i];
		__DescriptorLayoutInfo.bindingCount = p_Set.size();
		__DescriptorLayoutInfo.pBindings = &p_Set[0];
		__Result = vkCreateDescriptorSetLayout(g_GPU.gpu,&__DescriptorLayoutInfo,nullptr,&m_DSetLayouts[i]);
		COMM_ERR_COND(__Result!=VK_SUCCESS,"uniform layout definition failed");
	}

	// push constants
	COMM_MSG_COND(m_Interface.pc_memsize>GPU_GUARANTEED_PCU_MEMSIZE,LOG_YELLOW,
				  "the required push constant memory size (%li bytes) violates guaranteed minimum of 128 bytes",
				  m_Interface.pc_memsize);
	VkPushConstantRange* p_PushConstantRange = nullptr;
	if (m_Interface.pc_count)
	{
		VkPushConstantRange __PushConstantRange = {  };
		__PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
		__PushConstantRange.offset = 0;
		__PushConstantRange.size = m_Interface.pc_memsize;
		p_PushConstantRange = &__PushConstantRange;
	}
	// TODO correctly establish stage flags from interface mapping
	//		also find out if this is still important if layout is only used to create pipeline, not for dsets.
	//		in the end i don't think it matters much, set generation uses this when sourced from pipeline

	// assemble pipeline
	VkPipelineLayoutCreateInfo __LayoutInfo = {  };
	__LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	__LayoutInfo.setLayoutCount = m_DSetLayouts.size();
	__LayoutInfo.pSetLayouts = &m_DSetLayouts[0];
	__LayoutInfo.pushConstantRangeCount = !!m_Interface.pc_count;
	__LayoutInfo.pPushConstantRanges = p_PushConstantRange;
	__Result = vkCreatePipelineLayout(g_GPU.gpu,&__LayoutInfo,nullptr,&pipeline_layout);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"shader layout creation from vs:%s & fs:%s failed",vs,fs);

	// combine pipeline components into final graphics pipeline
	VkGraphicsPipelineCreateInfo __PipelineInfo = {  };
	__PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	__PipelineInfo.stageCount = 2;
	__PipelineInfo.pStages = __ShaderStages;
	__PipelineInfo.pVertexInputState = &__InputInfo;
	__PipelineInfo.pInputAssemblyState = &__AssemblyInfo;
	__PipelineInfo.pViewportState = &__ViewportInfo;
	__PipelineInfo.pRasterizationState = &__RasterInfo;
	__PipelineInfo.pMultisampleState = &__MSAAInfo;
	__PipelineInfo.pDepthStencilState = &__DepthStencilInfo;
	__PipelineInfo.pColorBlendState = &__BlendingInfo;
	__PipelineInfo.pDynamicState = &__DynamicInfo;
	__PipelineInfo.layout = pipeline_layout;
	__PipelineInfo.renderPass = render_pass;
	__PipelineInfo.subpass = 0;
	__PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	__PipelineInfo.basePipelineIndex = -1;
	__Result = vkCreateGraphicsPipelines(g_GPU.gpu,VK_NULL_HANDLE,1,&__PipelineInfo,nullptr,&pipeline);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create graphics pipeline");
	// TODO pipeline cache

	// purge shader binaries & modules from memory after load
	g_GPU.free(__VertexShader);
	g_GPU.free(__FragmentShader);
	free(__ShaderVS);
	free(__ShaderFS);
	// TODO store the shader modules to quickly switch between implemented features at runtime (options menu)
	// FIXME this mallocs and frees for each shader seperately, this is not ideal!

#else
	// TODO
	// TODO make the pre-baking of the pipeline compatible. this can be done by storing the process list
	//		as function pointer sequence, that will be executed everytime (is this really good though?)

#endif
}
// TODO implement full vulkan compatibility for all shader features, and also finally the on-the-fly-shader

/**
 *	assemble shader pipeline from compiled shaders
 *	pipeline flow: vertex shader -> (geometry shader) -> fragment shader
 *	\param vs: compiled vertex shader
 *	\param fs: compiled fragment shader
 */
#ifdef GLBUILD
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
#endif

/**
 *	automatically map vertex and index buffer object to vertex shader input
 *	\param channel: starting texture channel
 *	\param vbo: vertex buffer object
 *	\param ibo: (default nullptr) index buffer object
 *	NOTE vertex buffer needs to be active
 */
/*
void ShaderPipeline::map(u16 channel,VertexBuffer* vbo,VertexBuffer* ibo)
{
#ifndef VKBUILD
	// vertex buffer
	COMM_LOG("mapping shader (vbo = %lu:%lu,ibo = %lu:%lu) utilizing %lu texture channels",
			 m_VertexShader.interface.vbo_attribs.size(),m_VertexShader.interface.vbo_width,
			 m_VertexShader.interface.ibo_attribs.size(),m_VertexShader.interface.ibo_width,
			 m_FragmentShader.sampler_attribs.size()
		);
	enable();
	for (ShaderAttribute& attrib : m_VertexShader.interface.vbo_attribs) _define_attribute(attrib);
	m_VertexCursor = 0;

	// texture mapping
	for (u16 i=0;i<m_FragmentShader.sampler_attribs.size();i++)
		upload(m_FragmentShader.sampler_attribs[i].c_str(),channel+i);

	// index buffer
	if (ibo==nullptr||!m_VertexShader.interface.ibo_attribs.size()) return;
	ibo->bind();
	for (ShaderAttribute& attrib : m_VertexShader.interface.ibo_attribs) _define_index_attribute(attrib);
	m_IndexCursor = 0;
#endif
}
*/
// TODO i don't think this is necessary in the vulkan version. remove this if possible to avoid overmapping.

void ShaderPipeline::vanish()
{
#ifdef VKBUILD
	g_GPU.expect_idle();
	g_GPU.free(pipeline);
	g_GPU.free(pipeline_layout);
	for (VkDescriptorSetLayout& p_DSetLayout : m_DSetLayouts) g_GPU.free(p_DSetLayout);
	free(descriptions);
	result_attachment.vanish();
	g_GPU.free(render_pass);
#endif
}

/**
 *	enable shader pipeline
 */
void ShaderPipeline::enable()
{
#ifdef VKBUILD
	CommandBufferGFX* __CMDBuffer = g_GPU.acquire_graphical_command_buffer();
	vkCmdBindPipeline(__CMDBuffer->buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
	/*
	vkCmdBindDescriptorSets(__CMDBuffer->buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,
							(VkDescriptorSet*)&g_UniformBuffer.m_DSets[g_GPU.active_buffer],0,nullptr);
	*/
#else
	glUseProgram(m_ShaderProgram);
#endif
}

/**
 *	disable shader pipeline
 */
void ShaderPipeline::disable()
{
#ifdef VKBUILD
	// TODO
#else
	glUseProgram(0);
#endif
}

/**
 *	TODO
 */
void ShaderPipeline::generate_ubo(vector<DescriptorSetMemory>& sets)
{
	sets.resize(m_Interface.ubo_attribs.size());
	for (u8 i=0;i<m_Interface.ubo_attribs.size();i++)
	{
		map<u32,UBOAttribute>& p_Set = m_Interface.ubo_attribs[i];
		DescriptorSetMemory& p_DSetMemory = sets[i];
		p_DSetMemory.allocate(i,p_Set.size(),m_DSetLayouts);
		// FIXME the set layout + size at call does not make sense in the slightest
		for (auto p_Binding = p_Set.begin();p_Binding != p_Set.end();p_Binding++)
			p_DSetMemory.define(p_Binding->first,p_Binding->second);
	}
}
// TODO generate set specifical. this should then allow a global set at slot 0 for basic data & buffers that
//		change exactly once each frame, then switch at will for sets 1-3 (<4 is guaranteed)
// TODO only allocate new descriptor set memory, when the pattern is not already setup.
//		this will prevent e.g. the allocation for one-time update global states like
//		3D camera and 2D coordinate system.

/**
 *	TODO
 */
void ShaderPipeline::generate_pcm(void* pcm,u32 repeat)
{
	if (!m_Interface.pc_count) return;
	pcm = malloc(m_Interface.pc_memsize*repeat);
}

/**
 *	TODO
 */
void ShaderPipeline::upload_pcm(void* pcm,u32 ofs)
{
	if (!m_Interface.pc_count) return;
	vkCmdPushConstants(g_GPU.acquire_graphical_command_buffer()->buffer,pipeline_layout,
					   VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,ofs*m_Interface.pc_memsize,
					   m_Interface.pc_memsize,pcm);
}
// FIXME i'd rather not check every time
// TODO research if it is supported by transfer queue

/**
 *	extract uniform location from shader program
 *	\param uname: literal uniform variable name in shader program
 *	\returns uniform location
 */
u32 ShaderPipeline::get_uniform_location(const char* uname)
{
#ifdef VKBUILD
	// TODO
	return 0;

#else
	return glGetUniformLocation(m_ShaderProgram,uname);
#endif
}

// uniform variable upload function correlation map
typedef void (*uniform_upload)(u16,f32*);
#ifdef VKBUILD
void _uploadu(u16 uloc,f32* data) { /* TODO */ }
void _uploadi(u16 uloc,f32* data) { /* TODO */ }
void _upload1f(u16 uloc,f32* data) { /* TODO */ }
void _upload2f(u16 uloc,f32* data) { /* TODO */ }
void _upload3f(u16 uloc,f32* data) { /* TODO */ }
void _upload4f(u16 uloc,f32* data) { /* TODO */ }
void _upload4m(u16 uloc,f32* data) { /* TODO */ }
#else
void _uploadi(u16 uloc,f32* data) { glUniform1u(uloc,data[0]); }
void _uploadui(u16 uloc,f32* data) { glUniform1i(uloc,data[0],data[1]); }
void _upload1f(u16 uloc,f32* data) { glUniform1f(uloc,data[0]); }
void _upload2f(u16 uloc,f32* data) { glUniform2f(uloc,data[0],data[1]); }
void _upload3f(u16 uloc,f32* data) { glUniform3f(uloc,data[0],data[1],data[2]); }
void _upload4f(u16 uloc,f32* data) { glUniform4f(uloc,data[0],data[1],data[2],data[3]); }
void _upload4m(u16 uloc,f32* data) { glUniformMatrix4fv(uloc,1,GL_FALSE,data); }
#endif
uniform_upload uploadf[SHADER_UNIFORM_FORMAT_COUNT]
		= { _uploadu,_uploadi,_upload1f,_upload2f,_upload3f,_upload4f,_upload4m };

/**
 *	upload signed integer to shader
 *	\param varname: variable name as defined as "uniform" in shader (must be part of the pipeline)
 *	\param value: signed integer value to upload to variable
 *	NOTE shader pipeline needs to be active to upload values to uniform variables
 */
void ShaderPipeline::upload(const char* varname,s32 value)
{
#ifdef VKBUILD
	// TODO

#else
	glUniform1i(get_uniform_location(varname),value);
#endif
}

/**
 *	upload uniform variable to shader
 *	\param varname: variable name as defined as "uniform" in shader (must be part of the pipeline)
 *	\param value: value to upload to specified variable
 *	NOTE shader pipeline needs to be active to upload values to uniform variables
 */
void ShaderPipeline::upload(const char* varname,f32 value) { upload(varname,SHADER_UNIFORM_FLOAT,&value); }
void ShaderPipeline::upload(const char* varname,vec2 value) { upload(varname,SHADER_UNIFORM_VEC2,&value.x); }
void ShaderPipeline::upload(const char* varname,vec3 value) { upload(varname,SHADER_UNIFORM_VEC3,&value.x); }
void ShaderPipeline::upload(const char* varname,vec4 value) { upload(varname,SHADER_UNIFORM_VEC4,&value.x); }
void ShaderPipeline::upload(const char* varname,mat4 value)
	{ upload(varname,SHADER_UNIFORM_MAT44,glm::value_ptr(value)); }

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


#ifdef VKBUILD

/**
 *	helper to define different sorts of colour components by format and index
 *	\param index: colour component index
 *	\param format: requested colour component format, depending on usage
 *	\param result: (default false) true if component is result buffer
 */
void ShaderPipeline::_define_colour_component(u8 index,VkFormat format,bool result)
{
	// specify colour component
	descriptions[index] = {};
	descriptions[index].format = format;
	descriptions[index].samples = VK_SAMPLE_COUNT_1_BIT;
	descriptions[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	descriptions[index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	descriptions[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	descriptions[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	descriptions[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	descriptions[index].finalLayout
			= (result) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// specify fragment output location
	m_References[index] = {};
	m_References[index].attachment = index;
	m_References[index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

#else

/**
 *	point to attribute in vertex buffer raster
 *	\param attrib: shader attribute structure, holding attribute name and dimension
 *	NOTE shader pipeline, vertex array & vertex buffer need to be active to point to attribute
 */
void ShaderPipeline::_define_attribute(ShaderAttribute attrib)
{
	COMM_ERR_COND(m_VertexCursor+attrib.dim*SHADER_UPLOAD_VALUE_SIZE>m_VertexShader.interface.vbo_width,
				  "attribute dimension violates upload width");

	s32 __Attribute = _handle_attribute_location_by_name(attrib.location.c_str());
	glVertexAttribPointer(__Attribute,attrib.dim,GL_FLOAT,GL_FALSE,
						  m_VertexShader.interface.vbo_width,(void*)m_VertexCursor);
	m_VertexCursor += attrib.dim*SHADER_UPLOAD_VALUE_SIZE;
}

/**
 *	point to attribute in index buffer raster
 *	\param attrib: shader attribute structure, holding attribute name and dimension
 *	NOTE shader pipeline, vertex array & index buffer need to be active to point to attribute
 */
void ShaderPipeline::_define_index_attribute(ShaderAttribute attrib)
{
	COMM_ERR_COND(m_IndexCursor+attrib.dim*SHADER_UPLOAD_VALUE_SIZE>m_VertexShader.interface.ibo_width,
				  "index dimension violates upload width");

	s32 __Attribute = _handle_attribute_location_by_name(attrib.location.c_str());
	glVertexAttribPointer(__Attribute,attrib.dim,GL_FLOAT,GL_FALSE,
						  m_VertexShader.interface.ibo_width,(void*)m_IndexCursor);
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

#endif


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

/**
 *	attach variable in ram to auto update uniform in vram, forcing the variable dimension by caller
 *	\param name: uniform name in shader
 *	\param dim: forced variable dimension
 *	\param var: pointer to variable in memory, the uniform state will be updated accordingly
 */
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
