#include "renderer.h"


// ----------------------------------------------------------------------------------------------------
// Background Process Signals

// texture collector signals
ThreadSignal _sprite_texture_signal
#ifdef DEBUG
= { .name = "sprite texture" }
#endif
;

// sprite collector signals
ThreadSignal _sprite_signal
#ifdef DEBUG
= { .name = "sprite" }
#endif
;


// ----------------------------------------------------------------------------------------------------
// Text Component

/**
 *	dynamically align text content based on content dimensions
 */
void Text::align()
{
	// calculate text dimensions
	f32 wordlen = font->estimate_wordlength(data);
	dimensions = vec2(wordlen,font->size)*scale;

	// calculate position based on alignment and dimensions
	if (alignment.align<SCREEN_ALIGN_NEUTRAL)
	{
		vec2 __AlignedOffset = Renderer::align({ position,dimensions },alignment);
		offset.x = __AlignedOffset.x;
		offset.y = __AlignedOffset.y;
		return;
	}

	// compliment dimensions by offset
	offset.x = position.x-dimensions.x*.5f;
	offset.y = position.y-dimensions.y*.33f;
}

/**
 *	load instance buffer for text content according to specified font
 */
void Text::load_buffer()
{
	bool reallocate = buffer.capacity()<data.size();
	COMM_LOG_COND(reallocate,"allocating memory for text buffer");
	buffer.resize(data.size());

	// load font information for characters
	vec3 __Cursor = vec3(offset.x,offset.y,position.z);
	for (u32 i=0;i<data.size();i++)
	{
		TextCharacter& p_Character = buffer[i];
		PixelBufferComponent& p_Component = font->tex[data[i]-32];
		Glyph& p_Glyph = font->glyphs[data[i]-32];

		// load text data
		p_Character = {
			.offset = __Cursor,
			.scale = p_Glyph.scale*scale,
			.bearing = p_Glyph.bearing*scale,
			.colour = colour,
			.comp = p_Component
		};

		__Cursor.x += p_Glyph.advance*scale;
	}
}

/**
 *	calculate horizontal character intersection
 *	\param pos: horizontal intersecting pixel position
 *	\returns 0 if end of the word, buffer size if beginning of the word is last intersection and else in-between
 */
u32 Text::intersection(f32 pos)
{
	u32 i = 0;

	// starting intersection
	if (!data.size()) return 0;
	f32 __Cursor = position.x+font->glyphs[data[0]-32].advance*.5*scale;

	// iterate following characters
	while (i<data.size()-1&&__Cursor<pos)
	{
		__Cursor += (font->glyphs[data[i]-32].advance+font->glyphs[data[i+1]-32].advance)*.5*scale;
		i++;
	}
	return data.size()-i;
}


// ----------------------------------------------------------------------------------------------------
// Geometry Loading

/**
 *	load mesh geometry from .obj file
 *	\param path: path to .obj file explicitly defining geometry
 */
Mesh::Mesh(const char* path)
{
	vector<vec3> __Positions;
	vector<vec2> __UVCoordinates;
	vector<vec3> __Normals;
	vector<u32> __PositionIndices;
	vector<u32> __UVIndices;
	vector<u32> __NormalIndices;

	// open source file
	FILE* __OBJFile = fopen(path,"r");
	if (__OBJFile==NULL)
	{
		COMM_ERR("geometry definition file %s could not be found",path);
		return;
	}

	// iterate and sort geometry information
	char __Command[128];
	while (fscanf(__OBJFile,"%s",__Command)!=EOF)
	{
		// process position prefix
		if (!strcmp(__Command,"v"))
		{
			vec3 __Position;
			fscanf(__OBJFile,"%f %f %f\n",&__Position.x,&__Position.y,&__Position.z);
			__Positions.push_back(__Position);
		}

		// process uv coordinate prefix
		else if (!strcmp(__Command,"vt"))
		{
			vec2 __UVCoordinate;
			fscanf(__OBJFile,"%f %f\n",&__UVCoordinate.x,&__UVCoordinate.y);
			__UVCoordinates.push_back(__UVCoordinate);
		}

		// process normal prefix
		else if (!strcmp(__Command,"vn"))
		{
			vec3 __Normal;
			fscanf(__OBJFile,"%f %f %f\n",&__Normal.x,&__Normal.y,&__Normal.z);
			__Normals.push_back(__Normal);
		}

		// process face prefix
		else if (!strcmp(__Command,"f"))
		{
			u32 __PositionIndex[3];
			u32 __UVIndex[3];
			u32 __NormalIndex[3];
			fscanf(
					__OBJFile,"%u/%u/%u %u/%u/%u %u/%u/%u\n",
					&__PositionIndex[0],&__UVIndex[0],&__NormalIndex[0],
					&__PositionIndex[1],&__UVIndex[1],&__NormalIndex[1],
					&__PositionIndex[2],&__UVIndex[2],&__NormalIndex[2]
				);
			for (u8 i=0;i<3;i++)
			{
				__PositionIndices.push_back(__PositionIndex[i]);
				__UVIndices.push_back(__UVIndex[i]);
				__NormalIndices.push_back(__NormalIndex[i]);
			}
		}
	}

	// close file & allocate memory
	fclose(__OBJFile);
	vertices.reserve(__PositionIndices.size());

	// iterate faces & write vertices
	for (u32 i=0;i<__PositionIndices.size();i+=3)
	{
		for (u8 j=0;j<3;j++)
		{
			u32 n = i+j;
			Vertex __Vertex = {
				.position = __Positions[__PositionIndices[n]-1],
				.uv = __UVCoordinates[__UVIndices[n]-1],
				.normal = __Normals[__NormalIndices[n]-1]
			};
			vertices.push_back(__Vertex);
		}

		// precalculate tangent for gram-schmidt reorthogonalization & normal mapping
		vec3 __EdgeDelta0 = vertices[i+1].position-vertices[i].position;
		vec3 __EdgeDelta1 = vertices[i+2].position-vertices[i].position;
		vec2 __UVDelta0 = vertices[i+1].uv-vertices[i].uv;
		vec2 __UVDelta1 = vertices[i+2].uv-vertices[i].uv;
		f32 __Factor = 1.f/(__UVDelta0.x*__UVDelta1.y-__UVDelta0.y*__UVDelta1.x);
		glm::mat2x3 __CombinedEdges = glm::mat2x3(__EdgeDelta0,__EdgeDelta1);
		vec2 __CombinedUVs = vec2(__UVDelta1.y,-__UVDelta0.y);
		vec3 __Tangent = __Factor*(__CombinedEdges*__CombinedUVs);
		__Tangent = glm::normalize(__Tangent);
		for (u8 j=0;j<3;j++) vertices[i+j].tangent = __Tangent;
	}
}

/**
 *	load animation & mesh information from collada file
 *	\param path: path to .dae collada file
 */
u16 _rc_get_joint_count(aiNode* root)
{
	u16 __Result = 1;
	for (u16 i=0;i<root->mNumChildren;i++) __Result += _rc_get_joint_count(root->mChildren[i]);
	return __Result;
}

void _rc_assemble_joint_hierarchy(vector<MeshJoint>& joints,aiNode* root)
{
	// root joint translation
	u16 __MemoryID = joints.size();
	MeshJoint __Joint = {
		.id = root->mName.C_Str(),
		.uniform_location = "joint_transform["+std::to_string(__MemoryID)+"]",
		.transform = to_mat4(root->mTransformation),
		.children = vector<u16>(root->mNumChildren)
	};
	joints.push_back(__Joint);

	// recursively process children
	for (u16 i=0;i<root->mNumChildren;i++)
	{
		joints[__MemoryID].children[i] = joints.size();
		_rc_assemble_joint_hierarchy(joints,root->mChildren[i]);
	}
}

u16 _get_joint_id(vector<MeshJoint>& joints,string id)
{
	u16 i = 0;
	while (id!=joints[i].id) i++;
	return i;
}

AnimatedMesh::AnimatedMesh(const char* path)
{
	Assimp::Importer __Importer;
	const aiScene* __File = __Importer.ReadFile(
			path,
			aiProcess_CalcTangentSpace|aiProcess_Triangulate|aiProcess_JoinIdenticalVertices
		);

	// extract joints
	u16 __JointCount = _rc_get_joint_count(__File->mRootNode);
	joints.reserve(__JointCount);
	_rc_assemble_joint_hierarchy(joints,__File->mRootNode);

	// load mesh
	// extract bone armature offset
	aiMesh* __Mesh = __File->mMeshes[0];
	u16 __RootOffset = _get_joint_id(joints,__Mesh->mBones[0]->mName.C_Str());
	// TODO allow the loader to pull all existing meshes in the scene, not just the first one

	// extract bone influence weights
	// memory allocation
	u8 __WriteCount[__Mesh->mNumVertices] = { 0 };
	f32 __BoneIndices[__Mesh->mNumVertices][RENDERER_ANIMATION_INFLUENCE_RANGE] = { 0 };
	f32 __Weights[__Mesh->mNumVertices][RENDERER_ANIMATION_INFLUENCE_RANGE] = { 0 };
	// TODO maybe handle these in their own datastructure

	// iterate extraction
	for (u16 i=0;i<__Mesh->mNumBones;i++)
	{
		aiBone* __Bone = __Mesh->mBones[i];
		u16 __JointIndex = __RootOffset+i;
		joints[__JointIndex].offset = to_mat4(__Bone->mOffsetMatrix);
		// FIXME questionable placement for offset extraction

		// map bone weights onto vertices
		for (u32 j=0;j<__Bone->mNumWeights;j++)
		{
			aiVertexWeight& __Weight = __Bone->mWeights[j];

			// store indices & weights until overflow
			if (__WriteCount[__Weight.mVertexId]<RENDERER_ANIMATION_INFLUENCE_RANGE)
			{
				u8 k = __WriteCount[__Weight.mVertexId]++;
				__BoneIndices[__Weight.mVertexId][k] = __JointIndex;
				__Weights[__Weight.mVertexId][k] = __Weight.mWeight;
			}

			// priority store in case of weight overflow
			else
			{
				u8 __ProcIndex = 0;
				f32 __ProcWeight = __Weights[__Weight.mVertexId][0];  // TODO remove

				// iterate to fine least influential weight
				for (u8 k=1;k<RENDERER_ANIMATION_INFLUENCE_RANGE;k++)
				{
					if (__ProcWeight>__Weights[__Weight.mVertexId][k])
					{
						__ProcIndex = k;
						__ProcWeight = __Weights[__Weight.mVertexId][k];
					}
				}

				// overwrite most insignificant weight if current weight is important enough
				if (__Weight.mWeight>__Weights[__Weight.mVertexId][__ProcIndex])
					__Weights[__Weight.mVertexId][__ProcIndex] = __Weight.mWeight;
			}
		}
	}

	// compose vertex data
	// assemble vertex array
	vertices.reserve(__Mesh->mNumVertices);
	for (u64 i=0;i<__Mesh->mNumVertices;i++)
	{
		vertices.push_back({
				.position = to_vec3(__Mesh->mVertices[i]),
				.uv = to_vec2(__Mesh->mTextureCoords[0][i]),
				.normal = to_vec3(__Mesh->mNormals[i]),
				.tangent = to_vec3(__Mesh->mTangents[i]),
				.bone_index = vec4(__BoneIndices[i][0],__BoneIndices[i][1],
								   __BoneIndices[i][2],__BoneIndices[i][3]),
				.bone_weight = vec4(__Weights[i][0],__Weights[i][1],__Weights[i][2],__Weights[i][3])
			});
		// TODO be careful with the static 0 when expanding the loader
	}

	// allocate memory for element array
	for (u32 i=0;i<__Mesh->mNumFaces;i++) index_count += __Mesh->mFaces[i].mNumIndices;
	elements.reserve(index_count);

	// assemble element array
	for (u32 i=0;i<__Mesh->mNumFaces;i++)
	{
		for (u32 j=0;j<__Mesh->mFaces[i].mNumIndices;j++)
			elements.push_back(__Mesh->mFaces[i].mIndices[j]);
	}

	// extract animations
	// allocate memory & iterate animations
	animations.reserve(__File->mNumAnimations);
	for (u32 i=0;i<__File->mNumAnimations;i++)
	{
		aiAnimation* __Animation = __File->mAnimations[i];
		f64 __TPSinv = 1./__Animation->mTicksPerSecond;

		// register new animation
		animations.push_back({
				.joints = vector<AnimationJoint>(__Animation->mNumChannels),
				.duration = __Animation->mDuration*__TPSinv
			});

		// process animation channels
		for (u32 j=0;j<__Animation->mNumChannels;j++)
		{
			aiNodeAnim* __Node = __Animation->mChannels[j];
			AnimationJoint& __Joint = animations.back().joints[j];

			// process channel keys for related joint
			__Joint = {
				.id = _get_joint_id(joints,__Node->mNodeName.C_Str()),
				.position_keys = vector<vec3>(__Node->mNumPositionKeys),
				.scaling_keys = vector<vec3>(__Node->mNumScalingKeys),
				.rotation_keys = vector<quat>(__Node->mNumRotationKeys),
				.position_durations = vector<f64>(__Node->mNumPositionKeys),
				.scaling_durations = vector<f64>(__Node->mNumScalingKeys),
				.rotation_durations = vector<f64>(__Node->mNumRotationKeys)
			};
			// FIXME what happens to memory during this loop is truly gruesome

			// extract position keys
			for (u32 k=0;k<__Node->mNumPositionKeys;k++)
			{
				__Joint.position_keys[k] = to_vec3(__Node->mPositionKeys[k].mValue);
				__Joint.position_durations[k] = __Node->mPositionKeys[k].mTime*__TPSinv;
			}

			// extract scaling keys
			for (u32 k=0;k<__Node->mNumScalingKeys;k++)
			{
				__Joint.scaling_keys[k] = to_vec3(__Node->mScalingKeys[k].mValue);
				__Joint.scaling_durations[k] = __Node->mScalingKeys[k].mTime*__TPSinv;
			}

			// extract rotation keys
			for (u32 k=0;k<__Node->mNumRotationKeys;k++)
			{
				__Joint.rotation_keys[k] = to_quat(__Node->mRotationKeys[k].mValue);
				__Joint.rotation_durations[k] = __Node->mRotationKeys[k].mTime*__TPSinv;
			}
		}
	}
}
// FIXME do a resize and then overwrite instead of push back after reserve?

/**
 *	update active animation
 */
f32 _advance_keys(vector<f64>& durations,u16& crr,f64 progress)
{
	while (durations[crr+1]<progress) crr++;
	crr *= crr<durations.size()&&durations[crr]<progress;
	return (progress-durations[crr])/(durations[crr+1]-durations[crr]);
}

void AnimatedMesh::update()
{
	Animation& p_Animation = animations[current_animation];

	// interpolation delta
	progress += g_Frame.delta_time;
	progress = fmod(progress,p_Animation.duration);

	// iterate joints for location animation transformations
	for (AnimationJoint& p_Joint : p_Animation.joints)
	{
		// determine transformation keyframes
		f32 __TransformProgress = _advance_keys(p_Joint.position_durations,p_Joint.crr_position,progress);
		f32 __ScalingProgress = _advance_keys(p_Joint.scaling_durations,p_Joint.crr_scale,progress);
		f32 __RotationProgress = _advance_keys(p_Joint.rotation_durations,p_Joint.crr_rotation,progress);

		// interpolation between keyframes
		// translations
		vec3 __TranslateInterpolation = glm::mix(
				p_Joint.position_keys[p_Joint.crr_position],
				p_Joint.position_keys[p_Joint.crr_position+1],
				__TransformProgress
			);

		// scaling
		vec3 __ScaleInterpolation = glm::mix(
				p_Joint.scaling_keys[p_Joint.crr_scale],
				p_Joint.scaling_keys[p_Joint.crr_scale+1],
				__ScalingProgress
			);

		// rotation
		quat __RotateInterpolation = glm::slerp(
				p_Joint.rotation_keys[p_Joint.crr_rotation],
				p_Joint.rotation_keys[p_Joint.crr_rotation+1],
				__RotationProgress
			);

		// transformation
		joints[p_Joint.id].transform = glm::translate(mat4(1.f),__TranslateInterpolation)
				* glm::scale(mat4(1.f),__ScaleInterpolation)
				* glm::toMat4(__RotateInterpolation);
	}

	// calculate transform after parent influence
	mat4 __Parent = mat4(1.f);
	_rc_transform_interpolation(joints[0],__Parent);
	// FIXME it's unclear if joints[0] is always root node, this could lead to nasty consequences
}

void AnimatedMesh::_rc_transform_interpolation(MeshJoint& joint,mat4& parent_transform)
{
	mat4 __LocalTransform = parent_transform*joint.transform;
	joint.recursive_transform = __LocalTransform*joint.offset;
	for (u16 child : joint.children) _rc_transform_interpolation(joints[child],__LocalTransform);
}


// ----------------------------------------------------------------------------------------------------
// Geometry Batching

/**
 *	add mesh geometry to batch
 *	\param mesh: loaded mesh for explicit geometry information
 *	\param tex: multichannel texture data to upload
 *	\returns geometry id
 */
u32 GeometryBatch::add_geometry(Mesh& mesh,vector<Texture*>& tex)
{
	return add_geometry(&mesh.vertices[0],mesh.vertices.size(),sizeof(Vertex),tex);
}

/**
 *	add animated mesh geometry to batch
 *	\param mesh: animated mesh for explicit geometry information
 *	\param tex: multichannel texture data to upload
 *	\returns geometry id
 */
u32 GeometryBatch::add_geometry(AnimatedMesh& mesh,vector<Texture*>& tex)
{
	u32 id = add_geometry(&mesh.vertices[0],mesh.vertices.size(),sizeof(AnimationVertex),mesh.elements,tex);
	for (MeshJoint& p_Joint : mesh.joints)
		attach_uniform(id,p_Joint.uniform_location.c_str(),&p_Joint.recursive_transform);
	return id;
}

/**
 *	load geometry into batch
 *	\param verts: single precision floats, explicitly defining geometry
 *	\param vsize: amount of vertices (this is the pointer length divided by the upload dimension)
 *	\param ssize: upload dimension !in memory width!
 *	\param tex: multichannel texture data to upload
 *	\returns geometry id
 */
u32 GeometryBatch::add_geometry(void* verts,size_t vsize,size_t ssize,vector<Texture*>& tex)
{
	COMM_LOG("uploading geometry to batch");
	size_t __MemSize = vsize*ssize;
	size_t __Size = __MemSize/sizeof(f32);
	geometry.resize(geometry_cursor+__Size);
	memcpy(&geometry[geometry_cursor],verts,__MemSize);

	// store geometry information
	object.push_back({
			.offset = offset_cursor,
			.vertex_count = vsize,
			.textures = tex
		});
	offset_cursor += vsize;
	geometry_cursor += __Size;
	return object.size()-1;
}

/**
 *	load geometry into batch
 *	\param verts: single precision floats, explicitly defining geometry
 *	\param vsize: amount of vertices (this is the pointer length divided by the upload dimension)
 *	\param ssize: upload dimension !in memory width!
 *	\param elems: element array vector
 *	\param tex: multichannel texture data to upload
 *	\returns geometry id
 */
u32 GeometryBatch::add_geometry(void* verts,size_t vsize,size_t ssize,vector<u32>& elems,vector<Texture*>& tex)
{
	COMM_LOG("uploading geometry to batch");
	size_t __MemSize = vsize*ssize;
	size_t __Size = __MemSize/sizeof(f32);
	geometry.resize(geometry_cursor+__Size);
	memcpy(&geometry[geometry_cursor],verts,__MemSize);

	// copy element array vector
	elements.resize(element_cursor+elems.size());
	for (size_t i=0;i<elems.size();i++) elements[i] = elems[element_cursor+i];

	// store geometry information
	object.push_back({
			.offset = offset_cursor,
			.vertex_count = elems.size(),
			.textures = tex
		});
	offset_cursor += elems.size();
	geometry_cursor += __Size;
	element_cursor += elems.size();
	return object.size()-1;
}
// FIXME a lot of codecopy is happening here

/**
 *	upload batch geometry to gpu & automap shader pipeline
 */
void GeometryBatch::load()
{
	COMM_LOG("uploading geometry information to GPU");
	vao.bind();
	vbo.bind();
	vbo.upload_vertices(geometry);
	ebo.bind_elements();
	ebo.upload_elements(elements);
	shader->map(RENDERER_TEXTURE_UNMAPPED,&vbo);
}
// FIXME elements stay bound... segfault if this is not the case? doesn't really make sense

/**
 *	attach variable in ram to auto update uniform in vram
 *	\param gid: geometry id, returned when geometry was registered in batch
 *	\param name: uniform name in shader
 *	\param var: pointer to variable in memory, that will automatically be uploaded to gpu
 */
void GeometryBatch::attach_uniform(u32 gid,const char* name,f32* var)
{
	object[gid].uploads.push_back({
			.uloc = shader->get_uniform_location(name),
			.udim = SHADER_UNIFORM_FLOAT,
			.data = var
		});
}

void GeometryBatch::attach_uniform(u32 gid,const char* name,vec2* var)
{
	object[gid].uploads.push_back({
			.uloc = shader->get_uniform_location(name),
			.udim = SHADER_UNIFORM_VEC2,
			.data = &var->x
		});
}

void GeometryBatch::attach_uniform(u32 gid,const char* name,vec3* var)
{
	object[gid].uploads.push_back({
			.uloc = shader->get_uniform_location(name),
			.udim = SHADER_UNIFORM_VEC3,
			.data = &var->x
		});
}

void GeometryBatch::attach_uniform(u32 gid,const char* name,vec4* var)
{
	object[gid].uploads.push_back({
			.uloc = shader->get_uniform_location(name),
			.udim = SHADER_UNIFORM_VEC4,
			.data = &var->x
		});
}

void GeometryBatch::attach_uniform(u32 gid,const char* name,mat4* var)
{
	object[gid].uploads.push_back({
			.uloc = shader->get_uniform_location(name),
			.udim = SHADER_UNIFORM_MAT44,
			.data = glm::value_ptr(*var)
		});
}

/**
 *	setup particle batch by mesh geometry
 *	\param mesh: loaded mesh for explicit geometry information
 *	\param particles: amount of particles
 */
void ParticleBatch::load(Mesh& mesh,u32 particles)
{
	load(&mesh.vertices[0],mesh.vertices.size(),sizeof(Vertex),particles);
}

/**
 *	load particle mesh into batch memory
 *	\param verts: single precision floats, explicitly defining geometry
 *	\param vsize: amount of vertices (this is the pointer length divided by the upload dimension)
 *	\param ssize: upload dimension !in memory width!
 *	\param particles: amount of particles
 */
void ParticleBatch::load(void* verts,size_t vsize,size_t ssize,u32 particles)
{
	COMM_LOG("loading particle mesh geometry information");
	size_t size = vsize*ssize;
	geometry.resize(size/sizeof(f32));
	memcpy(&geometry[0],verts,size);

	// auto-mapping particle shader pipeline
	vao.bind();
	vbo.bind();
	vbo.upload_vertices(geometry);
	shader->map(RENDERER_TEXTURE_SPRITES,&vbo,&ibo);

	// store geometry information
	vertex_count = vsize;
	active_particles = particles;
}


// ----------------------------------------------------------------------------------------------------
// Renderer Main Features

/**
 *	setup renderer
 */
Renderer::Renderer()
{
	COMM_MSG(LOG_CYAN,"starting render system");

	COMM_LOG("starting font rasterizer");
	bool _failed = FT_Init_FreeType(&g_FreetypeLibrary);
	COMM_ERR_COND(_failed,"text rasterizer not available");

	COMM_LOG("pre-loading basic geometry data");
	f32 __QuadVertices[] = {
		-.5f,.5f,.0f,.0f, .5f,-.5f,1.f,1.f, .5f,.5f,1.f,.0f,
		.5f,-.5f,1.f,1.f, -.5f,.5f,.0f,.0f, -.5f,-.5f,.0f,1.f
	};
	f32 __CanvasVertices[] = {
		-1.f,1.f,.0f,1.f, 1.f,-1.f,1.f,.0f, 1.f,1.f,1.f,1.f,
		1.f,-1.f,1.f,.0f, -1.f,1.f,.0f,1.f, -1.f,-1.f,.0f,.0f
	};

	COMM_LOG("compiling shaders");
	VertexShader __SpriteVertexShader = VertexShader("core/shader/sprite.vert");
	FragmentShader __DirectFragmentShader = FragmentShader("core/shader/sprite.frag");
	VertexShader __TextVertexShader = VertexShader("core/shader/text.vert");
	FragmentShader __TextFragmentShader = FragmentShader("core/shader/text.frag");
	VertexShader __CanvasVertexShader = VertexShader("core/shader/canvas.vert");
	FragmentShader __LightingPassFragmentShader = FragmentShader("core/shader/pbs.frag");
	VertexShader __GeometryPassVertexShader = VertexShader("core/shader/gpass.vert");
	FragmentShader __GeometryPassFragmentShader = FragmentShader("core/shader/gpass.frag");
	VertexShader __ParticlePassVertexShader = VertexShader("core/shader/ipass.vert");
	FragmentShader __ParticlePassFragmentShader = FragmentShader("core/shader/ipass.frag");
	VertexShader __GeometryShadowVertexShader = VertexShader("core/shader/gshadow.vert");
	VertexShader __ParticleShadowVertexShader = VertexShader("core/shader/ishadow.vert");
	FragmentShader __ShadowFragmentShader = FragmentShader("core/shader/shadow.frag");

	// ----------------------------------------------------------------------------------------------------
	// Sprite Pipeline

	COMM_LOG("assembling pipelines:");
	COMM_LOG("sprite pipeline");
	m_SpritePipeline.assemble(__SpriteVertexShader,__DirectFragmentShader);
	m_SpriteVertexArray.bind();
	m_SpriteVertexBuffer.bind();
	m_SpriteVertexBuffer.upload_vertices(__QuadVertices,24);
	m_SpritePipeline.map(RENDERER_TEXTURE_SPRITES,&m_SpriteVertexBuffer,&m_SpriteInstanceBuffer);
	m_SpritePipeline.upload_coordinate_system();

	COMM_LOG("text pipeline");
	m_TextPipeline.assemble(__TextVertexShader,__TextFragmentShader);
	m_TextVertexArray.bind();
	m_SpriteVertexBuffer.bind();
	m_TextPipeline.map(RENDERER_TEXTURE_FONTS,&m_SpriteVertexBuffer,&m_TextInstanceBuffer);
	m_TextPipeline.upload_coordinate_system();

	COMM_LOG("canvas pipeline");
	m_CanvasPipeline.assemble(__CanvasVertexShader,__LightingPassFragmentShader);
	m_CanvasVertexArray.bind();
	m_CanvasVertexBuffer.bind();
	m_CanvasVertexBuffer.upload_vertices(__CanvasVertices,24);
	m_CanvasPipeline.map(RENDERER_TEXTURE_FORWARD,&m_CanvasVertexBuffer);

	COMM_LOG("geometry pass pipelines");
	m_GeometryPassPipeline = register_pipeline(__GeometryPassVertexShader,__GeometryPassFragmentShader);
	m_ParticlePassPipeline = register_pipeline(__ParticlePassVertexShader,__ParticlePassFragmentShader);

	COMM_LOG("shadow projection piplines");
	m_GeometryShadowPipeline = register_pipeline(__GeometryShadowVertexShader,__ShadowFragmentShader);
	m_ParticleShadowPipeline = register_pipeline(__ParticleShadowVertexShader,__ShadowFragmentShader);

	// ----------------------------------------------------------------------------------------------------
	// GPU Memory

	COMM_LOG("allocating sprite memory");
	m_GPUSpriteTextures.atlas.bind(RENDERER_TEXTURE_SPRITES);
	m_GPUSpriteTextures.allocate(RENDERER_SPRITE_MEMORY_WIDTH,RENDERER_SPRITE_MEMORY_HEIGHT,GL_RGBA);
	Texture::set_texture_parameter_linear_mipmap();
	Texture::set_texture_parameter_clamp_to_edge();

	COMM_LOG("allocating font memory");
	m_GPUFontTextures.atlas.bind(RENDERER_TEXTURE_FONTS);
	m_GPUFontTextures.allocate(RENDERER_FONT_MEMORY_WIDTH,RENDERER_FONT_MEMORY_HEIGHT,GL_RED);
	Texture::set_texture_parameter_linear_mipmap();
	Texture::set_texture_parameter_clamp_to_edge();

	// ----------------------------------------------------------------------------------------------------
	// Render Targets

	COMM_LOG("creating forward render target");
	m_ForwardFrameBuffer.start();
	m_ForwardFrameBuffer.define_colour_component(0,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_ForwardFrameBuffer.define_depth_component(FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_ForwardFrameBuffer.finalize();

	COMM_LOG("creating deferred render target");
	m_DeferredFrameBuffer.start();
	m_DeferredFrameBuffer.define_colour_component(0,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_DeferredFrameBuffer.define_colour_component(1,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,true);
	m_DeferredFrameBuffer.define_colour_component(2,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,true);
	m_DeferredFrameBuffer.define_colour_component(3,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y,true);
	m_DeferredFrameBuffer.define_colour_component(4,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_DeferredFrameBuffer.define_depth_component(FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_DeferredFrameBuffer.finalize();

	COMM_LOG("creating shadow projection render target");
	m_ShadowFrameBuffer.start();
	m_ShadowFrameBuffer.define_depth_component(RENDERER_SHADOW_RESOLUTION,RENDERER_SHADOW_RESOLUTION);
	Texture::set_texture_parameter_clamp_to_border();
	Texture::set_texture_parameter_border_colour(vec4(1));
	Framebuffer::stop();

	// ----------------------------------------------------------------------------------------------------
	// Start Subprocesses

	COMM_LOG("starting renderer subprocesses");
	_sprite_signal.stall();
	m_SpriteCollector = thread(Renderer::_collector<Sprite>,&m_Sprites,&_sprite_signal);
	m_SpriteCollector.detach();
	_sprite_texture_signal.stall();
	m_SpriteTextureCollector = thread(Renderer::_collector<PixelBufferComponent>,
									  &m_GPUSpriteTextures.textures,&_sprite_texture_signal);
	m_SpriteTextureCollector.detach();
	COMM_SCC("render system ready.");
}
// TODO join collector processes when exiting renderer, or maybe just let the os handle that and not care?

/**
 *	render visual result
 */
void Renderer::update()
{
	m_FrameStart = std::chrono::steady_clock::now();

	// shadow projection
	glCullFace(GL_FRONT);
	glViewport(0,0,RENDERER_SHADOW_RESOLUTION,RENDERER_SHADOW_RESOLUTION);
	m_ShadowFrameBuffer.start();
	_update_shadows(m_ShadowGeometryBatches,m_ShadowParticleBatches);
	glCullFace(GL_BACK);

	// 3D segment
	glViewport(0,0,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_ForwardFrameBuffer.start();
	_update_mesh(m_GeometryBatches,m_ParticleBatches);
	m_DeferredFrameBuffer.start();
	_update_mesh(m_DeferredGeometryBatches,m_DeferredParticleBatches);
	Framebuffer::stop();

	// rendertargets
	glDisable(GL_DEPTH_TEST);
	_update_canvas();
	glEnable(GL_DEPTH_TEST);

	// 2D segment
	_update_sprites();
	_update_text();

	// end-frame gpu management
	_gpu_upload();
}

/**
 *	exit renderer and end all it's subprocesses
 */
void Renderer::exit()
{
	_sprite_texture_signal.exit();
	_sprite_signal.exit();
}

/**
 *	register sprite texture to load and move to sprite pixel buffer
 *	\param path: path to texture file
 *	\returns pointer to texture component info to assign to a sprite later
 */
PixelBufferComponent* Renderer::register_sprite_texture(const char* path)
{
	PixelBufferComponent* p_Comp = m_GPUSpriteTextures.textures.next_free();
	m_GPUSpriteTextures.signal.stall();

	COMM_LOG("sprite texture register of %s",path);
	thread __LoadThread(GPUPixelBuffer::load_texture,&m_GPUSpriteTextures,p_Comp,path);
	__LoadThread.detach();

	return p_Comp;
}

/**
 *	register a new sprite instance for rendering
 *	\param texture: sprite texture to be assigned to the sprite canvas
 *	\param position: 2-dimensional position of sprite on screen, bounds defined by coordinate system
 *	\param size: width and height of the sprite
 *	\param rotation: (default .0f) rotation of the sprite in degrees
 *	\param alpha: (default 1.f) transparency of sprite clamped between 0 and 1. 0 = invisible -> 1 = opaque
 *	\param alignment: (default fullscreen neutral) sprite position alignment within borders
 *	\returns pointer to sprite data for modification purposes
 */
Sprite* Renderer::register_sprite(PixelBufferComponent* texture,vec3 position,vec2 size,f32 rotation,
								  f32 alpha,Alignment alignment)
{
	// determine memory location, overwrite has priority over appending
	Sprite* p_Sprite = m_Sprites.next_free();
	COMM_LOG("sprite register at: (%f,%f), %fx%f, %f° -> count = %d",
			 position.x,position.y,size.x,size.y,rotation,m_Sprites.active_range);

	// align sprite into borders
	if (alignment.align!=SCREEN_ALIGN_NEUTRAL)
	{
		vec2 hsize = size*.5f;
		vec2 __AlignedPosition = align({ vec2(position)-hsize,size },alignment)+size;
		position.x = __AlignedPosition.x;
		position.y = __AlignedPosition.y;
	}

	// write information to memory
	(*p_Sprite) = {
		.offset = position,
		.scale = size,
		.rotation = rotation,
		.alpha = alpha,
	};
	Renderer::assign_sprite_texture(p_Sprite,texture);
	return p_Sprite;
}

/**
 *	assign a sprite texture to a sprite canvas
 *	\param sprite: pointer to the sprite canvas received at creation
 *	\param texture: pointer to texture component info received at load request
 */
void Renderer::assign_sprite_texture(Sprite* sprite,PixelBufferComponent* texture)
{
	m_GPUSpriteTextures.signal.wait();
	sprite->tex_position = texture->offset;
	sprite->tex_dimension = texture->dimensions;
}

/**
 *	remove given sprite texture and free memory in array as well as releasing memory space on atlas
 *	\param texture: pointer to texture, which shall be removed
 */
void Renderer::delete_sprite_texture(PixelBufferComponent* texture)
{
	// signal cleanup
	texture->offset.x = RENDERER_POSITIONAL_DELETION_CODE;
	_sprite_texture_signal.proceed();

	// free texture atlas memory
	m_GPUSpriteTextures.mutex_memory_segments.lock();
	m_GPUSpriteTextures.memory_segments.push_back(*texture);
	m_GPUSpriteTextures.mutex_memory_segments.unlock();
	// TODO merge segments after adding free section to reduce segmentation
}

/**
 *	remove sprite from render list. quickly scaled invisible in main thread, later collected automatically
 *	\param sprite: reference to sprite, being removed
 */
void Renderer::delete_sprite(Sprite* sprite)
{
	sprite->offset.x = RENDERER_POSITIONAL_DELETION_CODE;
	sprite->scale = vec2(0,0);
	sprite = nullptr;
	_sprite_signal.proceed();
}

/**
 *	rasterize a vector font and upload pixel buffer to gpu memory
 *	\param path: path to .ttf vector font file
 *	\param size: rasterization size
 *	\returns font data memory, to use later when writing text with or in style of it
 */
Font* Renderer::register_font(const char* path,u16 size)
{
	COMM_LOG("font register from source %s",path);
	Font* p_Font = m_Fonts.next_free();
	m_GPUFontTextures.signal.stall();
	thread __LoadThread(GPUPixelBuffer::load_font,&m_GPUFontTextures,p_Font,path,size);
	__LoadThread.detach();
	return p_Font;
}

/**
 *	write text on screen
 *	\param font: pointer to loaded font
 *	\param data: text content to be displayed in given font
 *	\param position: positional offset of the text based on screen alignment
 *	\param scale: intuitive absolute text scaling in pixels, supported by automatic adaptive resolution
 *	\param colour: (default vec4(1)) text starting colour of all characters
 *	\param align: (default SCREEN_ALIGN_BOTTOMLEFT) text alignment on screen, modified by positional offset
 *	\returns list container of created text
 */
lptr<Text> Renderer::write_text(Font* font,string data,vec3 position,f32 scale,vec4 colour,Alignment align)
{
	m_GPUFontTextures.signal.wait();
	m_Texts.push_back({
			.font = font,
			.position = position,
			.scale = (f32)scale/font->size,
			.colour = colour,
			.alignment = align,
			.data = data
		});

	lptr<Text> p_Text = std::prev(m_Texts.end());
	p_Text->align();
	p_Text->load_buffer();
	return p_Text;
}

/**
 *	load texture into ram in background and register for vram upload when ready
 *	\param texture: pointer to texture in memory
 *	\param path: path to texture
 *	\param format: texture colour channel format
 *	\param data_queue: queue for texture vram upload
 *	\param queue_mutex: mutual exclusion for data queue to prevent race conditions
 */
void _load_texture(Texture* texture,const char* path,TextureFormat format,
				   queue<TextureDataTuple>* data_queue,std::mutex* queue_mutex)
{
	TextureData __Data = TextureData(format);
	__Data.load(path);
	queue_mutex->lock();
	data_queue->push(TextureDataTuple{ __Data,texture });
	queue_mutex->unlock();
}

/**
 *	load texture into memory
 *	\param path: path to texture file
 *	\param format: (default TEXTURE_FORMAT_RGBA) texture colour channel format
 *	\returns pointer to texture in ram, referencing texture in vram
 */
Texture* Renderer::register_texture(const char* path,TextureFormat format)
{
	COMM_LOG("mesh texture register of %s",path);
	Texture* p_Texture = m_MeshTextures.next_free();
	new(p_Texture) Texture();
	thread __LoadThread(_load_texture,p_Texture,path,format,&m_MeshTextureUploadQueue,&m_MutexMeshTextureUpload);
	__LoadThread.detach();
	return p_Texture;
}

/**
 *	register shader pipeline
 *	\param vs: vertex shader
 *	\param fs: fragment shader
 *	returns pointer to registered shader pipeline
 */
lptr<ShaderPipeline> Renderer::register_pipeline(VertexShader& vs,FragmentShader& fs)
{
	m_ShaderPipelines.push_back(ShaderPipeline());
	lptr<ShaderPipeline> p_Pipeline = std::prev(m_ShaderPipelines.end());
	p_Pipeline->assemble(vs,fs);
	return p_Pipeline;
}

/**
 *	register triangle mesh batch
 *	\param pipeline: shader pipeline, handling pixel output for newly created batch
 *	\returns pointer to created triangle mesh batch
 */
lptr<GeometryBatch> Renderer::register_geometry_batch(lptr<ShaderPipeline> pipeline)
{
	m_GeometryBatches.push_back({ .shader = pipeline });
	return std::prev(m_GeometryBatches.end());
}

/**
 *	register phyiscal mesh batch with standard geometry pass shader
 *	\returns pointer to created physical mesh batch
 */
lptr<GeometryBatch> Renderer::register_deferred_geometry_batch()
{
	m_DeferredGeometryBatches.push_back({ .shader = m_GeometryPassPipeline });
	return std::prev(m_DeferredGeometryBatches.end());
}

/**
 *	register physical mesh batch
 *	\param pipeline: shader pipeline, handling physical pass for newly created batch
 *	\returns pointer to created physical mesh batch
 */
lptr<GeometryBatch> Renderer::register_deferred_geometry_batch(lptr<ShaderPipeline> pipeline)
{
	m_DeferredGeometryBatches.push_back({ .shader = pipeline });
	return std::prev(m_DeferredGeometryBatches.end());
}

/**
 *	register particle batch
 *	\param pipeline: shader pipeline, handling pixel output for newly created batch
 *	\returns pointer to created particle batch
 */
lptr<ParticleBatch> Renderer::register_particle_batch(lptr<ShaderPipeline> pipeline)
{
	m_ParticleBatches.push_back({ .shader = pipeline });
	return std::prev(m_ParticleBatches.end());
}

/**
 *	register phyiscal particle batch
 *	\returns pointer to created physical particle batch
 */
lptr<ParticleBatch> Renderer::register_deferred_particle_batch()
{
	m_DeferredParticleBatches.push_back({ .shader = m_ParticlePassPipeline });
	return std::prev(m_DeferredParticleBatches.end());
}

/**
 *	register phyiscal particle batch
 *	\param pipeline: shader pipeline, handling physical pass for newly created batch
 *	\returns pointer to created physical particle batch
 */
lptr<ParticleBatch> Renderer::register_deferred_particle_batch(lptr<ShaderPipeline> pipeline)
{
	m_DeferredParticleBatches.push_back({ .shader = pipeline });
	return std::prev(m_DeferredParticleBatches.end());
}

/**
 *	allow a geometry batch to cast shadows onto the scene
 *	\param b: pointer to casting geometry batch
 */
void Renderer::register_shadow_batch(lptr<GeometryBatch> b)
{
	m_ShadowGeometryBatches.push_back(b);
	/*
	b->vao.bind();
	b->vbo.bind();
	m_GeometryShadowPipeline->map(RENDERER_TEXTURE_UNMAPPED,&b->vbo);
	*/
}

/**
 *	allow a particle batch to cast shadows onto the scene
 *	\param b: pointer to casting particle batch
 */
void Renderer::register_shadow_batch(lptr<ParticleBatch> b)
{
	m_ShadowParticleBatches.push_back(b);
	/*
	b->vao.bind();
	b->vbo.bind();
	m_ParticleShadowPipeline->map(RENDERER_TEXTURE_UNMAPPED,&b->vbo,&b->ibo);
	*/
}

/**
 *	create directional sunlight
 *	\param position: direction to sunlight, inverted direction will be direction of emission
 *	\param colour: colour of the emission
 *	\param intensity: emission intensity, multiplying the colour influence
 */
SunLight* Renderer::add_sunlight(vec3 position,vec3 colour,f32 intensity)
{
	m_Lighting.sunlights[m_Lighting.sunlights_active] = {
		.position = position,
		.colour = colour*intensity,
	};
	return &m_Lighting.sunlights[m_Lighting.sunlights_active++];
}

/**
 *	create pointlight
 *	\param position: position of the light emitter
 *	\param colour: colour of the emission
 *	\param intensity: emission intensity, multiplying the colour influence
 *	\param constant: constant component in attenuation
 *	\param linear: linear component in attenutation
 *	\param quadratic: quadratic component in attenuation
 *	\returns pointer to pointlight
 */
PointLight* Renderer::add_pointlight(vec3 position,vec3 colour,f32 intensity,f32 constant,
									 f32 linear,f32 quadratic)
{
	m_Lighting.pointlights[m_Lighting.pointlights_active] = {
		.position = position,
		.colour = colour*intensity,
		.constant = constant,
		.linear = linear,
		.quadratic = quadratic
	};
	return &m_Lighting.pointlights[m_Lighting.pointlights_active++];
}

/**
 *	create a shadow projection source
 *	\param source: position of projection source
 */
void Renderer::add_shadow(vec3 source)
{
	m_Lighting.shadow_projection = Camera3D(vec3(0),source,RENDERER_SHADOW_RANGE,RENDERER_SHADOW_RANGE,
											.1f,1000.f);
}
// TODO allow for multiple shadows to project at the same time
// TODO also create support for pointlight shadows

/**
 *	upload all setup lights to gpu lighting simulation processing
 */
void Renderer::upload_lighting()
{
	m_CanvasPipeline.enable();

	// upload directionlights
	for (u8 i=0;i<m_Lighting.sunlights_active;i++)
	{
		SunLight& light = m_Lighting.sunlights[i];
		string __ArrayLocation = "sunlights["+std::to_string(i)+"].";
		m_CanvasPipeline.upload((__ArrayLocation+"position").c_str(),light.position);
		m_CanvasPipeline.upload((__ArrayLocation+"colour").c_str(),light.colour);
	}
	m_CanvasPipeline.upload("sunlights_active",m_Lighting.sunlights_active);

	// upload pointlights
	for (u8 i=0;i<m_Lighting.pointlights_active;i++)
	{
		PointLight& light = m_Lighting.pointlights[i];
		string __ArrayLocation = "pointlights["+std::to_string(i)+"].";
		m_CanvasPipeline.upload((__ArrayLocation+"position").c_str(),light.position);
		m_CanvasPipeline.upload((__ArrayLocation+"colour").c_str(),light.colour);
		m_CanvasPipeline.upload((__ArrayLocation+"constant").c_str(),light.constant);
		m_CanvasPipeline.upload((__ArrayLocation+"linear").c_str(),light.linear);
		m_CanvasPipeline.upload((__ArrayLocation+"quadratic").c_str(),light.quadratic);
	}
	m_CanvasPipeline.upload("pointlights_active",m_Lighting.pointlights_active);
}
// TODO dynamic upload, e.g. when a single light gets updated all the lights need to be uploaded again

/**
 *	deactivate all lights to fundamentally reset the lighting setup
 */
void Renderer::reset_lighting()
{
	m_Lighting.sunlights_active = 0;
	m_Lighting.pointlights_active = 0;
	upload_lighting();
}

/**
 *	geometry realignment based on position
 *	\param geom: intersection rectangle over aligning geometry
 *	\param alignment: (default fullscreen neutral) target alignment within specified border
 *	\returns new position of geometry after alignment process
 */
vec2 Renderer::align(Rect geom,Alignment alignment)
{
	// setup
	vec2 __Position = geom.position;
	vec2 __GeomCenter = geom.extent*vec2(.5f);
	vec2 __BorderCenter = alignment.border.extent*vec2(.5f)+alignment.border.position;

	// adjust vertical alignment
	u8 vertical_alignment = 2-(alignment.align%3);
	__Position.y += vertical_alignment*(__BorderCenter.y-__GeomCenter.y);

	// adjust horizontal alignment
	u8 horizontal_alignment = alignment.align/3;
	if (!!horizontal_alignment) __Position.x += horizontal_alignment*(__BorderCenter.x-__GeomCenter.x);
	return __Position;
}

/**
 *	update all registered sprites
 */
void Renderer::_update_sprites()
{
	m_SpriteVertexArray.bind();
	m_SpriteInstanceBuffer.bind();
	m_SpriteInstanceBuffer.upload_vertices(m_Sprites.mem,BUFFER_MAXIMUM_TEXTURE_COUNT,GL_DYNAMIC_DRAW);
	m_SpritePipeline.enable();
	glDrawArraysInstanced(GL_TRIANGLES,0,6,m_Sprites.active_range);
}

/**
 *	update all registered text
 */
void Renderer::_update_text()
{
	// prepare gpu
	m_TextVertexArray.bind();
	m_TextInstanceBuffer.bind();
	m_TextPipeline.enable();

	// iterate text entities
	for (Text& p_Text : m_Texts)
	{
		m_TextInstanceBuffer.upload_vertices(p_Text.buffer,GL_DYNAMIC_DRAW);
		glDrawArraysInstanced(GL_TRIANGLES,0,6,p_Text.buffer.size());
	}
}

/**
 *	update framebuffer representations
 */
void Renderer::_update_canvas()
{
	m_CanvasVertexArray.bind();
	m_CanvasPipeline.enable();
	m_ForwardFrameBuffer.bind_colour_component(RENDERER_TEXTURE_FORWARD,0);
	m_DeferredFrameBuffer.bind_colour_component(RENDERER_TEXTURE_DEFERRED_COLOUR,0);
	m_DeferredFrameBuffer.bind_colour_component(RENDERER_TEXTURE_DEFERRED_POSITION,1);
	m_DeferredFrameBuffer.bind_colour_component(RENDERER_TEXTURE_DEFERRED_NORMAL,2);
	m_DeferredFrameBuffer.bind_colour_component(RENDERER_TEXTURE_DEFERRED_MATERIAL,3);
	m_DeferredFrameBuffer.bind_colour_component(RENDERER_TEXTURE_DEFERRED_EMISSION,4);
	m_ShadowFrameBuffer.bind_depth_component(RENDERER_TEXTURE_SHADOW_MAP);
	m_ForwardFrameBuffer.bind_depth_component(RENDERER_TEXTURE_FORWARD_DEPTH);
	m_DeferredFrameBuffer.bind_depth_component(RENDERER_TEXTURE_DEFERRED_DEPTH);
	m_CanvasPipeline.upload("camera_position",g_Camera.position);
	m_CanvasPipeline.upload("shadow_source",m_Lighting.shadow_projection.position);
	m_CanvasPipeline.upload("shadow_projection",
							m_Lighting.shadow_projection.proj*m_Lighting.shadow_projection.view);
	// TODO do this in upload lighting process later
	glDrawArrays(GL_TRIANGLES,0,6);
}

/**
 *	update triangle meshes
 *	\param gb: geometry batches to draw contained geometry from
 *	\param pb: particle batches to draw contained particles geometry from
 */
void Renderer::_update_mesh(list<GeometryBatch>& gb,list<ParticleBatch>& pb)
{
	// iterate static geometry
	for (GeometryBatch& p_Batch : gb)
	{
		p_Batch.shader->enable();
		p_Batch.vao.bind();
		for (GeometryTuple& p_Tuple : p_Batch.object)
		{
			// texture upload
			for (u8 i=0;i<p_Tuple.textures.size();i++) p_Tuple.textures[i]->bind(RENDERER_TEXTURE_UNMAPPED+i);

			// upload attached uniform value pointers
			p_Batch.shader->upload_camera();
			for (GeometryUniformUpload& p_Upload : p_Tuple.uploads)
				p_Batch.shader->upload(p_Upload.uloc,p_Upload.udim,p_Upload.data);

			// upload standard values & call gpu
			p_Batch.shader->upload("model",p_Tuple.transform.model);
			p_Batch.shader->upload("texel",p_Tuple.texel);
			//glDrawArrays(GL_TRIANGLES,p_Tuple.offset,p_Tuple.vertex_count);
			glDrawElements(GL_TRIANGLES,p_Tuple.vertex_count,
						   GL_UNSIGNED_INT,(void*)(p_Tuple.offset*sizeof(u32)));
		}
	}
	// FIXME uploading camera and then afterwards maybe overwrite it is working but it is shite

	// iterate particle geometry
	for (ParticleBatch& p_Batch : pb)
	{
		p_Batch.shader->enable();
		p_Batch.shader->upload_camera();
		p_Batch.vao.bind();
		glDrawArraysInstanced(GL_TRIANGLES,0,p_Batch.vertex_count,p_Batch.active_particles);
	}
}

/**
 *	draw casting geometry simplified for shadow projection
 *	\param gb: casting geometry batches for shadow projection
 *	\param pb: casting particle batches for shadow projection
 */
void Renderer::_update_shadows(list<lptr<GeometryBatch>>& gb,list<lptr<ParticleBatch>>& pb)
{
	// iterate static geometry
	for (lptr<GeometryBatch> p_Batch : gb)
	{
		m_GeometryShadowPipeline->enable();  // TODO make this dynamic
		m_GeometryShadowPipeline->upload_camera(m_Lighting.shadow_projection);
		p_Batch->vao.bind();
		for (GeometryTuple& p_Tuple : p_Batch->object)
		{
			// TODO geometry uniform upload, this will only be applicable if dynamic shading pipeline is working
			m_GeometryShadowPipeline->upload("model",p_Tuple.transform.model);
			//glDrawArrays(GL_TRIANGLES,p_Tuple.offset,p_Tuple.vertex_count);
			glDrawElements(GL_TRIANGLES,p_Tuple.vertex_count,
						   GL_UNSIGNED_INT,(void*)(p_Tuple.offset*sizeof(u32)));
		}
	}

	// iterate particle geometry
	for (lptr<ParticleBatch> p_Batch : pb)
	{
		m_ParticleShadowPipeline->enable();  // TODO same here as with m_GeometryShadowPipeline
		m_ParticleShadowPipeline->upload_camera(m_Lighting.shadow_projection);
		p_Batch->vao.bind();
		glDrawArraysInstanced(GL_TRIANGLES,0,p_Batch->vertex_count,p_Batch->active_particles);
	}
}

/**
 *	helper to unclutter the automatic load callbacks for gpu data
 */
void Renderer::_gpu_upload()
{
	m_GPUSpriteTextures.gpu_upload(RENDERER_TEXTURE_SPRITES,m_FrameStart);
	m_GPUFontTextures.gpu_upload(RENDERER_TEXTURE_FONTS,m_FrameStart);

	// singular textures
	m_MutexMeshTextureUpload.lock();
	while (m_MeshTextureUploadQueue.size()&&calculate_delta_time(m_FrameStart)<FRAME_TIME_BUDGET_MS)
	{
		TextureDataTuple& p_Tuple = m_MeshTextureUploadQueue.front();
		p_Tuple.texture->bind(RENDERER_TEXTURE_UNMAPPED);
		p_Tuple.data.gpu_upload();
		m_MeshTextureUploadQueue.pop();
		Texture::set_texture_parameter_linear_mipmap();
		Texture::set_texture_parameter_repeat();
		Texture::generate_mipmap();
	}
	m_MutexMeshTextureUpload.unlock();
}
// FIXME the same is happening in buffer.cpp, it seems untidy and is worth another thought


// ----------------------------------------------------------------------------------------------------
// Background Processes

/**
 *	automatically collecting deleted sprites and assign memory space for override
 *	\param xs: collectable array structure holding removable sprites conforming to the collection rules:
 *			- remove coding is in offset.x as RENDERER_POSITIONAL_DELETION_CODE
 *			- has to be stored as an InPlaceArray to support overwrite and range system
 *	\param signal: background collector needs an activation signal to know when it is sensible to collect
 */
template<typename T> void Renderer::_collector(InPlaceArray<T>* xs,ThreadSignal* signal)
{
	COMM_SCC("started %s collector background process",signal->name);

	// main loop
	while (signal->running)
	{
		signal->wait();
		signal->stall();
		if (!signal->running) break;
		COMM_LOG("%s collector is searching for removed objects...",signal->name);

		// iterate active sprite memory
		xs->overwrites = queue<u16>();
		u16 __Streak = 0;
		for (int i=0;i<xs->active_range;i++)
		{
			// sprite marked to be removed
			if (xs->mem[i].offset.x==RENDERER_POSITIONAL_DELETION_CODE)
			{
				COMM_MSG(LOG_PURPLE,"marked %s found at memory index %d and scheduled to overwrite",
						 signal->name,i);
				// FIXME i'm not so sure i like the logging here
				__Streak++;
				if (i==xs->active_range-1) xs->active_range -= __Streak;
				else xs->overwrites.push(i);
			}

			// end removal streak
			else __Streak = 0;
		}
	}

	COMM_LOG("%s collector background process finished",signal->name);
}
template void Renderer::_collector<Sprite>(InPlaceArray<Sprite>*,ThreadSignal*);
template void Renderer::_collector<PixelBufferComponent>(InPlaceArray<PixelBufferComponent>*,ThreadSignal*);
