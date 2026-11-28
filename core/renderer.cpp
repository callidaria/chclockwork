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
// Materials

// standard neutral fallback materials for physical based lighting calculations
Texture* Material::standards[MATERIAL_COMPONENT_CHANNEL_COUNT] = {
	g_Renderer.register_texture(vec3(1.f)),
	g_Renderer.register_texture(vec3(.5f,.5f,1)),
	g_Renderer.register_texture(vec3(0,1,1)),
	g_Renderer.register_texture(vec3(.0f))
};


// ----------------------------------------------------------------------------------------------------
// Alignment

/**
 *	geometry realignment based on position
 *	\param geom: intersection rectangle over aligning geometry
 *	\returns new position of geometry after alignment process
 */
vec2 Alignment::align(Rect geom)
{
	// setup
	vec2 __Position = geom.position;
	vec2 __GeomCenter = geom.extent*vec2(.5f);
	vec2 __BorderCenter = border.extent*vec2(.5f)+border.position;

	// alignment status & coordinate flip for ogl version
	u8 vertical_alignment = alignment%3;
#ifdef GLBUILD
	2-vertical_alignment;
#endif

	// adjust vertical alignment
	__Position.y += vertical_alignment*(__BorderCenter.y-__GeomCenter.y);

	// adjust horizontal alignment
	u8 horizontal_alignment = alignment/3;
	if (!!horizontal_alignment) __Position.x += horizontal_alignment*(__BorderCenter.x-__GeomCenter.x);
	return __Position;
}


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
	if (alignment.alignment<SCREEN_ALIGN_NEUTRAL)
	{
		vec2 __AlignedOffset = alignment.align({ position,dimensions });
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
	bool __SegmentInsufficient = buffer.size()<data.size();
	COMM_LOG_COND(__SegmentInsufficient,"allocating memory for text buffer");

	// allocate more memory when necessary
	if (__SegmentInsufficient)
	{
		size_t __Start = buffer.size();
		buffer.resize(data.size());
		for (size_t i=__Start;i<data.size();i++) buffer[i] = characters.next_free();
	}

	// load font information for characters
	vec3 __Cursor = vec3(offset.x,offset.y,position.z);
	for (u32 i=0;i<data.size();i++)
	{
		TextCharacter* p_Character = &characters.mem[buffer[i]];
		Rect& p_Component = font->tex[data[i]-32];
		Glyph& p_Glyph = font->glyphs[data[i]-32];

		// load text data
		*p_Character = {
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

/**
 *	remove text
 */
void Text::vanish()
{
	for (size_t c : buffer)
		characters.free_element(c,{ .offset=vec3(RENDERER_POSITIONAL_DELETION_CODE) });
}


// ----------------------------------------------------------------------------------------------------
// Meshes

// basic shapes preload
Mesh Mesh::SPHERE_LOWRES = Mesh("./res/primitive/low_sphere.obj");
Mesh Mesh::SPHERE = Mesh("./res/primitive/sphere.obj");
Mesh Mesh::CUBE = Mesh("./res/primitive/cube.obj");
Mesh Mesh::APE = Mesh("./res/primitive/ape.obj");
f32 Mesh::CUBEMAP_CUBE[CUBEMAP_CUBE_BUFFER_SIZE] = {
	 1,-1,-1,  1, 1,-1,  1, 1, 1,  1,-1,-1,  1, 1, 1,  1,-1, 1,  // +x
	-1,-1,-1, -1,-1, 1, -1, 1, 1, -1,-1,-1, -1, 1, 1, -1, 1,-1,  // -x
	 1, 1,-1, -1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1,-1, -1, 1, 1,  // +y
	-1,-1,-1,  1,-1,-1, -1,-1, 1, -1,-1, 1,  1,-1,-1,  1,-1, 1,  // -y
	 1,-1, 1,  1, 1, 1, -1,-1, 1, -1,-1, 1,  1, 1, 1, -1, 1, 1,  // +z
	 1, 1,-1,  1,-1,-1, -1, 1,-1,  1,-1,-1, -1,-1,-1, -1, 1,-1,  // -z
};

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


// ----------------------------------------------------------------------------------------------------
// Animations

/**
 *	request a positional change towards a target in a given time
 *	\param target: animation key, holding positional target & transition duration
 */
void MeshJoint::request_position(const AnimKey<vec3>& target)
{
	prog_position = 0;
	crr_position.key = ct_position;
	crr_position.duration = 0;
	target_position = { target.key,1./target.duration };
}

/**
 *	request a scaling change towards a target in a given time
 *	\param target: animation key, holding scaling target & transition duration
 */
void MeshJoint::request_scale(const AnimKey<vec3>& target)
{
	prog_scale = 0;
	crr_scale.key = ct_scale;
	crr_scale.duration = 0;
	target_scale = { target.key,1./target.duration };
}

/**
 *	request a rotational change towards a target in a given time
 *	\param target: animation key, holding rotation target & transition duration
 */
void MeshJoint::request_rotation(const AnimKey<quat>& target)
{
	prog_rotation = 0;
	crr_rotation.key = ct_rotation;
	crr_rotation.duration = 0;
	target_rotation = { target.key,1./target.duration };
}

/**
 * interpolate between joint transformation based on given data in struct
 */
void MeshJoint::interpolate()
{
	// time advancement
	prog_position += g_Frame.delta_time;
	prog_scale += g_Frame.delta_time;
	prog_rotation += g_Frame.delta_time;

	// calculate key progression
	f32 dt_position = glm::clamp(prog_position*target_position.duration,.0,1.);
	f32 dt_scale = glm::clamp(prog_scale*target_scale.duration,.0,1.);
	f32 dt_rotation = glm::clamp(prog_rotation*target_rotation.duration,.0,1.);
	// FIXME also this darn clamping here has to be removed. this uses processing power for basically nothing?
	// FIXME this is really slow! this approach has unique inversions everytime the ct key is stored.

	// interpolation
	ct_position = glm::mix(crr_position.key,target_position.key,dt_position);
	ct_scale = glm::mix(crr_scale.key,target_scale.key,dt_scale);
	ct_rotation = glm::slerp(crr_rotation.key,target_rotation.key,dt_rotation);
	transform = glm::translate(mat4(1.f),ct_position)*glm::scale(mat4(1.f),ct_scale)*glm::toMat4(ct_rotation);
}

/**
 *	(called by AnimatedMesh::AnimatedMesh())
 *	recursively counts joints in mesh tree structure
 *	\param root: root node of counting subtree
 *	\returns number of joints in this subtree counting parent, children and all subsequent children
 */
u16 _rc_get_joint_count(aiNode* root)
{
	u16 __Result = 1;
	for (u16 i=0;i<root->mNumChildren;i++) __Result += _rc_get_joint_count(root->mChildren[i]);
	return __Result;
}

/**
 *	(called by AnimatedMesh::AnimatedMesh())
 *	recursively assemble joint hierarchy by transferring the tree into a flat structure by depthsearch
 *	\param joints: reference to joint list, that will be filled by this function
 *	\param root: root note of current subtree
 */
void _rc_assemble_joint_hierarchy(vector<MeshJoint>& joints,aiNode* root)
{
	// root joint translation
	u16 __MemoryID = joints.size();
	joints.push_back({
			.id = root->mName.C_Str(),
#ifdef VKBUILD
			.uniform_location = __MemoryID,
#else
			.uniform_location = "joint_transform["+std::to_string(__MemoryID)+"]",
#endif
			.transform = to_mat4(root->mTransformation),
			.children = vector<u16>(root->mNumChildren)
		});
	MeshJoint& __Joint = joints.back();

	// extract transformation components
	decompose(__Joint.transform,__Joint.crr_position.key,__Joint.crr_scale.key,__Joint.crr_rotation.key);
	__Joint.target_position.key = __Joint.crr_position.key;
	__Joint.target_scale.key = __Joint.crr_scale.key;
	__Joint.target_rotation.key = __Joint.crr_rotation.key;

	// recursively process children
	for (u16 i=0;i<root->mNumChildren;i++)
	{
		joints[__MemoryID].children[i] = joints.size();
		_rc_assemble_joint_hierarchy(joints,root->mChildren[i]);
	}
}

/**
 *	(called by AnimatedMesh::AnimatedMesh())
 *	acquire joint numerical id from joint list by it's joint string id
 *	\param joints: list of joints
 *	\param id: alphanumeric joint id as imported from file structure
 *	\returns numerical joint id as given by depthsearch layout assembly
 */
u16 _get_joint_id(const vector<MeshJoint>& joints,string id)
{
	u16 i = 0;
	while (id!=joints[i].id) i++;
	return i;
}
// FIXME investigate if there is a better solution without iteration find after all

/**
 *	load animation & mesh information from collada file
 *	\param path: path to .dae collada file
 */
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
	recursive_transforms.resize(__JointCount);
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
				f32 __ProcWeight = __Weights[__Weight.mVertexId][0];

				// iterate to fine least influential weight
				for (u8 k=1;k<RENDERER_ANIMATION_INFLUENCE_RANGE;k++)
				{
					if (__ProcWeight<=__Weights[__Weight.mVertexId][k]) continue;
					__ProcIndex = k;
					__ProcWeight = __Weights[__Weight.mVertexId][k];
				}

				// overwrite most insignificant weight if current weight is important enough
				if (__Weight.mWeight>__Weights[__Weight.mVertexId][__ProcIndex])
					__Weights[__Weight.mVertexId][__ProcIndex] = __Weight.mWeight;
			}
		}
	}

	// compose vertex data
	// assemble vertex array
	vector<AnimationVertex> __Vertices = vector<AnimationVertex>(__Mesh->mNumVertices);
	for (u64 i=0;i<__Mesh->mNumVertices;i++)
	{
		__Vertices[i] = {
			.position = to_vec3(__Mesh->mVertices[i]),
			.uv = to_vec2(__Mesh->mTextureCoords[0][i]),
			.normal = to_vec3(__Mesh->mNormals[i]),
			.tangent = to_vec3(__Mesh->mTangents[i]),
			.bone_index = vec4(__BoneIndices[i][0],__BoneIndices[i][1],
							   __BoneIndices[i][2],__BoneIndices[i][3]),
			.bone_weight = vec4(__Weights[i][0],__Weights[i][1],__Weights[i][2],__Weights[i][3])
		};
		// TODO be careful with the static 0 when expanding the loader
	}

	// using element array to store correct vertex order
	u16 __IndexCount = 0;
	for (u32 i=0;i<__Mesh->mNumFaces;i++) __IndexCount += __Mesh->mFaces[i].mNumIndices;
	vertices.reserve(__IndexCount);
	for (u32 i=0;i<__Mesh->mNumFaces;i++)
	{
		for (u32 j=0;j<__Mesh->mFaces[i].mNumIndices;j++)
			vertices.push_back(__Vertices[__Mesh->mFaces[i].mIndices[j]]);
	}

	// extract animations
	// allocate memory & iterate animations
	animations.resize(__File->mNumAnimations);
	for (u32 i=0;i<__File->mNumAnimations;i++)
	{
		aiAnimation* __Animation = __File->mAnimations[i];
		f64 __TPSinv = 1./__Animation->mTicksPerSecond;

		// register new animation
		animations[i] = {
			.joints = vector<AnimationJoint>(__Animation->mNumChannels),
			.duration = __Animation->mDuration*__TPSinv
		};
		animations[i].duration_inv = 1./animations[i].duration;

		// process animation channels
		for (u32 j=0;j<__Animation->mNumChannels;j++)
		{
			aiNodeAnim* __Node = __Animation->mChannels[j];
			AnimationJoint& __Joint = animations[i].joints[j];

			// process channel keys for related joint
			__Joint = {
				.id = _get_joint_id(joints,__Node->mNodeName.C_Str()),
				.position_keys = vector<AnimKey<vec3>>(__Node->mNumPositionKeys),
				.scaling_keys = vector<AnimKey<vec3>>(__Node->mNumScalingKeys),
				.rotation_keys = vector<AnimKey<quat>>(__Node->mNumRotationKeys)
			};
			// FIXME what happens to memory during this loop is truly gruesome

			// extract position keys
			for (u32 k=0;k<__Node->mNumPositionKeys;k++)
			{
				__Joint.position_keys[k] = {
					.key = to_vec3(__Node->mPositionKeys[k].mValue),
					.duration = __Node->mPositionKeys[k].mTime*__TPSinv
				};
			}

			// extract scaling keys
			for (u32 k=0;k<__Node->mNumScalingKeys;k++)
			{
				__Joint.scaling_keys[k] = {
					.key = to_vec3(__Node->mScalingKeys[k].mValue),
					.duration = __Node->mScalingKeys[k].mTime*__TPSinv
				};
			}

			// extract rotation keys
			for (u32 k=0;k<__Node->mNumRotationKeys;k++)
			{
				__Joint.rotation_keys[k] = {
					.key = to_quat(__Node->mRotationKeys[k].mValue),
					.duration = __Node->mRotationKeys[k].mTime*__TPSinv
				};
			}
		}
	}
}

/**
 *	define standard animation loop
 *	\param id: animation id
 *	\param tt: transition time in seconds
 */
void AnimatedMesh::set_default_animation(u8 id,f32 tt)
{
	m_StandardAnimation = id;
	m_StandardTransitionTime = tt;
}

/**
 *	switch active animation by id
 *	\param id: animation id
 *	\param tt: transition time in seconds
 */
void AnimatedMesh::set_animation(u8 id,f32 tt)
{
	current_animation = id;
	m_AnimationTransitionTime = tt;
	m_Progress = .0;
}

/**
 *	(called by AnimatedMesh::find_joint)
 *	this recursively iterates flat mesh subtree of joints
 *	\param joints: full flat tree of mesh joints
 *	\param tr: index of root in joint subtree in process
 *	\param id: string id of joint as described in armature editor
 *	\returns pointer to found joint, else nullptr
 */
MeshJoint* _rc_find_joint(vector<MeshJoint>& joints,u16 tr,const string& id)
{
	if (joints[tr].id==id) return &joints[tr];
	for (u16 i : joints[tr].children)
	{
		MeshJoint* p_Result = _rc_find_joint(joints,i,id);
		if (p_Result) return p_Result;
	}
	return nullptr;
}

/**
 *	extract joint from flat mesh joint tree
 *	\param id: string id of joint as described in armature editor
 *	\returns address of joint with given id, else nullptr
 */
MeshJoint* AnimatedMesh::find_joint(const string& id)
{
	return _rc_find_joint(joints,0,id);
}

/**
 *	acquire progress of current animation
 *	\returns animation progress between 0 and 1
 */
f64 AnimatedMesh::get_progress()
{
	return m_Progress*animations[current_animation].duration_inv;
}

/**
 *	(called by AnimatedMesh::animate()) advances through animation keys until current is found
 *	\param keys: vector list of anim keys, iterated to find the current one
 *	\param progress: current progress of animation
 *	\returns current target animation key with adjusted key duration based on actual-time progress
 */
template<typename T> static inline AnimKey<T> _advance_keys(const vector<AnimKey<T>>& keys,f64 progress)
{
	u16 __Crr = 0;
	while (keys[__Crr+1].duration<progress) __Crr++;
	AnimKey<T> __Out = keys[wrap_next(__Crr,keys.size())];
	__Out.duration -= (progress-g_Frame.delta_time);
	return __Out;
}

/**
 *	update active animation
 */
void AnimatedMesh::animate()
{
	Animation& p_Animation = animations[current_animation];

	// interpolation delta & restore default animation after playback has finished
	m_Progress += g_Frame.delta_time;
	if (m_Progress>p_Animation.duration)
	{
		m_Progress -= p_Animation.duration;
		set_animation(m_StandardAnimation,m_StandardTransitionTime);
	}
	// TODO exchange rigid control structure with a usable one, then maybe change back to fmod solution

	// iterate joints for location animation transformations
	for (AnimationJoint& p_Joint : p_Animation.joints)
	{
		MeshJoint& p_MJoint = joints[p_Joint.id];

		// determine transformation keyframes
		AnimKey<vec3> __TargetPosition = _advance_keys(p_Joint.position_keys,m_Progress);
		AnimKey<vec3> __TargetScale = _advance_keys(p_Joint.scaling_keys,m_Progress);
		AnimKey<quat> __TargetRotation = _advance_keys(p_Joint.rotation_keys,m_Progress);

		// duration padding for transitions
		__TargetPosition.duration += m_AnimationTransitionTime;
		__TargetScale.duration += m_AnimationTransitionTime;
		__TargetRotation.duration += m_AnimationTransitionTime;

		// set joint transformation targets
		p_MJoint.request_position(__TargetPosition);
		p_MJoint.request_scale(__TargetScale);
		p_MJoint.request_rotation(__TargetRotation);
	}
	m_AnimationTransitionTime = glm::max(m_AnimationTransitionTime-g_Frame.delta_time,.0f);
}

/**
 *	update joint transform based on current target & progression
 */
void AnimatedMesh::update()
{
	// iterate joints for location animation transformations
	for (MeshJoint& p_Joint : joints) p_Joint.interpolate();

	// calculate transform after parent influence
	mat4 __Parent = mat4(1.f);
	_rc_transform_interpolation(joints[0],__Parent);
}
// TODO make it possible for animations to transform themselves around a selected joint or a root transform
// FIXME the transitions somehow are way to fast

/**
 *	recursively transform joint tree based on each parent (funamentals of forward kinematics)
 *	\param joint: joint of current root in animation joint subtree
 *	\param parent_transform: transformation basis inherited from parent joint
 */
void AnimatedMesh::_rc_transform_interpolation(MeshJoint& joint,mat4& parent_transform)
{
	joint.local_transform = parent_transform*joint.transform;
	recursive_transforms[joint.uniform_location] = joint.local_transform*joint.offset;
	for (u16 child : joint.children) _rc_transform_interpolation(joints[child],joint.local_transform);
}


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer Object

/**
 *	creation of uniform buffer object abstraction
 *	\param shader: shader pipeline, which carries the endpoint description of the draw the ubo will be bound on
 */
void UniformBufferObject::create(lptr<ShaderPipeline> shader)
{
	shader->generate_ubo(dsets);
	uniform.shader = shader;
}

/**
 *	bind ubo for draw. shader pipeline, that was passed at creation or with correlating endpoint must be bound.
 *	the latter can be helpful for alternate shader processing operations (e.g. shadows, silhouette shaders)
 */
void UniformBufferObject::bind()
{
	g_Renderer.m_GlobalDescriptorSet.bind(uniform.shader->pipeline_layout);
	for (DescriptorSetMemory& p_DSet : dsets) p_DSet.bind(uniform.shader->pipeline_layout);
}

/**
 *	update ubo before draw
 */
void UniformBufferObject::update()
{
	for (DescriptorSetMemory& p_DSet : dsets)
		p_DSet.update_frame();
}
// TODO implement distiction between constantly updating descriptor set & sparsely updating descriptor set
//		former will use update_frame(), latter should use update()

/**
 *	update ubo before draw.
 *	this automatically updates all registered memory by attach_memory(), before calling the actual ubo update
 */
void UniformBufferObject::auto_update()
{
	uniform.upload();
	update();
}

/**
 *	register a memory segment by pointer. this will be the correlation to a uniform value in shader at the
 *	given location.
 *	set is not required, due to the predefined channel purposes. for data that means set = 1 in shader.
 *	\param location: location of the target variable in shader 
 *	\param data: pointer to correlating host memory
 */
void UniformBufferObject::attach_memory(u16 location,void* data)
{
	uniform.attach_memory_segment(&dsets[UBO_CHANNEL_PURPOSE_DATA],location,data);
}

/**
 *	register a texture as accessible by a texture variable at the given location.
 *	set is not required, for textures set = 2 in shader, here it is set = 1. see channel purposes & global dset
 *	\param location: location of the target texture reference variable in shader
 *	\param texture: pointer to registered or preloaded texture
 */
void UniformBufferObject::attach_texture(u16 location,Texture* texture)
{
	TextureAttachment __Attachment = {
		.ubo = &dsets[UBO_CHANNEL_PURPOSE_TEXTURES],
		.texture = texture,
		.location = location
	};
	g_Renderer.m_TextureAttachmentRequests.push_back(__Attachment);
}

/**
 *	attach a material. this utilizes predefined pbs material channels to represent all necessary components.
 *	see MaterialComponentChannel to find out about pbs material target channels
 *	\param material: pointer to material
 */
void UniformBufferObject::attach_material(Material* material)
{
	for (u8 i=0;i<MATERIAL_COMPONENT_CHANNEL_COUNT;i++)
		attach_texture(i,material->components[i]);
}


// ----------------------------------------------------------------------------------------------------
// Geometry Batching

/**
 *	construct & preallocate handles for geometry batch
 *	\param sp: shader pipeline for batch draw
 *	\param obj_capacity: amount of allowed geometry to register
 */
void GeometryBatch::assemble(lptr<ShaderPipeline> sp,size_t obj_capacity)
{
	capacity = obj_capacity;
	objects = (GeometryTuple*)malloc(sizeof(GeometryTuple)*capacity);
	for (size_t i=0;i<capacity;i++) new (&objects[i]) GeometryTuple();
	shader = sp;
}
// TODO consider an object heap for a single allocation, then reference memory through pointers in batch

/**
 *	add mesh geometry to batch
 *	\param mesh: loaded mesh for explicit geometry information
 *	\returns pointer to geometry tuple
 */
GeometryTuple* GeometryBatch::add_geometry(Mesh& mesh)
{
	return add_geometry(&mesh.vertices[0],mesh.vertices.size(),sizeof(Vertex));
}

/**
 *	add animated mesh geometry to batch
 *	\param mesh: animated mesh for explicit geometry information
 *	\returns pointer to geometry tuple
 */
GeometryTuple* GeometryBatch::add_geometry(AnimatedMesh& mesh)
{
	GeometryTuple* p_Tuple = add_geometry(&mesh.vertices[0],mesh.vertices.size(),sizeof(AnimationVertex));
	p_Tuple->ubo.attach_memory(RENDERER_JOINT_ANIMATION_LOCATION,&mesh.recursive_transforms[0]);
	anim_meshes.push_back(&mesh);
	return p_Tuple;
}

/**
 *	load geometry into batch
 *	\param verts: single precision floats, explicitly defining geometry
 *	\param vsize: amount of vertices (this is the pointer length divided by the upload dimension)
 *	\param ssize: upload dimension in bytesize
 *	\returns pointer to geometry tuple
 */
GeometryTuple* GeometryBatch::add_geometry(void* verts,size_t vsize,size_t ssize)
{
	COMM_LOG("uploading geometry to batch");
	size_t __MemSize = vsize*ssize;
	size_t __Size = __MemSize/sizeof(f32);

	// write
	geometry.resize(geometry_cursor+__Size);
	memcpy(&geometry[geometry_cursor],verts,__MemSize);

	// store geometry information
	COMM_ERR_COND(count>=capacity,"geometry addition violates maximum geometry capacity");
	GeometryTuple* p_Tuple = &objects[count++];
	p_Tuple->offset = offset_cursor;
	p_Tuple->vertex_count = vsize;
	p_Tuple->transform.model = &p_Tuple->object_uniform_data.model;
	p_Tuple->ubo.create(shader);
	p_Tuple->ubo.attach_memory(RENDERER_GEOMETRY_INFO_LOCATION,&p_Tuple->object_uniform_data);
	shader->generate_pcm(p_Tuple->pcm);

	// cursor
	offset_cursor += vsize;
	geometry_cursor += __Size;
	return p_Tuple;
}

/**
 *	upload batch geometry to gpu & automap shader pipeline
 */
void GeometryBatch::load()
{
	COMM_LOG("uploading geometry information to GPU");
	size_t __Size = geometry.size()*sizeof(f32);
	vbo.allocate(__Size,VERTEX_BUFFER_TYPE_STATIC);
	vbo.upload(&geometry[0],__Size);
	vao.register_buffer(vbo);
}

/**
 *	update batch uniform buffer
 */
void GeometryBatch::update()
{
	for (size_t i=0;i<count;i++)
		objects[i].ubo.auto_update();
}

/**
 *	draw registered batch geometry
 */
void GeometryBatch::render()
{
	vao.bind();
	shader->enable();
	for (size_t i=0;i<count;i++)
	{
		GeometryTuple& p_Tuple = objects[i];
		p_Tuple.ubo.bind();
		shader->upload_pcm(p_Tuple.pcm);
		vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,p_Tuple.vertex_count,1,p_Tuple.offset,0);
	}
	// TODO map<u32,void*> can also describe memory uploads pretty independently
}

/**
 *	clean batch memory & destroy buffers
 */
void GeometryBatch::vanish()
{
	for (size_t i=0;i<capacity;i++) objects[i].~GeometryTuple();
	free(objects);
	vbo.free();
}


// ----------------------------------------------------------------------------------------------------
// Particle Batching

/**
 *	setup particle batch by mesh geometry
 *	\param mesh: loaded mesh for explicit geometry information
 *	\param particles: amount of particles
 *	\param isize: upload dimension of index buffer !in memory width!
 */
void ParticleBatch::load(Mesh& mesh,u32 particles,size_t isize)
{
	load(&mesh.vertices[0],mesh.vertices.size(),sizeof(Vertex),particles,isize);
}

/**
 *	load particle mesh into batch memory
 *	\param verts: single precision floats, explicitly defining geometry
 *	\param vsize: amount of vertices (this is the pointer length divided by the upload dimension)
 *	\param ssize: upload dimension !in memory width!
 *	\param particles: amount of particles
 *	\param isize: upload dimension of index buffer !in memory width!
 */
void ParticleBatch::load(void* verts,size_t vsize,size_t ssize,u32 particles,size_t isize)
{
	COMM_LOG("loading particle mesh geometry information");
	size_t __Size = vsize*ssize;

	// upload geometry & acquire instance memory segment
	vbo.allocate(__Size,VERTEX_BUFFER_TYPE_STATIC);
	vbo.upload(verts,__Size);
	imem = ibo.allocate(particles*isize,VERTEX_BUFFER_TYPE_DYNAMIC);

	// vertex array
	vao.register_buffer(vbo);
	vao.register_buffer_dynamic(ibo);

	// uniform buffer & push constants
	ubo.create(shader);
	shader->generate_pcm(pcm);

	// store geometry information
	ibo_data_size = particles*isize;
	vertex_count = vsize;
	active_particles = particles;
}

/**
 *	update batch uniform buffer
 */
void ParticleBatch::update_uniform()
{
	ubo.auto_update();
}

/**
 *	update entire instance data buffer
 */
void ParticleBatch::update_instances()
{
	ibo.register_upload(ibo_data_size);
}

/**
 *	draw all registered batch geometry duplicates
 */
void ParticleBatch::render()
{
	vao.bind();
	shader->enable();
	ubo.bind();
	shader->upload_pcm(pcm);
	vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,vertex_count,active_particles,0,0);
}

/**
 *	clean batch memory & destroy buffers
 *	\note should not be called during final closing operations, calling free only adjusts vram heap segments
 */
void ParticleBatch::vanish()
{
	vbo.free();
	ibo.free();
}


// ----------------------------------------------------------------------------------------------------
// Shadow Batches

/**
 *	draw geometry with alternate shadow shader for shadow projections
 */
void ShadowGeometryBatch::render()
{
	batch->vao.bind();
	shader->enable();
	for (size_t i=0;i<batch->count;i++)
	{
		GeometryTuple& p_Tuple = batch->objects[i];
		p_Tuple.ubo.bind();
		shader->upload_pcm(p_Tuple.pcm);
		vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,p_Tuple.vertex_count,1,p_Tuple.offset,0);
	}
}

/**
 *	draw particles with alternate shadow shader for shadow projections
 */
void ShadowParticleBatch::render()
{
	batch->vao.bind();
	shader->enable();
	batch->ubo.bind();
	shader->upload_pcm(batch->pcm);
	vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,batch->vertex_count,batch->active_particles,0,0);
}


// ----------------------------------------------------------------------------------------------------
// Render Unit

#ifdef VKBUILD

/**
 *	setup render system
 */
Renderer::Renderer()
{
	COMM_MSG(LOG_CYAN,"starting render system");
	COMM_LOG("starting font rasterizer");
	bool _failed = FT_Init_FreeType(&g_FreetypeLibrary);
	COMM_ERR_COND(_failed,"text rasterizer not available");

	// primitive sprite & canvas data
	f32 __QuadVertices[] = {
		-.5f,-.5f,.0f,1.f, .5f,.5f,1.f,.0f, .5f,-.5f,1.f,1.f,
		.5f,.5f,1.f,.0f, -.5f,-.5f,.0f,1.f, -.5f,.5f,.0f,.0f
	};
	f32 __FBVertices[] = {
		-1.f,-1.f,.0f,.0f, 1.f,1.f,1.f,1.f, 1.f,-1.f,1.f,.0f,
		1.f,1.f,1.f,1.f, -1.f,-1.f,.0f,.0f, -1.f,1.f,.0f,1.f
	};

	// SPRITES
	// sprite vertex data
	m_SpriteVertexBuffer.allocate(sizeof(__QuadVertices),VERTEX_BUFFER_TYPE_STATIC);
	m_SpriteVertexBuffer.upload(__QuadVertices,sizeof(__QuadVertices));
	m_Sprites.mem = (Sprite*)m_SpriteInstanceBuffer.allocate(
			RENDERER_MAXIMUM_SPRITE_COUNT*sizeof(Sprite),VERTEX_BUFFER_TYPE_DYNAMIC);

	// sprite vertex array
	m_SpriteVertexArray.register_buffer(m_SpriteVertexBuffer);
	m_SpriteVertexArray.register_buffer_dynamic(m_SpriteInstanceBuffer);

	// TEXT
	// text vertex data
	Text::characters.mem = (TextCharacter*)m_TextInstanceBuffer.allocate(
			RENDERER_MAXIMUM_CHARACTER_COUNT*sizeof(TextCharacter),VERTEX_BUFFER_TYPE_DYNAMIC);

	// text vertex array
	m_TextVertexArray.register_buffer(m_SpriteVertexBuffer);
	m_TextVertexArray.register_buffer_dynamic(m_TextInstanceBuffer);

	// SCENE FRAMEBUFFER
	// framebuffer vertex data
	m_TargetVertexBuffer.allocate(sizeof(__FBVertices),VERTEX_BUFFER_TYPE_STATIC);
	m_TargetVertexBuffer.upload(__FBVertices,sizeof(__FBVertices));

	// framebuffer vertex array
	m_TargetVertexArray.register_buffer(m_TargetVertexBuffer);

	// RESOURCE SETUP
	// textures
	m_GPUSpriteTextures.allocate(RENDERER_SPRITE_MEMORY_WIDTH,RENDERER_SPRITE_MEMORY_HEIGHT,
								 TEXTURE_FORMAT_SRGB,TEXTURE_FEATURE_SUPPRESS_MIPMAP_BIT,ATLAS_SPRITES_PADDING);
	m_GPUFontTextures.allocate(RENDERER_FONT_MEMORY_WIDTH,RENDERER_FONT_MEMORY_HEIGHT,
							   TEXTURE_FORMAT_MONOCHROME,TEXTURE_FEATURE_SUPPRESS_MIPMAP_BIT,
							   ATLAS_FONT_PADDING);

	// SHADERS
	// setup shader endpoint
	g_UniformBuffer.define(0,GSE_CAMERA_TRANSFORM_LOCATION,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT);
	g_UniformBuffer.define(0,GSE_CSYSTEM_TRANSFORM_LOCATION,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT);
	for (u16 i=GSE_SPRITE_ATLAS_LOCATION;i<GSE_SHADOW_MAP_LOCATION+1;i++)
		g_UniformBuffer.define(0,i,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT);
	g_UniformBuffer.define(0,GSE_CAMERA_ATTRIBUTES_LOCATION,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_FRAGMENT_BIT);
	g_UniformBuffer.define(0,GSE_LIGHTING_LOCATION,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_FRAGMENT_BIT);
	g_UniformBuffer.define(0,GSE_SHADOW_LOCATION,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						   VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT);
	g_UniformBuffer.define(1,RENDERER_GEOMETRY_INFO_LOCATION,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT);
	g_UniformBuffer.define(1,RENDERER_JOINT_ANIMATION_LOCATION,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT);
	for (u16 i=0;i<RENDERER_TEXTURE_DESCRIPTOR_CHANNELS;i++)
		g_UniformBuffer.define(2,i,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT);
	g_UniformBuffer.define(1,71,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT);  // voxelinfo
	g_UniformBuffer.generate_layouts();
	// TODO define this in an extern file & routinize

	// rendertargets
	Framebuffer::setup_result(&RENDER_STAGES[RENDER_STAGE_RESULT_ASSEMBLY],m_ResultBuffers);
	m_Framebuffers[RENDER_STAGE_FORWARD_DRAW].setup(TARGET_RESOLUTION_FULLSCREEN,
													&RENDER_STAGES[RENDER_STAGE_FORWARD_DRAW]);
	m_Framebuffers[RENDER_STAGE_GEOMETRY_PASS].setup(TARGET_RESOLUTION_FULLSCREEN,
													 &RENDER_STAGES[RENDER_STAGE_GEOMETRY_PASS]);
	m_Framebuffers[RENDER_STAGE_SHADOW_PROJECTION].setup(TARGET_RESOLUTION_SHADOWPROJ,
														 &RENDER_STAGES[RENDER_STAGE_SHADOW_PROJECTION]);

	// setup default pipelines
	m_SpritePipeline = register_pipeline(
			"./shader/vulkan/bin/sprite.vert","./shader/vulkan/bin/sprite.frag",
			TARGET_RESOLUTION_RESULT,RENDER_STAGE_RESULT_ASSEMBLY,SPF_FLIPPED|SPF_BLENDING);
	m_TextPipeline = register_pipeline("./shader/vulkan/bin/text.vert","./shader/vulkan/bin/text.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_RESULT_ASSEMBLY,SPF_FLIPPED|SPF_BLENDING);
	m_TargetPipeline = register_pipeline("./shader/vulkan/bin/pbs.vert","./shader/vulkan/bin/pbs.frag",
			TARGET_RESOLUTION_RESULT,RENDER_STAGE_RESULT_ASSEMBLY,SPF_DEFAULT);
	// FIXME target depth recording should be unnecessary

	// pipelines, setup for deferred scene processing
	m_GeometryPassPipeline = register_pipeline(
			"./shader/vulkan/bin/gpass.vert","./shader/vulkan/bin/gpass.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_GEOMETRY_PASS,SPF_DEFAULT
		);
	m_ShadowPassPipeline = register_pipeline(
			"./shader/vulkan/bin/shadow.vert","./shader/vulkan/bin/shadow.frag",
			TARGET_RESOLUTION_SHADOWPROJ,RENDER_STAGE_SHADOW_PROJECTION,SPF_FLIPPED
		);

	// upload 2D coordinate system
	m_UBufferMem.strafo.view = g_CoordinateSystem.view;
	m_UBufferMem.strafo.proj = g_CoordinateSystem.proj;

	// setup global ubo data for slot 0
	m_GlobalSetOffset = g_UniformBuffer.mcr.acquire_memory_segment_aligned(sizeof(UniformBufferMemory));
	m_GlobalDescriptorSet.allocate(0,16);
	m_GlobalDescriptorSet.define_data_segment(GSE_CAMERA_TRANSFORM_LOCATION,
											  m_GlobalSetOffset+offsetof(UniformBufferMemory,otrafo),
											  sizeof(ObjectTransformation));
	m_GlobalDescriptorSet.define_data_segment(GSE_CSYSTEM_TRANSFORM_LOCATION,
											  m_GlobalSetOffset+offsetof(UniformBufferMemory,strafo),
											  sizeof(SpriteTransformation));
	m_GlobalDescriptorSet.define_data_segment(GSE_CAMERA_ATTRIBUTES_LOCATION,
											  m_GlobalSetOffset+offsetof(UniformBufferMemory,camera),
											  sizeof(CameraAttributes));
	m_GlobalDescriptorSet.define_data_segment(GSE_LIGHTING_LOCATION,
											  m_GlobalSetOffset+offsetof(UniformBufferMemory,lighting),
											  sizeof(Lighting));
	m_GlobalDescriptorSet.define_data_segment(GSE_SHADOW_LOCATION,
											  m_GlobalSetOffset+offsetof(UniformBufferMemory,orth_shadow),
											  sizeof(OrthographicShadow));
	m_GlobalDescriptorSet.define_texture_segment(GSE_SPRITE_ATLAS_LOCATION,
												 GSE_SHADOW_MAP_LOCATION-GSE_SPRITE_ATLAS_LOCATION+1,
												 TEXTURE_ATTRIBUTE_TYPE_TEXTURE);

	// linking buffer results
	m_GlobalDescriptorSet.link_result(GSE_SPRITE_ATLAS_LOCATION,&m_GPUSpriteTextures.atlas);
	m_GlobalDescriptorSet.link_result(GSE_FONT_ATLAS_LOCATION,&m_GPUFontTextures.atlas);
	m_GlobalDescriptorSet.link_result(GSE_FORWARD_COLOUR_LOCATION,
			m_Framebuffers[RENDER_STAGE_FORWARD_DRAW].components[FORWARD_BUFFER_CHANNEL_COLOUR]);
	m_GlobalDescriptorSet.link_result(GSE_FORWARD_DEPTH_LOCATION,
			m_Framebuffers[RENDER_STAGE_FORWARD_DRAW].components[FORWARD_BUFFER_CHANNEL_DEPTH]);
	for (u16 i=0;i<DEFERRED_BUFFER_CHANNEL_COUNT;i++)
	{
		m_GlobalDescriptorSet.link_result(GSE_GBUFFER_COLOUR_LOCATION+i,
										  m_Framebuffers[RENDER_STAGE_GEOMETRY_PASS].components[i]);
	}
	m_GlobalDescriptorSet.link_result(GSE_SHADOW_MAP_LOCATION,
									  m_Framebuffers[RENDER_STAGE_SHADOW_PROJECTION].components[0]);

	// load ubo
	m_SpriteUBO.create(m_SpritePipeline);
	m_TextUBO.create(m_TextPipeline);

	/*
	COMM_LOG("starting renderer subprocesses");
	_sprite_signal.stall();
	m_SpriteCollector = thread(Renderer::_collector<Sprite>,&m_Sprites,&_sprite_signal);
	m_SpriteCollector.detach();
	_sprite_texture_signal.stall();
	m_SpriteTextureCollector = thread(Renderer::_collector<PixelBufferComponent>,
									  &m_GPUSpriteTextures.textures,&_sprite_texture_signal);
	m_SpriteTextureCollector.detach();
	*/

	COMM_SCC("render system ready.");
}
// TODO join collector processes when exiting renderer, or maybe just let the os handle that and not care?

/**
 *	precalculating setup (before wheel system update)
 */
void Renderer::precalculate()
{
	for (AnimatedMesh* p_Mesh : m_AnimatingMeshes) p_Mesh->animate();
	for (GeometryBatch& p_Batch : m_GeometryBatches)
	{
		for (AnimatedMesh* p_Mesh : p_Batch.anim_meshes)
			p_Mesh->update();
	}
	for (GeometryBatch& p_Batch : m_DeferredGeometryBatches)
	{
		for (AnimatedMesh* p_Mesh : p_Batch.anim_meshes)
			p_Mesh->update();
	}
}

/**
 *	draw geometry for shadow projection for both batch types
 *	\param gb: registered geometry batches for this purpose
 *	\param pb: registered particle batches for this purpose
 */
inline void _update_shadow(list<ShadowGeometryBatch>& gb,list<ShadowParticleBatch>& pb)
{
	for (ShadowGeometryBatch& p_GB : gb) p_GB.render();
	for (ShadowParticleBatch& p_PB : pb) p_PB.render();
}

/**
 *	draw geometry for both batch types
 *	\param gb: registered geometry batches for this purpose
 *	\param pb: registered particle batches for this purpose
 */
inline void _update_geometry(list<GeometryBatch>& gb,list<ParticleBatch>& pb)
{
	for (GeometryBatch& p_GB : gb) p_GB.render();
	for (ParticleBatch& p_PB : pb) p_PB.render();
}

/**
 *	update render system, gpu data & execute draw
 */
void Renderer::update()
{
	_gpu_upload();
	_update_uniforms();

	// shadow pass
	m_Framebuffers[RENDER_STAGE_SHADOW_PROJECTION].record();
	_update_shadow(m_ShadowGeometryBatches,m_ShadowParticleBatches);
	m_Framebuffers[RENDER_STAGE_SHADOW_PROJECTION].stop();

	// record scene, deferred component
	m_Framebuffers[RENDER_STAGE_GEOMETRY_PASS].record();
	_update_geometry(m_DeferredGeometryBatches,m_DeferredParticleBatches);
	m_Framebuffers[RENDER_STAGE_GEOMETRY_PASS].stop();

	// record scene, forward component
	m_Framebuffers[RENDER_STAGE_FORWARD_DRAW].record();
	_update_geometry(m_GeometryBatches,m_ParticleBatches);
	m_Framebuffers[RENDER_STAGE_FORWARD_DRAW].stop();

	// transition depth results
	Texture::transition_depth_texture(
			m_Framebuffers[RENDER_STAGE_FORWARD_DRAW].attachment_images[FORWARD_BUFFER_CHANNEL_DEPTH]);
	Texture::transition_depth_texture(
			m_Framebuffers[RENDER_STAGE_GEOMETRY_PASS].attachment_images[DEFERRED_BUFFER_CHANNEL_DEPTH]);
	Texture::transition_depth_texture(m_Framebuffers[RENDER_STAGE_SHADOW_PROJECTION].attachment_images[0]);

	// result assembly
	m_ResultBuffers[g_Frame.frame_id].record();
	_update_target();
	_update_sprites();
	_update_text();
	m_ResultBuffers[g_Frame.frame_id].stop();
}

/**
 *	exit renderer and end all it's subprocesses, also cleanup all resources
 */
void Renderer::vanish()
{
#ifdef VKBUILD
	// clean pipelines
	for (ShaderPipeline& p_ShaderPipeline : m_ShaderPipelines) p_ShaderPipeline.vanish();
	for (GeometryBatch& p_Batch : m_GeometryBatches) p_Batch.vanish();
	for (GeometryBatch& p_Batch : m_DeferredGeometryBatches) p_Batch.vanish();

	// clean framebuffers
	for (Framebuffer& p_ResultBuffer : m_ResultBuffers) p_ResultBuffer.vanish();
	for (u8 i=0;i<RENDER_STAGE_COUNT-1;i++) m_Framebuffers[i].vanish();
	for (u8 i=0;i<RENDER_STAGE_COUNT;i++) g_GPU.free(RENDER_STAGES[i].render_pass);

	// clear pixel buffers
	m_GPUSpriteTextures.atlas.vanish();
	m_GPUFontTextures.atlas.vanish();

	// clear registered textures
	for (size_t i=0;i<m_MeshTextures.active_range;i++)
	{
		if (!m_MeshTextures.mem[i].allocated) continue;
		m_MeshTextures.mem[i].vanish();
	}

	// clear registered cubemaps
	for (size_t i=0;i<m_Cubemaps.active_range;i++)
	{
		if (!m_Cubemaps.mem[i].cubemap.allocated) continue;
		m_Cubemaps.mem[i].vanish();
	}

	// clear vbo & ubo buffer memory
	g_VertexBufferHeap[VERTEX_BUFFER_TYPE_STATIC].vanish();
	g_VertexBufferHeap[VERTEX_BUFFER_TYPE_DYNAMIC].vanish();
	g_UniformBuffer.vanish();
#endif

	/*
	_sprite_texture_signal.exit();
	_sprite_signal.exit();
	*/
}

/**
 *	register sprite texture to load and move to sprite pixel buffer
 *	\param path: path to texture file
 *	\returns pointer to texture component info to assign to a sprite later
 */
Rect* Renderer::register_sprite_texture(const char* path)
{
	Rect* p_Comp = &m_GPUSpriteTextures.textures.mem[m_GPUSpriteTextures.textures.next_free()];
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
Sprite* Renderer::register_sprite(Rect* texture,vec3 position,vec2 size,f32 rotation,f32 alpha,
								  Alignment alignment)
{
	Sprite* p_Sprite = &m_Sprites.mem[m_Sprites.next_free()];

	// align sprite into borders
	if (alignment.alignment!=SCREEN_ALIGN_NEUTRAL)
	{
		vec2 hsize = size*.5f;
		vec2 __AlignedPosition = alignment.align({ vec2(position)-hsize,size })+size;
		position.x = __AlignedPosition.x;
		position.y = __AlignedPosition.y;
	}

	COMM_LOG("sprite register at: (%f,%f), %fx%f, %f° -> count = %lu",
			 position.x,position.y,size.x,size.y,rotation,m_Sprites.active_range);

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
void Renderer::assign_sprite_texture(Sprite* sprite,Rect* texture)
{
	m_GPUSpriteTextures.signal.wait();
	sprite->pbc = *texture;
}

/**
 *	remove given sprite texture and free memory in array as well as releasing memory space on atlas
 *	\param texture: pointer to texture, which shall be removed
 */
void Renderer::delete_sprite_texture(Rect* texture)
{
	// signal cleanup
	texture->position.x = RENDERER_POSITIONAL_DELETION_CODE;
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
	COMM_AWT("register font from source %s",path);
	Font* p_Font = &m_Fonts.mem[m_Fonts.next_free()];
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
	COMM_ERR_COND(position.z>=10||position.z<=0,"text positioning is out of orthographic clipping range");
	// FIXME inaccurate guard, after projection even 1 is clipped, while 7 works no problem?
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
 *	remove registered text, which means free attached characters from global character presentation memory
 *	\param text: text entity, as passed at creation
 */
void Renderer::delete_text(lptr<Text> text)
{
	text->vanish();
	m_Texts.erase(text);
}

/**
 *	register colour as texture for later attachment to meshes or particles, or for usage in material composition
 *	this will create a subthread, that generates colour texture parallel to further program execution
 *	\param colour: colour vector, defining texture data
 *	\param format: (default TEXTURE_FORMAT_RGBA) texture colourspace format
 *	\returns pointer to generated texture in ram, referencing generated colour texture in vram
 */
Texture* Renderer::register_texture(vec3 colour,TextureFormat format)
{
	COMM_LOG("mesh texture register colour v(%f,%f,%f)",colour.x,colour.y,colour.z);
	COMM_ERR_COND(format==TEXTURE_FORMAT_MONOCHROME,
				  "monochrome format should not be set for textures, defined by a colour vector");
	Texture* p_Texture = &m_MeshTextures.mem[m_MeshTextures.next_free()];
	new(p_Texture) Texture();
	thread __LoadThread(Texture::load_colour,p_Texture,colour,format,&m_MeshTextureUploadQueue);
	__LoadThread.detach();
	return p_Texture;
}

/**
 *	register texture & load texture in a subthread, parallel to further program execution
 *	\param path: path to texture file
 *	\param format: (default TEXTURE_FORMAT_RGBA) texture colourspace format
 *	\param feat: (default TEXTURE_FEATURE_STANDARD) bitwise texture features
 *	\returns pointer to texture in ram, referencing texture in vram
 */
Texture* Renderer::register_texture(const char* path,TextureFormat format,TextureFeatures feat)
{
	COMM_LOG("mesh texture register of %s",path);
	Texture* p_Texture = &m_MeshTextures.mem[m_MeshTextures.next_free()];
	new(p_Texture) Texture();
	thread __LoadThread(Texture::load_texture,p_Texture,path,format,feat,&m_MeshTextureUploadQueue);
	__LoadThread.detach();
	return p_Texture;
}
// TODO default texture format should be srgb, not linear rgb values.
//		probably even though the batched texture upload will mostly use linear rgb for material formats

/**
 *	register material texture composition from multiple textures
 *	\param albedo: the only required component, surface colour information
 *	\param normals: (default nullptr) surface normal manipulation, default results in colour vec3(.5,.5,1).
 *			no manipulation of surface normal vectors
 *	\param materials: (default nullptr) surface material definition for physical based shading, default results
 *			in colour vec3(0,1,1). no metallic, full diffuse, full ao multiplication (no ambient occlusion)
 *	\param emission: (default nullptr) surface emission map, for self irradiance without outside lighting.
 *			default results in colour vec3(0,0,0), no self emission of light by the surface.
 *	\returns pointer to registered material for mapping onto a object per ubo
 */
Material* Renderer::register_material(Texture* albedo,Texture* normals,Texture* materials,Texture* emission)
{
	COMM_ERR_COND(albedo==nullptr,"albedo texture must be provided in material creation");
	Material* p_Material = &m_Materials.mem[m_Materials.next_free()];
	p_Material->components[MATERIAL_COMPONENT_CHANNEL_ALBEDO] = albedo;
	p_Material->components[MATERIAL_COMPONENT_CHANNEL_NORMAL]
			= (normals!=nullptr) ? normals : Material::standards[MATERIAL_COMPONENT_CHANNEL_NORMAL];
	p_Material->components[MATERIAL_COMPONENT_CHANNEL_MATERIAL]
			= (materials!=nullptr) ? materials : Material::standards[MATERIAL_COMPONENT_CHANNEL_MATERIAL];
	p_Material->components[MATERIAL_COMPONENT_CHANNEL_EMISSION]
			= (emission!=nullptr) ? emission : Material::standards[MATERIAL_COMPONENT_CHANNEL_EMISSION];
	return p_Material;
}

/**
 *	classical cubemap. directory is the source, will load a texture for each side of the skybox
 *	\param path: path to cubemap directory. this should contain posx until negz textures
 *	\param format: texture colourspace format
 *	\returns pointer to registered cubemap for mapping onto a skybox per samplerCube
 */
Cubemap* Renderer::register_cubemap_classical(const char* path,TextureFormat format)
{
	Cubemap* p_Cubemap = &m_Cubemaps.mem[m_Cubemaps.next_free()];
	new(p_Cubemap) Cubemap();
	thread __LoadThread(Cubemap::load,p_Cubemap,path,format,&m_CubemapUploadQueue);
	__LoadThread.detach();
	return p_Cubemap;
}

/**
 *	single texture. this texture can be projected by cubemap_hdr.frag onto a skybox. contains irradiance data
 *	\param path: path to single .hdr texture
 *	\note this requires an .hdr irradiance map, will be packed into 4-byte unsigned float format
 *	\returns pointer to registered cubemap for mapping onto a skybox by cubemap_hdr.frag shader
 */
Texture* Renderer::register_cubemap_hdr(const char* path)
{
	return register_texture(path,TEXTURE_FORMAT_RGBu11,TEXTURE_FEATURE_SUPPRESS_MIPMAP_BIT);
}

/**
 *	register shader pipeline
 *	\param vs: path to vertex shader
 *	\param fs: path to fragment shader
 *	\param tr: resolution of result
 *	\param rs: render stage, the new pipeline will operate within
 *	\param feat: enabled/active features during shader pipeline processing
 *	\returns registered shader pipeline
 */
lptr<ShaderPipeline> Renderer::register_pipeline(const char* vs,const char* fs,TargetResolution tr,
												 RenderStage rs,u32 feat)
{
	COMM_LOG_COND(rs>=RENDER_STAGE_COUNT,"no custom pipelines for this stage allowed");
	m_ShaderPipelines.push_back(ShaderPipeline());
	lptr<ShaderPipeline> p_Pipeline = std::prev(m_ShaderPipelines.end());
	p_Pipeline->assemble(tr,vs,fs,RENDER_STAGES[rs],feat);
	return p_Pipeline;
}
// TODO somehow autodefine those by shader analysis? but there is a problem with result specification!

/**
 *	register triangle mesh batch
 *	\param pipeline: shader pipeline, handling pixel output for newly created batch
 *	\param size: amount of allowed geometry registrations
 *	\returns pointer to created triangle mesh batch
 */
lptr<GeometryBatch> Renderer::register_geometry_batch(lptr<ShaderPipeline> pipeline,size_t size)
{
	m_GeometryBatches.push_back({  });
	lptr<GeometryBatch> p_Batch = std::prev(m_GeometryBatches.end());
	p_Batch->assemble(pipeline,size);
	return p_Batch;
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
 *	register phyiscal mesh batch with standard geometry pass shader
 *	\param size: amount of allowed geometry registrations
 *	\returns pointer to created physical mesh batch
 */
lptr<GeometryBatch> Renderer::register_deferred_geometry_batch(size_t size)
{
	m_DeferredGeometryBatches.push_back({  });
	lptr<GeometryBatch> p_Batch = std::prev(m_DeferredGeometryBatches.end());
	p_Batch->assemble(m_GeometryPassPipeline,size);
	return p_Batch;
}

/**
 *	register physical mesh batch
 *	\param pipeline: shader pipeline, handling physical pass for newly created batch
 *	\param size: amount of allowed geometry registrations
 *	\returns pointer to created physical mesh batch
 */
lptr<GeometryBatch> Renderer::register_deferred_geometry_batch(lptr<ShaderPipeline> pipeline,size_t size)
{
	m_DeferredGeometryBatches.push_back({  });
	lptr<GeometryBatch> p_Batch = std::prev(m_DeferredGeometryBatches.end());
	p_Batch->assemble(pipeline,size);
	return p_Batch;
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
	// register geometry batch
	m_ShadowGeometryBatches.push_back({
			.batch = b,
			.shader = m_ShadowPassPipeline
		});
}

/**
 *	allow a geometry batch to cast shadows onto the scene
 *	\param b: pointer to casting geometry batch
 *	\param pipeline: pointer to custom shadow pass shader pipeline
 */
void Renderer::register_shadow_batch(lptr<GeometryBatch> b,lptr<ShaderPipeline> pipeline)
{
	// register geometry batch
	m_ShadowGeometryBatches.push_back({
			.batch = b,
			.shader = pipeline,
		});
}

/**
 *	allow a particle batch to cast shadows onto the scene
 *	\param b: pointer to casting particle batch
 *	\param pipeline: pointer to custom shadow pass shader pipeline
 */
void Renderer::register_shadow_batch(lptr<ParticleBatch> b,lptr<ShaderPipeline> pipeline)
{
	m_ShadowParticleBatches.push_back({
			.batch = b,
			.shader = pipeline,
		});
}

/**
 *	directional light, without distance relevance, no attenuation, indiscriminately orthogonal
 *	\param position: light position, position describes the sunlight's positional attitude towards origin
 *	\param colour: light colour vector
 *	\param intensity: light brightness
 *	\param shadow_source: (default false) true if this sunlight should cast a shadow
 *	\returns pointer to added sunlight
 */
SunLight* Renderer::add_sunlight(vec3 position,vec3 colour,f32 intensity,bool shadow_source)
{
	m_UBufferMem.lighting.sunlights[m_UBufferMem.lighting.sunlights_active] = {
		.position = position,
		.colour = colour*intensity
	};

	// update orthographic light projection
	if (shadow_source)
	{
		m_ShadowProjection = Camera3D(vec3(0),position,30,30,1000.f);
		m_UBufferMem.orth_shadow.view = m_ShadowProjection.view;
		m_UBufferMem.orth_shadow.proj = m_ShadowProjection.proj;
		m_UBufferMem.orth_shadow.source = position;
	}

	return &m_UBufferMem.lighting.sunlights[m_UBufferMem.lighting.sunlights_active++];
}
// TODO stacking multiple shadow sources to combine into mutidirectional map
// TODO also shadow cascading and dynamic targeting information, attached to scene center or camera focus

/**
 *	pointlight attenuates over distance, as described by a function
 *	\param position: light source position
 *	\param colour: light colour vector
 *	\param intensity: light brightness
 *	\param constant: constant component of pointlight attenuation function
 *	\param linear: linear component of pointlight attenuation function
 *	\param quadratic: quadratic component of pointlight attenuation function
 *	\returns pointer to added pointlight
 */
PointLight* Renderer::add_pointlight(vec3 position,vec3 colour,f32 intensity,f32 constant,
									 f32 linear,f32 quadratic)
{
	m_UBufferMem.lighting.pointlights[m_UBufferMem.lighting.pointlights_active] = {
		.position = position,
		.colour = colour*intensity,
		.constant = constant,
		.linear = linear,
		.quadratic = quadratic
	};
	return &m_UBufferMem.lighting.pointlights[m_UBufferMem.lighting.pointlights_active++];
}

/**
 *	register animated mesh for automatic animation
 *	\param mesh: address of mesh to automatically animate
 */
void Renderer::animate(AnimatedMesh* mesh)
{
	m_AnimatingMeshes.push_back(mesh);
}

/**
 *	update draw of all registered sprites
 */
void Renderer::_update_sprites()
{
#ifdef VKBUILD
	m_SpriteVertexArray.bind();
	m_SpritePipeline->enable();
	m_SpriteUBO.bind();
	vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,6,m_Sprites.active_range,0,0);
#else
	m_SpriteVertexArray.bind();
	m_SpriteInstanceBuffer.bind();
	m_SpriteInstanceBuffer.upload_vertices(m_Sprites.mem);
	m_SpritePipeline.enable();
	glDrawArraysInstanced(GL_TRIANGLES,0,6,m_Sprites.active_range);
#endif
}

/**
 *	update draw of all registered characters from text components
 */
void Renderer::_update_text()
{
#ifdef VKBUILD
	m_TextVertexArray.bind();
	m_TextPipeline->enable();
	m_TextUBO.bind();
	vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,6,Text::characters.active_range,0,0);
#else
	m_TextVertexArray.bind();
	m_TextInstanceBuffer.bind();
	m_TextPipeline.enable();
	for (Text& p_Text : m_Texts)
	{
		m_TextInstanceBuffer.upload_vertices(&p_Text.buffer[0],p_Text.buffer.size()*sizeof(TextCharacter));
		glDrawArraysInstanced(GL_TRIANGLES,0,6,p_Text.buffer.size());
	}
#endif
}

/**
 *	draw final frame
 */
void Renderer::_update_target()
{
	m_TargetVertexArray.bind();
	m_TargetPipeline->enable();
	m_GlobalDescriptorSet.bind(m_TargetPipeline->pipeline_layout);
	vkCmdDraw(g_GPU.acquire_graphical_command_buffer()->buffer,6,1,0,0);
}

/**
 *	update shader uniform variables
 */
void Renderer::_update_uniforms()
{
	// camera update
	m_UBufferMem.otrafo.view = g_Camera.view;
	m_UBufferMem.otrafo.proj = g_Camera.proj;
	m_UBufferMem.camera.position = g_Camera.position;
	// TODO dont copy over like this

	// update global set
	g_UniformBuffer.write(&m_UBufferMem,sizeof(UniformBufferMemory),m_GlobalSetOffset);
	m_GlobalDescriptorSet.update_frame();

	// update ui uniforms
	m_SpriteUBO.update();
	m_TextUBO.update();

	// iterate registered geometry batches for uniform updates
	for (GeometryBatch& p_Batch : m_GeometryBatches) p_Batch.update();
	for (GeometryBatch& p_Batch : m_DeferredGeometryBatches) p_Batch.update();

	// iterate registered geometry batches for uniform updates
	for (ParticleBatch& p_Batch : m_ParticleBatches) p_Batch.update_uniform();
	for (ParticleBatch& p_Batch : m_DeferredParticleBatches) p_Batch.update_uniform();
}

/**
 *	handle pending gpu uploads for all forwarded data
 */
void Renderer::_gpu_upload()
{
	// text & sprites
	m_SpriteInstanceBuffer.register_upload(m_Sprites.active_range*sizeof(Sprite));
	m_TextInstanceBuffer.register_upload(Text::characters.active_range*sizeof(TextCharacter));

	// execute upload for all registered geometric data updates
	g_VertexBufferHeap[VERTEX_BUFFER_TYPE_STATIC].gpu_upload();
	g_VertexBufferHeap[VERTEX_BUFFER_TYPE_DYNAMIC].gpu_upload();
	// TODO later only regularely update dynamic component & static component only when requested

	// upload textures
	m_GPUSpriteTextures.gpu_upload();
	m_GPUFontTextures.gpu_upload();

	// mesh textures
	bool __MeshTextureUpdated = false;
	m_MeshTextureUploadQueue.mut.lock();
	while (m_MeshTextureUploadQueue.tqueue.size())
	{
		TextureDataTuple& p_Tuple = m_MeshTextureUploadQueue.tqueue.front();
		p_Tuple.texture->allocate(p_Tuple.data.width,p_Tuple.data.height,p_Tuple.format,p_Tuple.features);
		p_Tuple.texture->transition_texture_transfer();
		p_Tuple.texture->gpu_upload(p_Tuple.data);
		m_MeshTextureUploadQueue.tqueue.pop();
		__MeshTextureUpdated = true;
	}
	m_MeshTextureUploadQueue.mut.unlock();

	m_CubemapUploadQueue.mut.lock();
	while (m_CubemapUploadQueue.tqueue.size())
	{
		Cubemap* p_Cubemap = m_CubemapUploadQueue.tqueue.front();
		p_Cubemap->gpu_upload();
		m_CubemapUploadQueue.tqueue.pop();
	}
	m_CubemapUploadQueue.mut.unlock();

	// link results, process attachments
	if (!__MeshTextureUpdated) return;
	for (lptr<TextureAttachment> p_Attachment = m_TextureAttachmentRequests.begin();
		 p_Attachment!=m_TextureAttachmentRequests.end();)
	{
		if (!p_Attachment->texture->allocated)
		{
			p_Attachment++;
			continue;
		}
		p_Attachment->ubo->link_result(p_Attachment->location,p_Attachment->texture);
		p_Attachment = m_TextureAttachmentRequests.erase(p_Attachment);
	}
}
// TODO when closing the program, show the maximum amount of used sprite, texture and mesh index slots
//		the measurement has to apply to a single update state
// TODO skip parts of this upload based on change signals by load commands
// TODO this also proposes the implementation of a maximum allowed texture range read for the device

#else


/**
 *	upload all setup lights to gpu lighting simulation processing
 */
/*
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
*/
// TODO dynamic upload, e.g. when a single light gets updated all the lights need to be uploaded again

/**
 *	deactivate all lights to fundamentally reset the lighting setup
 */
/*
void Renderer::reset_lighting()
{
	m_Lighting.sunlights_active = 0;
	m_Lighting.pointlights_active = 0;
	upload_lighting();
}
*/


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
template void Renderer::_collector<Rect>(InPlaceArray<Rect>*,ThreadSignal*);

#endif
