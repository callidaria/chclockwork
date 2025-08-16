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
 *	TODO
 */
ShaderPipeline::~ShaderPipeline()
{
#ifdef VKBUILD
	vkDeviceWaitIdle(g_Vk.gpu);
	vkDestroyPipeline(g_Vk.gpu,m_Pipeline,nullptr);
	vkDestroyRenderPass(g_Vk.gpu,render_pass,nullptr);
	vkDestroyPipelineLayout(g_Vk.gpu,m_PipelineLayout,nullptr);
#endif
}

/**
 *	TODO
 */
constexpr u32 _dynamic_state_count = 2;
VkDynamicState _dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
void ShaderPipeline::assemble(const char* vs,const char* fs)
{
#ifdef VKBUILD
	// read precompiled shader binaries
	u32 __ShaderSizeVS,__ShaderSizeFS;
	char* __ShaderVS = read_file_binary(vs,__ShaderSizeVS);
	char* __ShaderFS = read_file_binary(fs,__ShaderSizeFS);

	// setup shader info
	VkShaderModule __VertexShader,__FragmentShader;
	VkShaderModuleCreateInfo __ModuleInfo = {  };
	__ModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	// vertex shader
	__ModuleInfo.codeSize = __ShaderSizeVS;
	__ModuleInfo.pCode = (u32*)__ShaderVS;
	VkResult __Result = vkCreateShaderModule(g_Vk.gpu,&__ModuleInfo,nullptr,&__VertexShader);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"vertex shader %s could not be loaded",vs);

	// fragment shader
	__ModuleInfo.codeSize = __ShaderSizeFS;
	__ModuleInfo.pCode = (u32*)__ShaderFS;
	__Result = vkCreateShaderModule(g_Vk.gpu,&__ModuleInfo,nullptr,&__FragmentShader);
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

	// fixed function vertex input state
	VkPipelineVertexInputStateCreateInfo __InputInfo = {  };
	__InputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	__InputInfo.vertexBindingDescriptionCount = 0;
	__InputInfo.pVertexBindingDescriptions = nullptr;
	__InputInfo.vertexAttributeDescriptionCount = 0;
	__InputInfo.pVertexAttributeDescriptions = nullptr;
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

	// viewport setup
	m_Viewport = {
		.x = .0f,
		.y = .0f,
		.width = (f32)g_Vk.sc_extent.width,
		.height = (f32)g_Vk.sc_extent.height,
		.minDepth = .0f,
		.maxDepth = 1.f,  // TODO is this value range or actual distance, probably the former right?
	};
	// TODO move this out of here

	// scissor setup
	m_Scissor = {
		.offset = { 0,0 },
		.extent = g_Vk.sc_extent,
	};
	// TODO this can all be pre-stored & reused for each of those pipeline setups

	// fixed function viewport
	VkPipelineViewportStateCreateInfo __ViewportInfo = {  };
	__ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	__ViewportInfo.viewportCount = 1;
	__ViewportInfo.pViewports = &m_Viewport;
	__ViewportInfo.scissorCount = 1;
	__ViewportInfo.pScissors = &m_Scissor;
	// TODO investigate why this setting even exists? what is this multiple viewport setup for?

	// fixed function rasterization
	VkPipelineRasterizationStateCreateInfo __RasterInfo = {  };
	__RasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	__RasterInfo.depthClampEnable = VK_FALSE;  // TODO utilize this instead of depth clear + border colour
	__RasterInfo.rasterizerDiscardEnable = VK_FALSE;
	__RasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	__RasterInfo.lineWidth = 1.f;
	__RasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	__RasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // TODO change this back to ccw
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

	// assemble pipeline
	VkPipelineLayoutCreateInfo __LayoutInfo = {  };
	__LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	__LayoutInfo.setLayoutCount = 0;
	__LayoutInfo.pSetLayouts = nullptr;
	__LayoutInfo.pushConstantRangeCount = 0;
	__LayoutInfo.pPushConstantRanges = nullptr;
	__Result = vkCreatePipelineLayout(g_Vk.gpu,&__LayoutInfo,nullptr,&m_PipelineLayout);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"shader layout creation from vs:%s & fs%s failed",vs,fs);

	// colour attachment
	VkAttachmentDescription __CAttachment = {  };
	__CAttachment.format = g_Vk.sc_format.format;
	__CAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	__CAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	__CAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	__CAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	__CAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	__CAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // TODO configure this in unison with clear op
	__CAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// specify fragment output location
	VkAttachmentReference __AttachmentReference = {  };
	__AttachmentReference.attachment = 0;
	__AttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// specify graphical subpass
	VkSubpassDescription __SubpassDesc = {  };
	__SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	__SubpassDesc.colorAttachmentCount = 1;
	__SubpassDesc.pColorAttachments = &__AttachmentReference;

	// subpass dependency
	VkSubpassDependency __SubpassDependency = {  };
	__SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	__SubpassDependency.dstSubpass = 0;
	__SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	__SubpassDependency.srcAccessMask = 0;
	__SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	__SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// render pass
	VkRenderPassCreateInfo __RPInfo = {  };
	__RPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	__RPInfo.attachmentCount = 1;
	__RPInfo.pAttachments = &__CAttachment;
	__RPInfo.subpassCount = 1;
	__RPInfo.pSubpasses = &__SubpassDesc;
	__RPInfo.dependencyCount = 1;
	__RPInfo.pDependencies = &__SubpassDependency;
	__Result = vkCreateRenderPass(g_Vk.gpu,&__RPInfo,nullptr,&render_pass);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create render pass");

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
	__PipelineInfo.pDepthStencilState = nullptr;
	__PipelineInfo.pColorBlendState = &__BlendingInfo;
	__PipelineInfo.pDynamicState = &__DynamicInfo;
	__PipelineInfo.layout = m_PipelineLayout;
	__PipelineInfo.renderPass = render_pass;
	__PipelineInfo.subpass = 0;
	__PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	__PipelineInfo.basePipelineIndex = -1;
	__Result = vkCreateGraphicsPipelines(g_Vk.gpu,VK_NULL_HANDLE,1,&__PipelineInfo,nullptr,&m_Pipeline);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to create graphics pipeline");
	// TODO pipeline cache

	// purge shader binaries & modules from memory after load
	vkDestroyShaderModule(g_Vk.gpu,__VertexShader,nullptr);
	vkDestroyShaderModule(g_Vk.gpu,__FragmentShader,nullptr);
	free(__ShaderVS);
	free(__ShaderFS);
	// TODO store the shader modules to quickly switch between implemented features at runtime (options menu)
	// FIXME this mallocs and frees for each shader seperately, this is not ideal!

#else
	// TODO
#endif
}
// TODO implement full vulkan compatibility for all shader features, and also finally the on-the-fly-shader

/**
 *	TODO
 *	NOTE experimental! this will be removed later when test image works
 */
void ShaderPipeline::render()
{
	// wait until current frame draw is ready
	vkWaitForFences(g_Vk.gpu,1,&g_Vk.frame_progress,VK_TRUE,UINT64_MAX);
	vkResetFences(g_Vk.gpu,1,&g_Vk.frame_progress);

	// get next swapchain image
	u32 __BufferID;
	VkResult __Result = vkAcquireNextImageKHR(g_Vk.gpu,g_Vk.swapchain,UINT64_MAX,g_Vk.image_ready,
											  VK_NULL_HANDLE,&__BufferID);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"available target frame could not be aquired");

	// reset command buffer
	vkResetCommandBuffer(g_Vk.cmd_buffer,0);

	// start command buffer
	VkCommandBufferBeginInfo __CMDInfo = {  };
	__CMDInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	__CMDInfo.flags = 0;
	__CMDInfo.pInheritanceInfo = nullptr;
	__Result = vkBeginCommandBuffer(g_Vk.cmd_buffer,&__CMDInfo);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"issue while registering a command");
	// TODO the creation info can be pre-cached instead and then just used based on registration type later

	// setup clear colour
	VkClearValue __ClearColour = {{{ .0f,.0f,.0f,1.f }}};
	// TODO foa: wtf? also: definitely set this once and run with it

	// setup begin draw
	VkRenderPassBeginInfo __RPBeginInfo = {  };
	__RPBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	__RPBeginInfo.renderPass = render_pass;
	__RPBeginInfo.framebuffer = g_Vk.framebuffers[__BufferID];
	__RPBeginInfo.renderArea.offset = { 0,0 };
	__RPBeginInfo.renderArea.extent = g_Vk.sc_extent;
	__RPBeginInfo.clearValueCount = 1;  // TODO ok but what? what on earth! do multiple clear colours do?
	__RPBeginInfo.pClearValues = &__ClearColour;
	vkCmdBeginRenderPass(g_Vk.cmd_buffer,&__RPBeginInfo,VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(g_Vk.cmd_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline);

	// viewport setup
	vkCmdSetViewport(g_Vk.cmd_buffer,0,1,&m_Viewport);
	vkCmdSetScissor(g_Vk.cmd_buffer,0,1,&m_Scissor);
	// FIXME investigate this, it seems like this could be solved with a little more elegance

	// gpu drawcall
	vkCmdDraw(g_Vk.cmd_buffer,3,1,0,0);
	// TODO it seems like this call controls the instance switch by value. this is WAY nicer than ogl, abuse this

	// finish buffer registration
	vkCmdEndRenderPass(g_Vk.cmd_buffer);
	__Result = vkEndCommandBuffer(g_Vk.cmd_buffer);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to successfully write command buffer");
	// TODO outsource appropriately to pipeline probably

	// submit buffer
	VkSemaphore __ImageSemaphores[] = { g_Vk.image_ready };
	VkSemaphore __RenderSemaphores[] = { g_Vk.render_done };
	VkPipelineStageFlags __StageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo __SubmitInfo = {  };
	__SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	__SubmitInfo.waitSemaphoreCount = 1;
	__SubmitInfo.pWaitSemaphores = __ImageSemaphores;
	__SubmitInfo.pWaitDstStageMask = __StageFlags;
	__SubmitInfo.commandBufferCount = 1;
	__SubmitInfo.pCommandBuffers = &g_Vk.cmd_buffer;
	__SubmitInfo.signalSemaphoreCount = 1;
	__SubmitInfo.pSignalSemaphores = __RenderSemaphores;
	__Result = vkQueueSubmit(g_Vk.graphical_queue,1,&__SubmitInfo,g_Vk.frame_progress);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"failed to submit command buffer");

	// swap
	VkSwapchainKHR __SwapChains[] = { g_Vk.swapchain };
	VkPresentInfoKHR __PresentInfo = {  };
	__PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	__PresentInfo.waitSemaphoreCount = 1;
	__PresentInfo.pWaitSemaphores = __RenderSemaphores;
	__PresentInfo.swapchainCount = 1;
	__PresentInfo.pSwapchains = __SwapChains;
	__PresentInfo.pImageIndices = &__BufferID;
	__PresentInfo.pResults = nullptr;
	__Result = vkQueuePresentKHR(g_Vk.presentation_queue,&__PresentInfo);
	COMM_ERR_COND(__Result!=VK_SUCCESS,"there has been an issue with frame presentation");
}

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
