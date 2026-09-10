#ifndef CORE_RENDERER_HEADER
#define CORE_RENDERER_HEADER


#include <numeric>
#include "gpu_interface.h"
#include "buffer.h"
#include "shader.h"


constexpr f32 RENDERER_POSITIONAL_DELETION_CODE = -1247.f;
constexpr u8 RENDERER_ANIMATION_INFLUENCE_RANGE = 4;

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
// States

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
// Entity Data

struct Text
{
	// utility
	void align();
	void load_buffer();
	u32 intersection(f32 pos);

	// data
	Font* font;
	vec3 position;
	vec2 offset;
	f32 scale;
	vec2 dimensions;
	vec4 colour;
	Alignment alignment;
	string data;
	vector<TextCharacter> buffer;
};

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
	string uniform_location;

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
	mat4 recursive_transform = mat4(1.f);

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
// Batches

struct PBGMaterials
{
	mat4 model = mat4(1.f);
	f32 texel = 1.f;
	u32 colour;
	u32 normal;
	u32 material;
	u32 emission;
};

struct TextureDataTuple
{
	TextureData data;
	GPUPixelBuffer* texture;
};

struct GeometryTuple
{
	size_t offset;
	size_t vertex_count;
	Transform3D transform;
	vector<Texture*> textures;
	ShaderUniformUpload uniform;
	f32 texel = 1.f;
};

struct GeometryBatch
{
	// utility
	// batch geometry loading
	u32 add_geometry(Mesh& mesh,const vector<Texture*>& tex);
	u32 add_geometry(AnimatedMesh& mesh,const vector<Texture*>& tex);
	u32 add_geometry(void* verts,size_t vsize,size_t ssize,const vector<Texture*>& tex);
	void load();
	void vanish();

	// data
	VertexArray vao;
	VertexBuffer vbo;
	lptr<ShaderPipeline> shader;
	vector<GeometryTuple> objects;
	vector<AnimatedMesh*> anim_meshes;
	vector<f32> geometry;
	void* pcm;
	//vector<u32> elements;
	u32 geometry_cursor = 0;
	u32 element_cursor = 0;
	u32 offset_cursor = 0;
};
// TODO utilize an element buffer later, careful when restructuring! anim geometry is straightforward though

struct ParticleBatch
{
	// utility
	void load(Mesh& mesh,u32 particles,size_t isize);
	void load(void* verts,size_t vsize,size_t ssize,u32 particles,size_t isize);
	void vanish();

	// data
	VertexArray vao;
	VertexBuffer vbo;
	VertexBuffer ibo;
	lptr<ShaderPipeline> shader;
	vector<f32> geometry;
	void* pcm;
	u32 vertex_count;
	u32 active_particles = 0;
};


// ----------------------------------------------------------------------------------------------------
// Lighting

struct SunLight
{
	vec3 position;
	vec3 colour;
};

struct PointLight
{
	vec3 position;
	vec3 colour;
	f32 constant;
	f32 linear;
	f32 quadratic;
};

struct Lighting
{
	SunLight sunlights[8];
	PointLight pointlights[64];
	u8 sunlights_active = 0;
	u8 pointlights_active = 0;
	Camera3D shadow_projection;
	bool shadow_forced = false;
};

struct ShadowGeometryBatch
{
	lptr<GeometryBatch> batch;
	lptr<ShaderPipeline> shader;
	vector<ShaderUniformUpload> uniform;
};

struct ShadowParticleBatch
{
	lptr<ParticleBatch> batch;
	lptr<ShaderPipeline> shader;
};


#ifdef VKBUILD

class Renderer
{
public:
	Renderer();
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
	inline void delete_text(lptr<Text> text) { m_Texts.erase(text); }

	// textures
	GPUPixelBuffer* register_texture(const char* path,TextureFormat format=TEXTURE_FORMAT_RGBA);

	// scene
	lptr<ShaderPipeline> register_pipeline(const char* vs,const char* fs,u8 bfr_count,bool depth=false);
	lptr<GeometryBatch> register_geometry_batch(lptr<ShaderPipeline> pipeline);
	lptr<ParticleBatch> register_particle_batch(lptr<ShaderPipeline> pipeline);
	lptr<GeometryBatch> register_deferred_geometry_batch();
	lptr<GeometryBatch> register_deferred_geometry_batch(lptr<ShaderPipeline> pipeline);
	//lptr<ParticleBatch> register_deferred_particle_batch();
	lptr<ParticleBatch> register_deferred_particle_batch(lptr<ShaderPipeline> pipeline);

private:
	void _update_sprites();
	void _update_text();
	static void _update_mesh(list<GeometryBatch>& batches);
	static void _update_particles(list<ParticleBatch>& batches);
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
	vector<Framebuffer> m_ResultBuffers = vector<Framebuffer>(g_Frame.result_image_views.size());
	Framebuffer m_Framebuffer;  // FIXME naming!!!!
	Framebuffer m_GBuffer;

	// textures
	GPUPixelBuffer m_GPUSpriteTextures;
	GPUPixelBuffer m_GPUFontTextures;

	// mesh textures
	InPlaceArray<GPUPixelBuffer> m_MeshTextures = InPlaceArray<GPUPixelBuffer>(RENDERER_MAXIMUM_TEXTURE_COUNT);
	queue<TextureDataTuple> m_MeshTextureUploadQueue;
	std::mutex m_MutexMeshTextureUpload;

	// sprites
	InPlaceArray<Sprite> m_Sprites = InPlaceArray<Sprite>(RENDERER_MAXIMUM_SPRITE_COUNT);

	// text
	InPlaceArray<Font> m_Fonts = InPlaceArray<Font>(RENDERER_MAXIMUM_FONT_COUNT);
	list<Text> m_Texts;
	size_t m_CharCount = 0;
	// FIXME font memory is too strict and i don't think this is a nice approach in this case

	// pipelines
	ShaderPipeline m_SpritePipeline = ShaderPipeline(1,true);
	ShaderPipeline m_TextPipeline = ShaderPipeline(1,true);
	ShaderPipeline m_TargetPipeline = ShaderPipeline(1,true);
	lptr<ShaderPipeline> m_GeometryPassPipeline;
	//lptr<ShaderPipeline> m_ParticlePassPipeline;
	list<ShaderPipeline> m_ShaderPipelines;

	// batches
	list<GeometryBatch> m_GeometryBatches;
	list<ParticleBatch> m_ParticleBatches;
	list<GeometryBatch> m_DeferredGeometryBatches;
	list<ParticleBatch> m_DeferredParticleBatches;

	// uniform buffer
	UniformBufferMemory m_UBufferMem;
};


// TODO light structures, except for shadow projections are universal and belong outside gfxapi related stuff
#else


// ----------------------------------------------------------------------------------------------------
// Renderer Component

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
	inline void delete_text(lptr<Text> text) { m_Texts.erase(text); }

	// textures
	Texture* register_texture(const char* path,TextureFormat format=TEXTURE_FORMAT_RGBA);

	// scene
	lptr<ShaderPipeline> register_pipeline(VertexShader& vs,FragmentShader& fs);
	lptr<GeometryBatch> register_geometry_batch(lptr<ShaderPipeline> pipeline);
	lptr<GeometryBatch> register_deferred_geometry_batch();
	lptr<GeometryBatch> register_deferred_geometry_batch(lptr<ShaderPipeline> pipeline);
	lptr<ParticleBatch> register_particle_batch(lptr<ShaderPipeline> pipeline);
	lptr<ParticleBatch> register_deferred_particle_batch();
	lptr<ParticleBatch> register_deferred_particle_batch(lptr<ShaderPipeline> pipeline);

	// shadow projection
	void register_shadow_batch(lptr<GeometryBatch> b);
	void register_shadow_batch(lptr<GeometryBatch> b,lptr<ShaderPipeline> pipeline);
	void register_shadow_batch(lptr<ParticleBatch> b);
	void register_shadow_batch(lptr<ParticleBatch> b,lptr<ShaderPipeline> pipeline);

	// lighting
	SunLight* add_sunlight(vec3 position,vec3 colour,f32 intensity);
	PointLight* add_pointlight(vec3 position,vec3 colour,f32 intensity,f32 constant,f32 linear,f32 quadratic);
	void add_shadow(vec3 source,bool forced=true);
	void upload_lighting();
	void reset_lighting();

	// utility
	void animate(AnimatedMesh* mesh);

private:

	// pipeline steps
	void _update_sprites();
	void _update_text();
	void _update_canvas();
	static void _update_mesh(list<GeometryBatch>& gb,list<ParticleBatch>& pb);
	void _update_shadows(list<ShadowGeometryBatch>& gb,list<ShadowParticleBatch>& pb);
	void _gpu_upload();

	// background procedures
	template<typename T> static void _collector(InPlaceArray<T>* xs,ThreadSignal* signal);

private:

	// ----------------------------------------------------------------------------------------------------
	// Runtime Profiler
#ifdef DEBUG
	RuntimeProfilerData m_ProfilerFullFrame = PROF_CRT("full frametime");
#endif

	// ----------------------------------------------------------------------------------------------------
	// Threading

	thread m_SpriteCollector;
	thread m_SpriteTextureCollector;

	// ----------------------------------------------------------------------------------------------------
	// Data Management & Pipelines

	VertexBuffer m_SpriteVertexBuffer;
	VertexBuffer m_CanvasVertexBuffer;

	VertexBuffer m_SpriteInstanceBuffer;
	VertexBuffer m_TextInstanceBuffer;

	ShaderPipeline m_SpritePipeline;
	ShaderPipeline m_TextPipeline;
	ShaderPipeline m_CanvasPipeline;

	Framebuffer m_ForwardFrameBuffer = Framebuffer(1);
	Framebuffer m_DeferredFrameBuffer = Framebuffer(5);
	Framebuffer m_ShadowFrameBuffer = Framebuffer(0);

	// ----------------------------------------------------------------------------------------------------
	// Render Object Information

	// textures
	GPUPixelBuffer m_GPUSpriteTextures;
	GPUPixelBuffer m_GPUFontTextures;

	// mesh textures
	InPlaceArray<Texture> m_MeshTextures = InPlaceArray<Texture>(RENDERER_MAXIMUM_TEXTURE_COUNT);
	queue<TextureDataTuple> m_MeshTextureUploadQueue;
	std::mutex m_MutexMeshTextureUpload;

	// sprites
	InPlaceArray<Sprite> m_Sprites = InPlaceArray<Sprite>(BUFFER_MAXIMUM_TEXTURE_COUNT);

	// text
	InPlaceArray<Font> m_Fonts = InPlaceArray<Font>(RENDERER_MAXIMUM_FONT_COUNT);
	list<Text> m_Texts;
	// FIXME font memory is too strict and i don't think this is a nice approach in this case

	// mesh
	list<ShadowGeometryBatch> m_ShadowGeometryBatches;
	list<ShadowParticleBatch> m_ShadowParticleBatches;

	// animation
	vector<AnimatedMesh*> m_AnimatingMeshes;
	// FIXME figure out what happens to deleted animated meshes that are linked here and in geometry batch

	// lighting
	lptr<ShaderPipeline> m_GeometryShadowPipeline;
	lptr<ShaderPipeline> m_ParticleShadowPipeline;
	Lighting m_Lighting;
};

#endif

inline Renderer g_Renderer = Renderer();


#endif
