#ifndef CORE_RENDERER_HEADER
#define CORE_RENDERER_HEADER

#include <numeric>
#include "gpu_interface.h"
#include "buffer.h"
#include "shader.h"


// ----------------------------------------------------------------------------------------------------
// Constants

// range & code
constexpr f32 RENDERER_POSITIONAL_DELETION_CODE = -1237.f;
constexpr u8 RENDERER_ANIMATION_INFLUENCE_RANGE = 4;

// forward buffer channels
enum ForwardBufferChannel
{
	FORWARD_BUFFER_CHANNEL_COLOUR,
	FORWARD_BUFFER_CHANNEL_DEPTH,
	FORWARD_BUFFER_CHANNEL_COUNT
};

// deferred buffer channels
enum DeferredBufferChannel
{
	DEFERRED_BUFFER_CHANNEL_COLOUR,
	DEFERRED_BUFFER_CHANNEL_POSITION,
	DEFERRED_BUFFER_CHANNEL_NORMAL,
	DEFERRED_BUFFER_CHANNEL_MATERIAL,
	DEFERRED_BUFFER_CHANNEL_EMISSION,
	DEFERRED_BUFFER_CHANNEL_DEPTH,
	DEFERRED_BUFFER_CHANNEL_COUNT
};

// location endpoint
enum GlobalShaderEndpoint : u16
{
	GSE_CAMERA_TRANSFORM_LOCATION = 0,
	GSE_CSYSTEM_TRANSFORM_LOCATION,
	GSE_SPRITE_ATLAS_LOCATION,
	GSE_FONT_ATLAS_LOCATION,
	GSE_FORWARD_COLOUR_LOCATION,
	GSE_FORWARD_DEPTH_LOCATION,
	GSE_GBUFFER_COLOUR_LOCATION,
	GSE_GBUFFER_POSITION_LOCATION,
	GSE_GBUFFER_NORMAL_LOCATION,
	GSE_GBUFFER_MATERIAL_LOCATION,
	GSE_GBUFFER_EMISSION_LOCATION,
	GSE_GBUFFER_DEPTH_LOCATION,
	GSE_SHADOW_MAP_LOCATION,
	GSE_CAMERA_ATTRIBUTES_LOCATION = 40,
	GSE_LIGHTING_LOCATION,
	GSE_SHADOW_LOCATION,
};

// uniform buffer set description
enum UBOChannelPurpose
{
	UBO_CHANNEL_PURPOSE_DATA,
	UBO_CHANNEL_PURPOSE_TEXTURES,
	UBO_CHANNEL_PURPOSE_AUTOUPDATE
};

constexpr u16 RENDERER_GEOMETRY_INFO_LOCATION = 70;
constexpr u16 RENDERER_JOINT_ANIMATION_LOCATION = 128;
// TODO check dynamic endpoint setup against possible constant violations


// ----------------------------------------------------------------------------------------------------
// GBuffer Layout

constexpr u8 RENDERER_MAXIMUM_BUFFER_FORMATS = 8;
struct PipelineBufferFormats
{
	GPUBufferFormat formats[RENDERER_MAXIMUM_BUFFER_FORMATS];
	u8 size;
	bool depth;
};

// formats
static inline PipelineBufferFormats RENDERER_STANDARD_BUFFER_FORMAT = {
	.formats = { GPU_BUFFER_FORMAT_SRGB },
	.size = 1,
	.depth = true
};
static inline PipelineBufferFormats RENDERER_GEOMETRY_BUFFER_FORMAT = {
	.formats = {
		GPU_BUFFER_FORMAT_RGB,
		GPU_BUFFER_FORMAT_FLOAT,GPU_BUFFER_FORMAT_FLOAT,GPU_BUFFER_FORMAT_FLOAT,
		GPU_BUFFER_FORMAT_RGB
	},
	.size = 5,
	.depth = true
};
static inline PipelineBufferFormats RENDERER_DEPTH_BUFFER_FORMAT = {
	.formats = {  },
	.size = 0,
	.depth = true
};

enum TextureChannelMap : u16
{
	RENDERER_TEXTURE_SPRITES,
	RENDERER_TEXTURE_FONTS,
	RENDERER_TEXTURE_FORWARD,
	RENDERER_TEXTURE_DEFERRED_COLOUR,
	RENDERER_TEXTURE_DEFERRED_POSITION,
	RENDERER_TEXTURE_DEFERRED_NORMAL,
	RENDERER_TEXTURE_DEFERRED_MATERIAL,
	RENDERER_TEXTURE_DEFERRED_EMISSION,
	RENDERER_TEXTURE_SHADOW_MAP,
	RENDERER_TEXTURE_FORWARD_DEPTH,
	RENDERER_TEXTURE_DEFERRED_DEPTH,
	RENDERER_TEXTURE_UNMAPPED
};


// ----------------------------------------------------------------------------------------------------
// GPU Data Structures

struct Sprite
{
	vec3 offset = vec3(0);
	vec2 scale = vec2(0);
	f32 rotation = .0f;
	f32 alpha = 1.f;
	Rect pbc;
};

struct TextCharacter
{
	vec3 offset = vec3(0);
	vec2 scale = vec2(0);
	vec2 bearing = vec2(0);
	vec4 colour = vec4(1);
	Rect comp;
};

struct Vertex
{
	vec3 position;
	vec2 uv;
	vec3 normal;
	vec3 tangent;
};

struct AnimationVertex
{
	vec3 position;
	vec2 uv;
	vec3 normal;
	vec3 tangent;
	vec4 bone_index;
	vec4 bone_weight;
};


// ----------------------------------------------------------------------------------------------------
// Materials

enum MaterialComponentChannel : u8
{
	MATERIAL_COMPONENT_CHANNEL_ALBEDO,
	MATERIAL_COMPONENT_CHANNEL_NORMAL,
	MATERIAL_COMPONENT_CHANNEL_MATERIAL,
	MATERIAL_COMPONENT_CHANNEL_EMISSION,
	MATERIAL_COMPONENT_CHANNEL_COUNT
};

struct Material
{
	static Texture* standards[MATERIAL_COMPONENT_CHANNEL_COUNT];
	Texture* components[MATERIAL_COMPONENT_CHANNEL_COUNT];
};


// ----------------------------------------------------------------------------------------------------
// Animations

template<typename T> struct AnimKey
{
	T key;
	f64 duration = .0;
};

struct AnimationJoint
{
	u16 id;
	u16 crr_position = 0;
	u16 crr_scale = 0;
	u16 crr_rotation = 0;
	vector<AnimKey<vec3>> position_keys;
	vector<AnimKey<vec3>> scaling_keys;
	vector<AnimKey<quat>> rotation_keys;
};

struct Animation
{
	vector<AnimationJoint> joints;
	f64 duration;
	f64 duration_inv;
};


// ----------------------------------------------------------------------------------------------------
// Text

enum ScreenAlignment
{
	SCREEN_ALIGN_BOTTOMLEFT,
	SCREEN_ALIGN_CENTERLEFT,
	SCREEN_ALIGN_TOPLEFT,
	SCREEN_ALIGN_BOTTOMCENTER,
	SCREEN_ALIGN_CENTER,
	SCREEN_ALIGN_TOPCENTER,
	SCREEN_ALIGN_BOTTOMRIGHT,
	SCREEN_ALIGN_CENTERRIGHT,
	SCREEN_ALIGN_TOPRIGHT,
	SCREEN_ALIGN_NEUTRAL,
};

struct Alignment
{
	vec2 align(Rect geom);
	Rect border = { vec2(0),vec2(MATH_CARTESIAN_XRANGE,MATH_CARTESIAN_YRANGE) };
	ScreenAlignment alignment = SCREEN_ALIGN_NEUTRAL;
};

struct Text
{
	// utility
	void align();
	void load_buffer();
	u32 intersection(f32 pos);
	void vanish();

	// data
	Font* font;
	vec3 position;
	vec2 offset;
	f32 scale;
	vec2 dimensions;
	vec4 colour;
	Alignment alignment;
	string data;
	vector<size_t> buffer;

	// global
	inline static InPlaceArray<TextCharacter> characters;
};


// ----------------------------------------------------------------------------------------------------
// Meshes

class Mesh
{
public:
	Mesh(const char* path);
	static inline Mesh sphere_lowres() { return Mesh("./res/primitive/low_sphere.obj"); }
	static inline Mesh sphere() { return Mesh("./res/primitive/sphere.obj"); };
	static inline Mesh cube() { return Mesh("./res/primitive/cube.obj"); }
	static inline Mesh ape() { return Mesh("./res/primitive/ape.obj"); }
	// TODO pre-store geometry and copy, this will always reload from disc when new primitive is created

public:
	vector<Vertex> vertices;
};
// TODO element draw requires an element array instead of raw vertex store (also for AnimatedMesh)

struct MeshJoint
{
	// utility
	void request_position(const AnimKey<vec3>& target);
	void request_scale(const AnimKey<vec3>& target);
	void request_rotation(const AnimKey<quat>& target);
	void interpolate();

	// data mapping
	string id;
#ifdef VKBUILD
	u32 uniform_location;
#else
	string uniform_location;
#endif

	// transform data manipulation
	f32 prog_position = .0f;
	f32 prog_scale = .0f;
	f32 prog_rotation = .0f;
	vec3 ct_position;
	vec3 ct_scale;
	quat ct_rotation;
	AnimKey<vec3> crr_position;
	AnimKey<vec3> crr_scale;
	AnimKey<quat> crr_rotation;
	AnimKey<vec3> target_position;
	AnimKey<vec3> target_scale;
	AnimKey<quat> target_rotation;

	// final transform
	mat4 offset;
	mat4 transform = mat4(1.f);
	mat4 local_transform = mat4(1.f);

	// tree structure
	vector<u16> children;
};
// TODO maybe instead of indices, pointers for children?

class AnimatedMesh
{
public:
	AnimatedMesh(const char* path);
	void set_default_animation(u8 id,f32 tt=.0f);
	void set_animation(u8 id,f32 tt=.0f);
	MeshJoint* find_joint(const string& id);
	f64 get_progress();
	void animate();
	void update();

private:
	void _rc_transform_interpolation(MeshJoint& joint,mat4& parent_transform);

public:
	vector<AnimationVertex> vertices;
	vector<MeshJoint> joints;
	vector<mat4> recursive_transforms;
	vector<Animation> animations;
	u16 current_animation = 0;

private:
	u16 m_StandardAnimation = 0;
	f32 m_StandardTransitionTime = .0f;
	f32 m_AnimationTransitionTime = .0f;
	f64 m_Progress = .0;
};
// TODO do not load this into it's own animation mesh, rather than extract animations and attach to mesh later
//		then again this can be done later, the related mesh probably will always be attached (weighing)


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer Object

struct TextureAttachment
{
	DescriptorSetMemory* ubo;
	Texture* texture;
	u16 location;
};

struct UniformBufferObject
{
	// utility
	void create(lptr<ShaderPipeline> shader);
	void bind();
	void update();
	void auto_update();
	void attach_memory(u16 location,void* data);
	void attach_texture(u16 location,Texture* texture);
	void attach_material(Material* material);

	// data
	vector<DescriptorSetMemory> dsets;
	ShaderUniformUpload uniform;
};


// ----------------------------------------------------------------------------------------------------
// Batches

struct GeometryTuple
{
	size_t offset;
	size_t vertex_count;
	Transform3D transform;
};

struct GeometryBatch
{
	// utility
	// batch geometry loading
	u32 add_geometry(Mesh& mesh);
	u32 add_geometry(AnimatedMesh& mesh);
	u32 add_geometry(void* verts,size_t vsize,size_t ssize);
	void load();
	void update();
	void render();
	void vanish();

	// data
	VertexArray vao;
	VertexBuffer vbo;
	lptr<ShaderPipeline> shader;
	vector<GeometryTuple> objects;
	vector<ObjectInfo> data;
	vector<AnimatedMesh*> anim_meshes;
	vector<f32> geometry;
	vector<UniformBufferObject> ubos;
	void* pcm;
	//vector<u32> elements;
	u32 geometry_cursor = 0;
	u32 element_cursor = 0;
	u32 offset_cursor = 0;
};
// TODO utilize an element buffer later, careful when restructuring! anim geometry is straightforward though
// TODO cross-loading animations & static meshes into one geometry batch is nonsensical, due to the missing
//		overlap in shader pipeline setup & demands. this can be ripped apart into two difference batch types

struct ParticleBatch
{
	// utility
	void load(Mesh& mesh,u32 particles,size_t isize);
	void load(void* verts,size_t vsize,size_t ssize,u32 particles,size_t isize);
	void update();
	void render();
	void vanish();

	// data
	VertexArray vao;
	VertexBuffer vbo;
	VertexBuffer ibo;
	lptr<ShaderPipeline> shader;
	vector<f32> geometry;
	void* imem;
	UniformBufferObject ubo;
	void* pcm;
	u32 vertex_count;
	u32 active_particles = 0;
};

struct ShadowGeometryBatch
{
	void render();
	lptr<GeometryBatch> batch;
	lptr<ShaderPipeline> shader;
	vector<ShaderUniformUpload> uniform;
	// FIXME strange approach to use uniform like this. maybe find an equivalent to work with both versions
};

struct ShadowParticleBatch
{
	void render();
	lptr<ParticleBatch> batch;
	lptr<ShaderPipeline> shader;
};


// ----------------------------------------------------------------------------------------------------
// Renderer

class Renderer
{
public:
	Renderer();
	void precalculate();
	void update();
	void vanish();

	// sprite
	Rect* register_sprite_texture(const char* path);
	Sprite* register_sprite(Rect* texture,vec3 position,vec2 size,f32 rotation=.0f,
							f32 alpha=1.f,Alignment alignment={});
	void assign_sprite_texture(Sprite* sprite,Rect* texture);
	void delete_sprite_texture(Rect* texture);
	static void delete_sprite(Sprite* sprite);

	// text
	Font* register_font(const char* path,u16 size);
	lptr<Text> write_text(Font* font,string data,vec3 position,f32 scale,vec4 colour=vec4(1),Alignment align={});
	void delete_text(lptr<Text> text);

	// textures
	Texture* register_texture(vec3 colour,TextureFormat format=TEXTURE_FORMAT_RGBA);
	Texture* register_texture(const char* path,TextureFormat format=TEXTURE_FORMAT_RGBA);
	Material* register_material(Texture* albedo,Texture* normals=nullptr,Texture* materials=nullptr,
									 Texture* emission=nullptr);

	// scene
	lptr<ShaderPipeline> register_pipeline(TargetResolution tr,const char* vs,const char* fs,
										   PipelineBufferFormats& format,u32 feat);
	lptr<ShaderPipeline> register_result_pipeline(const char* vs,const char* fs,u32 feat);
	lptr<GeometryBatch> register_geometry_batch(lptr<ShaderPipeline> pipeline);
	lptr<ParticleBatch> register_particle_batch(lptr<ShaderPipeline> pipeline);
	lptr<GeometryBatch> register_deferred_geometry_batch();
	lptr<GeometryBatch> register_deferred_geometry_batch(lptr<ShaderPipeline> pipeline);
	lptr<ParticleBatch> register_deferred_particle_batch(lptr<ShaderPipeline> pipeline);

	// shadow projection
	void register_shadow_batch(lptr<GeometryBatch> b);
	void register_shadow_batch(lptr<GeometryBatch> b,lptr<ShaderPipeline> pipeline);
	void register_shadow_batch(lptr<ParticleBatch> b,lptr<ShaderPipeline> pipeline);

	// lighting
	SunLight* add_sunlight(vec3 position,vec3 colour,f32 intensity,bool shadow_source=false);
	PointLight* add_pointlight(vec3 position,vec3 colour,f32 intensity,f32 constant,f32 linear,f32 quadratic);

	// animation
	void animate(AnimatedMesh* mesh);

private:
	// threaded texture actions
	static void _load_colour(Texture* texture,vec3 colour,TextureFormat format,
							 queue<TextureDataTuple>* data_queue,std::mutex* queue_mutex);
	static void _load_texture(Texture* texture,const char* path,TextureFormat format,
							  queue<TextureDataTuple>* data_queue,std::mutex* queue_mutex);

	// pipeline steps
	void _update_uniforms();
	void _update_sprites();
	void _update_text();
	void _update_target();
	void _gpu_upload();

private:
	// buffers
	VertexBuffer m_SpriteVertexBuffer;
	VertexBuffer m_SpriteInstanceBuffer;
	VertexBuffer m_TextInstanceBuffer;
	VertexBuffer m_TargetVertexBuffer;

	// vertex arrays
	VertexArray m_SpriteVertexArray;
	VertexArray m_TextVertexArray;
	VertexArray m_TargetVertexArray;

	// targets
	vector<Framebuffer> m_ResultBuffers;
	Framebuffer m_Framebuffer;  // FIXME naming!!!!
	Framebuffer m_GBuffer;
	Framebuffer m_ShadowMap;

	// textures
	GPUPixelBuffer m_GPUSpriteTextures;
	GPUPixelBuffer m_GPUFontTextures;

	// mesh textures
	InPlaceArray<Texture> m_MeshTextures = InPlaceArray<Texture>(RENDERER_MAXIMUM_TEXTURE_COUNT);
	InPlaceArray<Material> m_Materials = InPlaceArray<Material>(RENDERER_MAXIMUM_MATERIAL_COUNT);
	queue<TextureDataTuple> m_MeshTextureUploadQueue;
	list<TextureAttachment> m_TextureAttachmentRequests;
	std::mutex m_MutexMeshTextureUpload;

	// sprites
	InPlaceArray<Sprite> m_Sprites;
	DescriptorSetMemory m_GlobalUBO;
	UniformBufferObject m_SpriteUBO,m_TextUBO;

	// text
	InPlaceArray<Font> m_Fonts = InPlaceArray<Font>(RENDERER_MAXIMUM_FONT_COUNT);
	list<Text> m_Texts;
	// TODO maybe convert some of those lists to maps, so the cross script interaction & performance is enhanced.
	//		generally get rid of the linked lists?

	// animation
	vector<AnimatedMesh*> m_AnimatingMeshes;

	// pipelines
	lptr<ShaderPipeline> m_SpritePipeline;
	lptr<ShaderPipeline> m_TextPipeline;
	lptr<ShaderPipeline> m_TargetPipeline;
	lptr<ShaderPipeline> m_GeometryPassPipeline;
	lptr<ShaderPipeline> m_ShadowPassPipeline;
	list<ShaderPipeline> m_ShaderPipelines;

	// batches
	list<GeometryBatch> m_GeometryBatches;
	list<ParticleBatch> m_ParticleBatches;
	list<GeometryBatch> m_DeferredGeometryBatches;
	list<ParticleBatch> m_DeferredParticleBatches;

	// uniform buffer
	DescriptorSetMemory m_GlobalDescriptorSet;
	UniformBufferMemory m_UBufferMem;
	size_t m_GlobalSetOffset;

	// lighting
	Camera3D m_ShadowProjection;
	list<ShadowGeometryBatch> m_ShadowGeometryBatches;
	list<ShadowParticleBatch> m_ShadowParticleBatches;
	// TODO this will be setup with lights in the future, just to test for now

	// access control
	friend struct UniformBufferObject;
};

inline Renderer g_Renderer = Renderer();


#endif
