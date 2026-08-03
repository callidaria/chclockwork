#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Tools

/**
 *	TODO
 */
static inline void _shader_interface_automap(const char* path,ShaderInterface& interface)
{
	// setup attribute write head for vertex components until engine annotation overwrites to instance
	vector<ShaderAttribute>* __WriteHead = &interface.vbo_attribs;
	size_t* __WidthHead = &interface.vbo_width;

	// assess input pattern for vbo/ibo automapping
	std::ifstream __File(path);
	string __Line;
	while (!__File.eof())
	{
		std::getline(__File,__Line);

		// line trim for layout prefix
#ifdef VKBUILD
		s64 __Location = -1;
		if (__Line.find("layout")==0)
		{
			size_t __LocationDef = __Line.find('=')+1;
			size_t __Until = __Line.find(')');

			// extract data input location
			__Location = stoi(__Line.substr(__LocationDef,__Until));
			COMM_ERR_COND(__Location<0,"no location extracted, this will lead to faulty data reads in shader");
			// FIXME this will also read location from uniforms, which is unnecessary

			// trim
			if (__Until!=std::string::npos) __Line = __Line.substr(__Until+2);
			// FIXME this will break when there is no whitespace between the location and in signifier
		}
#endif

		// definition processing
		if (__Line.find("// engine: ibo")==0)
		{
			__WriteHead = &interface.ibo_attribs;
			__WidthHead = &interface.ibo_width;
			continue;
		}
		else if (__Line.find("in")!=0) continue;
		else if (__Line.find("void")==0) break;

		// extract input information
		vector<string> tokens;
		split_words(tokens,__Line);

		// trim location
#ifdef GLBUILD
		tokens[2].pop_back();
#endif

		// interpret input definition line
		u8 dim = (tokens[1]=="float") ? 1 : tokens[1][3]-0x30;
		__WriteHead->push_back({
#ifdef VKBUILD
				.location = (u32)__Location,
#else
				.location = tokens[2],
#endif
				.offset = (*__WidthHead)*SHADER_UPLOAD_VALUE_SIZE,
				.dim = dim
			});
		(*__WidthHead) += dim;
	}
}


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

#ifdef VKBUILD

/**
 *	TODO
 */
UniformBuffer::UniformBuffer(u32 binding_count)
{
	m_PSizes.reserve(binding_count);
	m_Bindings.reserve(binding_count);
	m_Writes.reserve(binding_count);
	m_DescriptorInfos.reserve(binding_count);
	// TODO those can be free'd after setup has finished

	// setup default sampler
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
	// TODO research, this is an interesting feature. unfortunately only works with nearest
	__SamplerInfo.compareEnable = VK_FALSE;
	__SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	__SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	__SamplerInfo.mipLodBias = .0f;
	__SamplerInfo.minLod = 0;
	__SamplerInfo.maxLod = 0;
	VkResult __Result = vkCreateSampler(g_GPU.gpu,&__SamplerInfo,nullptr,&m_DefaultSampler);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"ubo default sampler creation failed");
}

/**
 *	TODO
 */
void UniformBuffer::define_geometry_buffer(u32 location,size_t size)
{
	COMM_MSG_COND(m_Bindings.capacity()<=m_Bindings.size(),LOG_YELLOW,
				  "uniform buffer binding malloc not sufficient, resizing (capacity>%ld)...",m_Bindings.size());

	// descriptor pool size
	VkDescriptorPoolSize __PSize = {  };
	__PSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__PSize.descriptorCount = GPU_BUFFER_COUNT*UNIFORM_DESCRIPTOR_SET_COUNT;
	m_PSizes.push_back(__PSize);

	// bindings
	VkDescriptorSetLayoutBinding __Binding = {  };
	__Binding.binding = location;
	__Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__Binding.descriptorCount = 1;  // TODO allow for multiple definitions at the same time? careful! sizing!
	__Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // TODO dynamics. what for? just to be sure i guess.
	__Binding.pImmutableSamplers = nullptr;  // TODO research
	m_Bindings.push_back(__Binding);
	// TODO debug level map if location is a duplicate to easily check development time mismatch!
	// TODO research if there is more to binding index than reference? maybe performance downsides to splits?

	// write descriptors
	VkWriteDescriptorSet __WriteDescriptor = {  };
	__WriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	__WriteDescriptor.dstBinding = location;
	__WriteDescriptor.dstArrayElement = 0;  // TODO research
	__WriteDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	__WriteDescriptor.descriptorCount = 1;
	m_Writes.push_back(__WriteDescriptor);

	DescriptorInfo __Desc = {};
	__Desc.type = DESCRIPTOR_TYPE_BUFFER;
	__Desc.info.buffer = {  };
	__Desc.info.buffer.offset = m_Size;
	__Desc.info.buffer.range = size;
	m_DescriptorInfos.push_back(__Desc);
	m_Size += size;
}

/**
 *	TODO
 */
size_t UniformBuffer::define_pixel_buffer(u32 location,VkDescriptorType type)
{
	COMM_MSG_COND(m_Bindings.capacity()<=m_Bindings.size(),LOG_YELLOW,
				  "sampler binding malloc not sufficient, resizing (capacity>%ld)...",m_Bindings.size());

	// descriptor pool size
	VkDescriptorPoolSize __PSize = {  };
	__PSize.type = type;
	__PSize.descriptorCount = GPU_BUFFER_COUNT*2;
	m_PSizes.push_back(__PSize);

	// bindings
	VkDescriptorSetLayoutBinding __Binding = {  };
	__Binding.binding = location;
	__Binding.descriptorCount = 1;
	__Binding.descriptorType = type;
	__Binding.pImmutableSamplers = nullptr;
	__Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_Bindings.push_back(__Binding);
	// TODO solve the same things as in other definition implementation (also fragment bit e.g. height manip)

	// write descriptors
	VkWriteDescriptorSet __WriteDescriptor = {  };
	__WriteDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	__WriteDescriptor.dstBinding = location;
	__WriteDescriptor.dstArrayElement = 0;
	__WriteDescriptor.descriptorType = type;
	__WriteDescriptor.descriptorCount = 1;
	m_Writes.push_back(__WriteDescriptor);

	// image info
	DescriptorInfo __Desc = {  };
	__Desc.type = DESCRIPTOR_TYPE_IMAGE;
	__Desc.info.image = {  };
	__Desc.info.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_DescriptorInfos.push_back(__Desc);
	return m_DescriptorInfos.size()-1;
}

/**
 *	TODO
 */
void UniformBuffer::link_result(size_t i,GPUPixelBuffer& texture)
{
	m_DescriptorInfos[i].info.image.imageView = texture.image_view;
	m_DescriptorInfos[i].info.image.sampler = texture.sampler;
}

/**
 *	TODO
 */
void UniformBuffer::link_result(size_t i,VkImageView buffer)
{
	m_DescriptorInfos[i].info.image.imageView = buffer;
	m_DescriptorInfos[i].info.image.sampler = m_DefaultSampler;
}

/**
 *	TODO
 */
void UniformBuffer::assemble()
{
	COMM_AWT("allocating the uniform buffer");

	// generate buffer
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		GPU::generate_buffer(m_UBO[i],m_UBOMemory[i],m_Size,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkMapMemory(g_GPU.gpu,m_UBOMemory[i],0,m_Size,0,&m_UBOMapped[i]);
	}
	// TODO stage this too? host_visible? i don't think so bröther

	// descriptor pool creation
	VkDescriptorPoolCreateInfo __DPoolInfo = {  };
	__DPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	__DPoolInfo.poolSizeCount = m_PSizes.size();
	__DPoolInfo.pPoolSizes = &m_PSizes[0];
	__DPoolInfo.maxSets = GPU_BUFFER_COUNT*UNIFORM_DESCRIPTOR_SET_COUNT;
	__DPoolInfo.flags = 0;
	VkResult __Result = vkCreateDescriptorPool(g_GPU.gpu,&__DPoolInfo,nullptr,&m_DescriptorPool);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate driver descriptor pool");

	// uniform layout
	VkDescriptorSetLayoutCreateInfo __LayoutInfo = {  };
	__LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	__LayoutInfo.bindingCount = m_Bindings.size();
	__LayoutInfo.pBindings = &m_Bindings[0];
	__Result = vkCreateDescriptorSetLayout(g_GPU.gpu,&__LayoutInfo,nullptr,&dset_layout);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"uniform layout definition failed");

	// texture layout
	__LayoutInfo = {  };
	__LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	/*
	__LayoutInfo.bindingCount = m_Bindings.size();
	__LayoutInfo.pBindings = &m_Bindings[0];
	*/
	__Result = vkCreateDescriptorSetLayout(g_GPU.gpu,&__LayoutInfo,nullptr,&dset_layout_textures);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"texture layout definition failed");

	// descriptor sets
	vector<VkDescriptorSetLayout> __DSetLayouts(GPU_BUFFER_COUNT*2,dset_layout);
	VkDescriptorSetAllocateInfo __DSetAllocInfo = {  };
	__DSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	__DSetAllocInfo.descriptorPool = m_DescriptorPool;
	__DSetAllocInfo.descriptorSetCount = GPU_BUFFER_COUNT*UNIFORM_DESCRIPTOR_SET_COUNT;
	__DSetAllocInfo.pSetLayouts = &__DSetLayouts[0];
	__Result = vkAllocateDescriptorSets(g_GPU.gpu,&__DSetAllocInfo,&m_DSets[0].data);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to allocate descriptor set memory");

	COMM_CNF();
}

/**
 *	TODO
 */
void UniformBuffer::finalize()
{
	COMM_AWT("update ubo linking");

	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		for (size_t j=0;j<m_Writes.size();j++)
		{
			m_Writes[j].dstSet = m_DSets[i].data;
			switch (m_DescriptorInfos[j].type)
			{
			case DESCRIPTOR_TYPE_BUFFER:
				m_DescriptorInfos[j].info.buffer.buffer = m_UBO[i];
				m_Writes[j].pBufferInfo = &m_DescriptorInfos[j].info.buffer;
				break;
			case DESCRIPTOR_TYPE_IMAGE: m_Writes[j].pImageInfo = &m_DescriptorInfos[j].info.image;
				break;
			}
		}
		vkUpdateDescriptorSets(g_GPU.gpu,m_Writes.size(),&m_Writes[0],0,nullptr);
	}

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

/**
 *	TODO
 */
void UniformBuffer::vanish()
{
	for (u8 i=0;i<GPU_BUFFER_COUNT;i++)
	{
		g_GPU.free(m_UBO[i]);
		g_GPU.free(m_UBOMemory[i]);
	}
	g_GPU.free(m_DescriptorPool);
	g_GPU.free(dset_layout);
	g_GPU.free(m_DefaultSampler);
}
// TODO maybe this buffer needs to be moved to shader.h instead, being closely related to it's features

#endif


// ----------------------------------------------------------------------------------------------------
// Shader Pipeline

#ifndef VKBUILD

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

#ifdef VKBUILD
const VkFormat _vertex_shader_input_formats[5] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R32_SFLOAT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_SFLOAT,
};
constexpr u32 _dynamic_state_count = 2;
VkDynamicState _dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
#endif

/**
 *	TODO
 *	TODO remove sl after moving uniform buffer definition
 */
void ShaderPipeline::assemble(const char* vs,const char* fs,bool flipped,bool pconstants)
{
#ifdef VKBUILD
	COMM_MSG_COND(m_Cursor!=depth_channel,LOG_YELLOW,
				  "render pass definition is called for finalization, but not all components were defined");

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
	ShaderInterface __Interface;
	std::filesystem::path __VertexSource(vs);
	_shader_interface_automap((__VertexSource.parent_path().parent_path()/__VertexSource.filename()).c_str(),
							  __Interface);

	// vertex binding setup
	VkVertexInputBindingDescription __InputBindings[] = { {},{} };
	__InputBindings[0].binding = 0;
	__InputBindings[0].stride = SHADER_UPLOAD_VALUE_SIZE*__Interface.vbo_width;
	__InputBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	__InputBindings[1].binding = 1;
	__InputBindings[1].stride = SHADER_UPLOAD_VALUE_SIZE*__Interface.ibo_width;
	__InputBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	// TODO find out if this has performance implications

	// vertex attribute setup
	u32 __Location = 0;
	u32 __AttributeCount = __Interface.vbo_attribs.size()+__Interface.ibo_attribs.size();
	vector<VkVertexInputAttributeDescription> __AttributeDesc(__AttributeCount);
	for (ShaderAttribute& __Attrib : __Interface.vbo_attribs)
	{
		__AttributeDesc[__Location] = {  };
		__AttributeDesc[__Location].binding = 0;
		__AttributeDesc[__Location].location = __Attrib.location;
		__AttributeDesc[__Location].format = _vertex_shader_input_formats[__Attrib.dim];
		__AttributeDesc[__Location].offset = __Attrib.offset;
		__Location++;
	}

	// instance attribute setup
	for (ShaderAttribute& __Attrib : __Interface.ibo_attribs)
	{
		__AttributeDesc[__Location] = {  };
		__AttributeDesc[__Location].binding = 1;
		__AttributeDesc[__Location].location = __Attrib.location;
		__AttributeDesc[__Location].format = _vertex_shader_input_formats[__Attrib.dim];
		__AttributeDesc[__Location].offset = __Attrib.offset;
		__Location++;
	}

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
	__RasterInfo.frontFace = (flipped)?VK_FRONT_FACE_CLOCKWISE:VK_FRONT_FACE_COUNTER_CLOCKWISE;
	__RasterInfo.depthBiasEnable = VK_FALSE;
	__RasterInfo.depthBiasConstantFactor = .0f;
	__RasterInfo.depthBiasClamp = .0f;
	__RasterInfo.depthBiasSlopeFactor = .0f;
	// TODO wait, this basically does what i do for sm in ogl dynamic sloping for depth maps?? thats crazy!

	// colour blending attachment
	VkPipelineColorBlendAttachmentState __CBlendAttachment = {  };
	__CBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT
			|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
	__CBlendAttachment.blendEnable = VK_TRUE;
	__CBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	__CBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	__CBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	__CBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	__CBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	__CBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// fixed function colour blending
	VkPipelineColorBlendStateCreateInfo __BlendingInfo = {  };
	__BlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	__BlendingInfo.logicOpEnable = VK_FALSE;
	__BlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	__BlendingInfo.attachmentCount = 1;
	__BlendingInfo.pAttachments = &__CBlendAttachment;
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
	__DepthStencilInfo.stencilTestEnable = VK_FALSE;  // TODO enable this. we need stencil trickery

	// push constants
	VkPushConstantRange* p_PushConstantRange = nullptr;
	if (pconstants)
	{
		VkPushConstantRange __PushConstantRange = {  };
		__PushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		__PushConstantRange.offset = 0;
		__PushConstantRange.size = sizeof(PushConstantMemory);
		p_PushConstantRange = &__PushConstantRange;
	}

	// assemble pipeline
	VkPipelineLayoutCreateInfo __LayoutInfo = {  };
	__LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	__LayoutInfo.setLayoutCount = 2;
	__LayoutInfo.pSetLayouts = &g_UniformBuffer.dset_layout;
	__LayoutInfo.pushConstantRangeCount = pconstants;
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
void ShaderPipeline::assemble(VertexShader vs,FragmentShader fs)
{
	m_VertexShader = vs;
	m_FragmentShader = fs;
	// FIXME this CAN and SHOULD be critisized! awful memory management through heavy copy!

	// assemble program
#ifdef VKBUILD
	// TODO

#else
	m_ShaderProgram = glCreateProgram();
	glAttachShader(m_ShaderProgram,vs.shader);
	glAttachShader(m_ShaderProgram,fs.shader);
	glLinkProgram(m_ShaderProgram);
#endif
}

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
	vkCmdBindDescriptorSets(__CMDBuffer->buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,2,
							(VkDescriptorSet*)&g_UniformBuffer.m_DSets[g_GPU.active_buffer],0,nullptr);
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
void _upload1f(u16 uloc,f32* data) { /* TODO */ }
void _upload2f(u16 uloc,f32* data) { /* TODO */ }
void _upload3f(u16 uloc,f32* data) { /* TODO */ }
void _upload4f(u16 uloc,f32* data) { /* TODO */ }
void _upload4m(u16 uloc,f32* data) { /* TODO */ }
#else
void _upload1f(u16 uloc,f32* data) { glUniform1f(uloc,data[0]); }
void _upload2f(u16 uloc,f32* data) { glUniform2f(uloc,data[0],data[1]); }
void _upload3f(u16 uloc,f32* data) { glUniform3f(uloc,data[0],data[1],data[2]); }
void _upload4f(u16 uloc,f32* data) { glUniform4f(uloc,data[0],data[1],data[2],data[3]); }
void _upload4m(u16 uloc,f32* data) { glUniformMatrix4fv(uloc,1,GL_FALSE,data); }
#endif
uniform_upload uploadf[] = { _upload1f,_upload2f,_upload3f,_upload4f,_upload4m };

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
