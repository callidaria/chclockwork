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
// Additional Utility

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

	// adjust vertical alignment
	u8 vertical_alignment = 2-(alignment%3);
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
	COMM_LOG_COND(buffer.capacity()<data.size(),"allocating memory for text buffer");
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
			.uniform_location = "joint_transform["+std::to_string(__MemoryID)+"]",
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
	current_animation = id;
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

/**
 *	recursively transform joint tree based on each parent (funamentals of forward kinematics)
 *	\param joint: joint of current root in animation joint subtree
 *	\param parent_transform: transformation basis inherited from parent joint
 */
void AnimatedMesh::_rc_transform_interpolation(MeshJoint& joint,mat4& parent_transform)
{
	mat4 __LocalTransform = parent_transform*joint.transform;
	joint.recursive_transform = __LocalTransform*joint.offset;
	for (u16 child : joint.children) _rc_transform_interpolation(joints[child],__LocalTransform);
}


#ifdef VKBUILD
const s32 TEST_INSTANCE_AMOUNT_X = 3;
const s32 TEST_INSTANCE_AMOUNT_Y = 3;
const s32 TEST_INSTANCE_AMOUNT_Z = 3;
const s32 TEST_INSTANCE_AMOUNT_GENERAL
			= TEST_INSTANCE_AMOUNT_X*TEST_INSTANCE_AMOUNT_Y*TEST_INSTANCE_AMOUNT_Z;

// TODO those are all prototype implementations!
//		doc will be created later down the line when everything is in order
Renderer::Renderer()
{
	g_GPU.swap();

	Mesh __Mesh = Mesh("./res/private/test.obj");
	vector<u32> __Indices(__Mesh.vertices.size());
	std::iota(__Indices.begin(),__Indices.end(),0);
	m_RenderSize = __Indices.size();

	// instances
	u32 i = 0;
	ObjectInstance __Instances[TEST_INSTANCE_AMOUNT_GENERAL] = { };
	for (s32 z=-TEST_INSTANCE_AMOUNT_Z/2;z<(TEST_INSTANCE_AMOUNT_Z/2)+TEST_INSTANCE_AMOUNT_Z%2;z++)
	{
		for (s32 y=-TEST_INSTANCE_AMOUNT_Y/2;y<(TEST_INSTANCE_AMOUNT_Y/2)+TEST_INSTANCE_AMOUNT_Y%2;y++)
		{
			for (s32 x=-TEST_INSTANCE_AMOUNT_X/2;x<(TEST_INSTANCE_AMOUNT_X/2)+TEST_INSTANCE_AMOUNT_Z%2;x++)
			{
				__Instances[i] = { vec3(x*2,y*2,z*2) };
				i++;
			}
		}
	}

	// render target
	m_Framebuffer.define_colour_component(0,FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_Framebuffer.define_depth_component(FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_Framebuffer.finalize();
	m_Framebuffer.link_output();

	// vertex data
	m_VertexBuffer.allocate(sizeof(Vertex)*__Mesh.vertices.size()+sizeof(u32)*__Indices.size(),true);
	m_VertexBuffer.upload(&__Mesh.vertices[0],sizeof(Vertex)*__Mesh.vertices.size(),
						  &__Indices[0],sizeof(u32)*__Indices.size());
	m_VertexBuffer.update();

	// instance data
	m_InstanceBuffer.allocate(sizeof(__Instances));
	m_InstanceBuffer.upload(__Instances,sizeof(__Instances));

	// vertex array
	m_VertexArray.allocate(2);
	m_VertexArray.register_buffer_indexed(m_VertexBuffer);
	m_VertexArray.register_buffer(m_InstanceBuffer);

	// texture
	m_PixelBuffer.load_texture("./res/private/test.png");

	// §§testing
	g_UniformBuffer.define(0,sizeof(ObjectTransformation));
	g_UniformBuffer.define(1,m_PixelBuffer);
	g_UniformBuffer.assemble();

	// pipeline
	m_TestingPipeline.assemble(m_Framebuffer,
							   "./core/shader/vulkan/bin/mesh.vert",
							   "./core/shader/vulkan/bin/mesh.frag");

	m_Rotation = glm::radians(-120.f);
}

void Renderer::update()
{
	m_TestingPipeline.enable();
	m_InstanceBuffer.update();
	m_VertexArray.transfer_ownership();
	m_Framebuffer.record();
	m_VertexArray.bind_indexed(m_Framebuffer);

	// camera update test
	m_Trafo.view = g_Camera.view;
	m_Trafo.proj = g_Camera.proj;
	// TODO also create the ability the link a camera to the uniform
	//		right now this happens for both matrices individually, which is not appropriate

	// prototype update tbr
	//m_Rotation += g_Frame.delta_time*glm::radians(4.f);
	m_Trafo.model = glm::rotate(mat4(1.f),m_Rotation,vec3(0,0,1));
	g_UniformBuffer.update(&m_Trafo,sizeof(m_Trafo));

	// drawcall
	vkCmdBindDescriptorSets(g_GPU.acquire_graphical_command_buffer()->buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,
							m_TestingPipeline.pipeline_layout,0,1,
							&g_UniformBuffer.m_DSets[g_GPU.active_buffer],0,nullptr);
	vkCmdDrawIndexed(g_GPU.acquire_graphical_command_buffer()->buffer,
					 m_RenderSize,TEST_INSTANCE_AMOUNT_GENERAL,0,0,0);

	m_Framebuffer.stop();
}

/**
 *	sorting out how this makes most sense:
 *
 *	1st the buffers
 *		creating a buffer should be done by generating a buffer, uploading to it and having a few options
 *		options when and if to include index buffers might be offered here, but that really just happens at bind
 *		if is a buffer staged, mapped and unmapped is a hugely important distinction
 *		there is a difference between vertex buffers only in the case of upload additional index information
 *		vertex buffers (as they are) only hold geometrical information, fed to the shaders. this is their purpose
 *		the vertex buffer should store it's offset, so that authoritative callers can utilize them when uploading
 *		maybe it should be possible to store multiple geometric buffers into a single vbo??
 *		research if streaming into a single global vbo for all equi-material wg is an efficient alternative
 *		the default case for upload is always through a staging buffer. this seems to be the fastest way
 *		this includes all vbs, those that stay the same throughout and also dynamic instance data buffers
 *		vertex buffer memory is static and can be freed immediately, while instance memory needs to stay active
 *		this suggests, that the staging/upload and the unmap/free call are to be separated, depending on usage
 *		also there must be a feature to directly update data in-memory
 *		copy must be part of an update function, that can be called once in case of vb or per-frame in case of ib
 *
 *	2nd the "vertex array"
 *		to bind and upload the buffers they will be called by bind vertex/index buffer commands descriptively
 *		the vertex array should not be a distict structure or class, it only acts as an analogue to ogl
 *		during the bind process, the cmd_buffer in framebuffer will be used to store the command until execution
 *		this raises the important question if the framebuffer as such should take care of buffer upload
 *
 *	3rd the static pipeline
 *		the pipeline should work with the buffers and uniforms, but it should NOT be defined by them.
 *		the pipeline setup will consist of only direct decisions and automatically analyzed shader states
 *		an example of the latter is already implemented in the ogl version:
 *		the pipeline will read the defined needs by the shader and adopt the desired upload structure
 *		it is then the duty of the buffers to provide the requested data, they have no influence over the demands
 *
 *	4th the framebuffer rendertarget
 *		the rendertarget is supposed to be !the! command authority. all draw commands belong to a target
 *		a rendertarget should also stand on it's own feet and should not obey any other authority, besides mutex
 *		this should suggest that the framebuffer rendertarget and the blitter are to be unified into one
 *		for each target there is a respective command buffer, as there should be when flipping asynchronously
 *		but right now blitter and framebuffer are in a dire war over resource authority, that should be sorted
 *
 *	5th the blitter
 *		the blitter should only receive what is designated as the final result in form of a rendertarget
 *		first it has to create the frame, setup swapchain et cetera and then it should draw the target
 *		this is the full extent of the blitters purpose
 *
 *	6th the hardware interface
 *		the hardware interface is supposed to be handling all frees and mundane creations
 *		creations that require a complex info struct setup are not to be misinterpreted as mundane
 *		the hi is supposed to automatically scan for relevant hardware and test it for support upon inclusion
 *		also the hi should globally store the gpu, so that it is accessible to all following components
 */

void Renderer::vanish()
{
	m_TestingPipeline.vanish();
	m_Framebuffer.vanish();
	m_VertexBuffer.free();
	// FIXME allow for vbs to be free'd right after upload without fencelocking the host. what an embarrassment
	m_VertexBuffer.vanish();
	m_InstanceBuffer.free();
	m_InstanceBuffer.vanish();
	m_PixelBuffer.vanish();
	g_UniformBuffer.vanish();
}


#else

// ----------------------------------------------------------------------------------------------------
// Geometry Batching

/**
 *	add mesh geometry to batch
 *	\param mesh: loaded mesh for explicit geometry information
 *	\param tex: multichannel texture data to upload
 *	\returns geometry id
 */
u32 GeometryBatch::add_geometry(Mesh& mesh,const vector<Texture*>& tex)
{
	return add_geometry(&mesh.vertices[0],mesh.vertices.size(),sizeof(Vertex),tex);
}

/**
 *	add animated mesh geometry to batch
 *	\param mesh: animated mesh for explicit geometry information
 *	\param tex: multichannel texture data to upload
 *	\returns geometry id
 */
u32 GeometryBatch::add_geometry(AnimatedMesh& mesh,const vector<Texture*>& tex)
{
	u32 id = add_geometry(&mesh.vertices[0],mesh.vertices.size(),sizeof(AnimationVertex),tex);
	for (MeshJoint& p_Joint : mesh.joints)
		objects[id].uniform.attach_uniform(p_Joint.uniform_location.c_str(),&p_Joint.recursive_transform);
	anim_meshes.push_back(&mesh);
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
u32 GeometryBatch::add_geometry(void* verts,size_t vsize,size_t ssize,const vector<Texture*>& tex)
{
	COMM_LOG("uploading geometry to batch");
	size_t __MemSize = vsize*ssize;
	size_t __Size = __MemSize/sizeof(f32);
	geometry.resize(geometry_cursor+__Size);
	memcpy(&geometry[geometry_cursor],verts,__MemSize);

	// store geometry information
	objects.push_back({
			.offset = offset_cursor,
			.vertex_count = vsize,
			.textures = tex,
		});
	objects.back().uniform.shader = shader;
	offset_cursor += vsize;
	geometry_cursor += __Size;
	return objects.size()-1;
}

/**
 *	upload batch geometry to gpu & automap shader pipeline
 */
void GeometryBatch::load()
{
	COMM_LOG("uploading geometry information to GPU");
	vbo.allocate(geometry.size()*sizeof(f32));
	vbo.upload(&geometry[0],geometry.size()*sizeof(f32));  // FIXME duplicate!
	shader->map(RENDERER_TEXTURE_UNMAPPED,&vbo);
}

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
	size_t size = vsize*ssize;
	geometry.resize(size/sizeof(f32));
	memcpy(&geometry[0],verts,size);

	// auto-mapping particle shader pipeline
	vbo.allocate(geometry.size()*sizeof(f32));
	//ibo.allocate(particles*isize);
	vbo.upload(&geometry[0],geometry.size()*sizeof(f32));  // FIXME duplicate!
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
	/*
	m_SpritePipeline.assemble(__SpriteVertexShader,__DirectFragmentShader);
	m_SpriteVertexBuffer.allocate(24*sizeof(f32));
	m_SpriteInstanceBuffer.allocate(BUFFER_MAXIMUM_TEXTURE_COUNT*sizeof(Sprite),BUFFER_TYPE_INDEX);
	m_SpriteVertexBuffer.upload(__QuadVertices);
	m_SpritePipeline.map(RENDERER_TEXTURE_SPRITES,&m_SpriteVertexBuffer,&m_SpriteInstanceBuffer);
	m_SpritePipeline.upload_coordinate_system();

	COMM_LOG("text pipeline");
	m_TextPipeline.assemble(__TextVertexShader,__TextFragmentShader);
	m_TextInstanceBuffer.allocate(RENDERER_MAXIMUM_CHARACTER_COUNT*sizeof(TextCharacter),BUFFER_TYPE_INDEX);
	m_TextPipeline.map(RENDERER_TEXTURE_FONTS,&m_SpriteVertexBuffer,&m_TextInstanceBuffer);
	m_TextPipeline.upload_coordinate_system();
	*/

	COMM_LOG("canvas pipeline");
	m_CanvasPipeline.assemble(__CanvasVertexShader,__LightingPassFragmentShader);
	m_CanvasVertexBuffer.allocate(24*sizeof(f32));
	m_CanvasVertexBuffer.upload(__CanvasVertices,sizeof(__CanvasVertices));  // FIXME duplicate?!??
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
	m_GPUSpriteTextures.allocate(RENDERER_SPRITE_MEMORY_WIDTH,RENDERER_SPRITE_MEMORY_HEIGHT,TEXTURE_FORMAT_RGBA);
	Texture::set_texture_parameter_linear_mipmap();
	Texture::set_texture_parameter_clamp_to_edge();

	COMM_LOG("allocating font memory");
	m_GPUFontTextures.atlas.bind(RENDERER_TEXTURE_FONTS);
	m_GPUFontTextures.allocate(RENDERER_FONT_MEMORY_WIDTH,RENDERER_FONT_MEMORY_HEIGHT,TEXTURE_FORMAT_MONOCHROME);
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
	m_ShadowFrameBuffer.stop();

	// ----------------------------------------------------------------------------------------------------
	// Start Subprocesses

	COMM_LOG("starting renderer subprocesses");
	/*
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
 *	precalculating setup (before wheel system setup)
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
 *	render visual result
 */
void Renderer::update()
{
	// shadow projection
	g_GPU.cull_backfaces(false);
	g_Frame.set_viewport(RENDERER_SHADOW_RESOLUTION,RENDERER_SHADOW_RESOLUTION);
	m_ShadowFrameBuffer.start();
	_update_shadows(m_ShadowGeometryBatches,m_ShadowParticleBatches);
	g_GPU.cull_backfaces(true);

	// 3D segment
	g_Frame.set_viewport(FRAME_RESOLUTION_X,FRAME_RESOLUTION_Y);
	m_ForwardFrameBuffer.start();
	_update_mesh(m_GeometryBatches,m_ParticleBatches);
	m_DeferredFrameBuffer.start();
	_update_mesh(m_DeferredGeometryBatches,m_DeferredParticleBatches);
	m_DeferredFrameBuffer.stop();

	// rendertargets
	g_GPU.disable_feature(GPU_FEATURE_DEPTH_TEST);
	_update_canvas();
	g_GPU.enable_feature(GPU_FEATURE_DEPTH_TEST);

	// 2D segment
	_update_sprites();
	_update_text();

	// end-frame gpu management
	_gpu_upload();
}

/**
 *	exit renderer and end all it's subprocesses
 */
void Renderer::vanish()
{
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
	if (alignment.alignment!=SCREEN_ALIGN_NEUTRAL)
	{
		vec2 hsize = size*.5f;
		vec2 __AlignedPosition = alignment.align({ vec2(position)-hsize,size })+size;
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
	thread __LoadThread(_load_texture,p_Texture,path,format,
						&m_MeshTextureUploadQueue,&m_MutexMeshTextureUpload);
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
	// register geometry batch
	m_ShadowGeometryBatches.push_back({
			.batch = b,
			.shader = m_GeometryShadowPipeline,
			.uniform = vector<ShaderUniformUpload>(b->objects.size()),
		});

	// create uniform upload correlation
	for (u32 i=0;i<b->objects.size();i++)
	{
		m_ShadowGeometryBatches.back().uniform[i].shader = m_GeometryShadowPipeline;
		m_ShadowGeometryBatches.back().uniform[i].correlate(b->objects[i].uniform);
	}
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
			.uniform = vector<ShaderUniformUpload>(b->objects.size()),
		});

	// create uniform upload correlation
	for (u32 i=0;i<b->objects.size();i++)
	{
		m_ShadowGeometryBatches.back().uniform[i].shader = pipeline;
		m_ShadowGeometryBatches.back().uniform[i].correlate(b->objects[i].uniform);
	}
}

/**
 *	allow a particle batch to cast shadows onto the scene
 *	\param b: pointer to casting particle batch
 */
void Renderer::register_shadow_batch(lptr<ParticleBatch> b)
{
	m_ShadowParticleBatches.push_back({
			.batch = b,
			.shader = m_ParticleShadowPipeline,
		});
	//m_ShadowParticleBatches.uniform.correlate(b->uniform);
	// TODO expand the correlation when particle batch allows for uniform upload specification
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
	//m_ShadowParticleBatches.uniform.correlate(b->uniform);
}

/**
 *	create directional sunlight
 *	if there is no explicitly defined shadow projection the first sunlight will automatically create one
 *	\param position: direction to sunlight, inverted direction will be direction of emission
 *	\param colour: colour of the emission
 *	\param intensity: emission intensity, multiplying the colour influence
 */
SunLight* Renderer::add_sunlight(vec3 position,vec3 colour,f32 intensity)
{
	// add sunlight
	m_Lighting.sunlights[m_Lighting.sunlights_active] = {
		.position = position,
		.colour = colour*intensity,
	};

	// auto-set shadow projection
	if (!m_Lighting.shadow_forced&&!m_Lighting.sunlights_active) add_shadow(position,false);
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
 *	\param forced: (default true) true if this shadow projection overrides incoming lighting changes
 */
void Renderer::add_shadow(vec3 source,bool forced)
{
	m_Lighting.shadow_forced = forced;
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
 *	register animated mesh for automatic animation
 *	\param mesh: address of mesh to automatically animate
 */
void Renderer::animate(AnimatedMesh* mesh)
{
	m_AnimatingMeshes.push_back(mesh);
}

/**
 *	update all registered sprites
 */
void Renderer::_update_sprites()
{
	/*
	m_SpriteVertexArray.bind();
	m_SpriteInstanceBuffer.bind();
	m_SpriteInstanceBuffer.upload_vertices(m_Sprites.mem);
	m_SpritePipeline.enable();
	glDrawArraysInstanced(GL_TRIANGLES,0,6,m_Sprites.active_range);
	*/
}

/**
 *	update all registered text
 */
void Renderer::_update_text()
{
	/*
	// prepare gpu
	m_TextVertexArray.bind();
	m_TextInstanceBuffer.bind();
	m_TextPipeline.enable();

	// iterate text entities
	for (Text& p_Text : m_Texts)
	{
		m_TextInstanceBuffer.upload_vertices(&p_Text.buffer[0],p_Text.buffer.size()*sizeof(TextCharacter));
		glDrawArraysInstanced(GL_TRIANGLES,0,6,p_Text.buffer.size());
	}
	*/
}

/**
 *	update framebuffer representations
 */
void Renderer::_update_canvas()
{
	m_CanvasVertexBuffer.bind();
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
		p_Batch.vbo.bind();
		for (GeometryTuple& p_Tuple : p_Batch.objects)
		{
			// texture upload
			for (u8 i=0;i<p_Tuple.textures.size();i++) p_Tuple.textures[i]->bind(RENDERER_TEXTURE_UNMAPPED+i);

			// upload attached uniform value pointers
			p_Batch.shader->upload_camera();
			p_Tuple.uniform.upload();
			p_Batch.shader->upload("model",p_Tuple.transform.model);
			p_Batch.shader->upload("texel",p_Tuple.texel);

			// upload standard values & call gpu
			glDrawArrays(GL_TRIANGLES,p_Tuple.offset,p_Tuple.vertex_count);
		}
	}
	// FIXME uploading camera and then afterwards maybe overwrite it is working but it is shite

	// iterate particle geometry
	/*
	for (ParticleBatch& p_Batch : pb)
	{
		p_Batch.shader->enable();
		p_Batch.shader->upload_camera();
		p_Batch.vbo.bind();
		glDrawArraysInstanced(GL_TRIANGLES,0,p_Batch.vertex_count,p_Batch.active_particles);
	}
	*/
}

/**
 *	draw casting geometry simplified for shadow projection
 *	\param gb: casting geometry batches for shadow projection
 *	\param pb: casting particle batches for shadow projection
 */
void Renderer::_update_shadows(list<ShadowGeometryBatch>& gb,list<ShadowParticleBatch>& pb)
{
	// iterate static geometry
	for (ShadowGeometryBatch& p_Batch : gb)
	{
		p_Batch.shader->enable();
		p_Batch.shader->upload_camera(m_Lighting.shadow_projection);
		p_Batch.batch->vbo.bind();
		for (u32 i=0;i<p_Batch.batch->objects.size();i++)
		{
			GeometryTuple& p_Tuple = p_Batch.batch->objects[i];
			p_Batch.uniform[i].upload();
			p_Batch.shader->upload("model",p_Tuple.transform.model);
			glDrawArrays(GL_TRIANGLES,p_Tuple.offset,p_Tuple.vertex_count);
		}
	}

	// iterate particle geometry
	/*
	for (ShadowParticleBatch& p_Batch : pb)
	{
		p_Batch.shader->enable();
		p_Batch.shader->upload_camera(m_Lighting.shadow_projection);
		p_Batch.batch->vao.bind();
		glDrawArraysInstanced(GL_TRIANGLES,0,p_Batch.batch->vertex_count,p_Batch.batch->active_particles);
	}
	*/
}

/**
 *	helper to unclutter the automatic load callbacks for gpu data
 */
void Renderer::_gpu_upload()
{
	m_GPUSpriteTextures.gpu_upload(RENDERER_TEXTURE_SPRITES);
	m_GPUFontTextures.gpu_upload(RENDERER_TEXTURE_FONTS);

	// singular textures
	m_MutexMeshTextureUpload.lock();
	while (m_MeshTextureUploadQueue.size()&&calculate_delta_time_ms(g_Frame.fstart)<FRAME_TIME_BUDGET_MS)
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

#endif
