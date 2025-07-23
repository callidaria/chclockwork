#ifndef CORE_RENDERER_HEADER
#define CORE_RENDERER_HEADER


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
	SCREEN_ALIGN_TOPLEFT,
	SCREEN_ALIGN_CENTERLEFT,
	SCREEN_ALIGN_BOTTOMLEFT,
	SCREEN_ALIGN_TOPCENTER,
	SCREEN_ALIGN_CENTER,
	SCREEN_ALIGN_BOTTOMCENTER,
	SCREEN_ALIGN_TOPRIGHT,
	SCREEN_ALIGN_CENTERRIGHT,
	SCREEN_ALIGN_BOTTOMRIGHT,
	SCREEN_ALIGN_NEUTRAL,
};

struct Alignment
{
	Rect border = { vec2(0),vec2(MATH_CARTESIAN_XRANGE,MATH_CARTESIAN_YRANGE) };
	ScreenAlignment align = SCREEN_ALIGN_NEUTRAL;
};


// ----------------------------------------------------------------------------------------------------
// GPU Data Structures

struct Sprite
{
	vec3 offset = vec3(0);
	vec2 scale = vec2(0);
	f32 rotation = .0f;
	f32 alpha = 1.f;
	vec2 tex_position;
	vec2 tex_dimension;
};

struct TextCharacter
{
	vec3 offset = vec3(0);
	vec2 scale = vec2(0);
	vec2 bearing = vec2(0);
	vec4 colour = vec4(1);
	PixelBufferComponent comp;
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

public:
	vector<Vertex> vertices;
};
// TODO element draw requires an element array instead of raw vertex store

struct MeshJoint
{
	string id;
	string uniform_location;
	mat4 offset;
	mat4 transform = mat4(1.f);
	mat4 recursive_transform = mat4(1.f);
	vector<u16> children;
};

struct AnimationJoint
{
	u16 id;
	u16 crr_position = 0;
	u16 crr_scale = 0;
	u16 crr_rotation = 0;
	vector<vec3> position_keys;
	vector<vec3> scaling_keys;
	vector<quat> rotation_keys;
	vector<f64> position_durations;  // TODO maybe join duration & keys?
	vector<f64> scaling_durations;
	vector<f64> rotation_durations;
};

struct Animation
{
	vector<AnimationJoint> joints;
	f64 duration;
};

class AnimatedMesh
{
public:
	AnimatedMesh(const char* path);

public:
	vector<AnimationVertex> vertices;
	vector<u32> elements;
	vector<MeshJoint> joints;
	vector<Animation> animations;
	u16 index_count = 0;
	u16 current_animation = 0;
	f64 progress = .0;
};
// TODO do not load this into it's own animation mesh, rather than extract animations and attach to mesh later
//		then again this can be done later, the related mesh probably will always be attached (weighing)


// ----------------------------------------------------------------------------------------------------
// Batches

struct TextureDataTuple
{
	TextureData data;
	Texture* texture;
};

struct GeometryUniformUpload
{
	u32 uloc;
	UniformDimension udim;
	f32* data;
};

struct GeometryTuple
{
	size_t offset;
	size_t vertex_count;
	Transform3D transform;
	vector<Texture*> textures;
	vector<GeometryUniformUpload> uploads;
	f32 texel = 1.f;
};

struct GeometryBatch
{
	// utility
	// batch geometry loading
	u32 add_geometry(Mesh& mesh,vector<Texture*>& tex);
	u32 add_geometry(AnimatedMesh& mesh,vector<Texture*>& tex);
	u32 add_geometry(void* verts,size_t vsize,size_t ssize,vector<Texture*>& tex);
	u32 add_geometry(void* verts,size_t vsize,size_t ssize,vector<u32>& elems,vector<Texture*>& tex);
	void load();

	// auto uniform upload
	void attach_uniform(u32 gid,const char* name,f32* var);
	void attach_uniform(u32 gid,const char* name,vec2* var);
	void attach_uniform(u32 gid,const char* name,vec3* var);
	void attach_uniform(u32 gid,const char* name,vec4* var);
	void attach_uniform(u32 gid,const char* name,mat4* var);

	// data
	VertexArray vao;
	VertexBuffer vbo;
	VertexBuffer ebo;
	lptr<ShaderPipeline> shader;
	vector<GeometryTuple> object;
	vector<float> geometry;
	vector<u32> elements;
	u32 geometry_cursor = 0;
	u32 element_cursor = 0;
	u32 offset_cursor = 0;
};
// TODO element buffer test with simple mesh geometry

struct ParticleBatch
{
	// utility
	void load(Mesh& mesh,u32 particles);
	void load(void* verts,size_t vsize,size_t ssize,u32 particles);

	// data
	VertexArray vao;
	VertexBuffer vbo;
	VertexBuffer ibo;
	lptr<ShaderPipeline> shader;
	vector<float> geometry;
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
};


// ----------------------------------------------------------------------------------------------------
// Renderer Component

class Renderer
{
public:
	Renderer();

	void update();
	void exit();

	// sprite
	PixelBufferComponent* register_sprite_texture(const char* path);
	Sprite* register_sprite(PixelBufferComponent* texture,vec3 position,vec2 size,f32 rotation=.0f,
							f32 alpha=1.f,Alignment alignment={});
	void assign_sprite_texture(Sprite* sprite,PixelBufferComponent* texture);
	void delete_sprite_texture(PixelBufferComponent* texture);
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
	void register_shadow_batch(lptr<ParticleBatch> b);

	// lighting
	SunLight* add_sunlight(vec3 position,vec3 colour,f32 intensity);
	PointLight* add_pointlight(vec3 position,vec3 colour,f32 intensity,f32 constant,f32 linear,f32 quadratic);
	void add_shadow(vec3 source);
	void upload_lighting();
	void reset_lighting();

	// utility
	static vec2 align(Rect geom,Alignment alignment);

private:

	// pipeline steps
	void _update_sprites();
	void _update_text();
	void _update_canvas();
	static void _update_mesh(list<GeometryBatch>& gb,list<ParticleBatch>& pb);
	void _update_shadows(list<lptr<GeometryBatch>>& gb,list<lptr<ParticleBatch>>& pb);
	void _gpu_upload();

	// background procedures
	template<typename T> static void _collector(InPlaceArray<T>* xs,ThreadSignal* signal);

private:

	// ----------------------------------------------------------------------------------------------------
	// Runtime Profiler
#ifdef DEBUG
	RuntimeProfilerData m_ProfilerFullFrame = PROF_CRT("full frametime");
#endif
	std::chrono::steady_clock::time_point m_FrameStart;

	// ----------------------------------------------------------------------------------------------------
	// Threading

	thread m_SpriteCollector;
	thread m_SpriteTextureCollector;

	// ----------------------------------------------------------------------------------------------------
	// Data Management & Pipelines

	VertexArray m_SpriteVertexArray;
	VertexArray m_TextVertexArray;
	VertexArray m_CanvasVertexArray;

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
	list<ShaderPipeline> m_ShaderPipelines;
	list<GeometryBatch> m_GeometryBatches;
	list<ParticleBatch> m_ParticleBatches;
	list<GeometryBatch> m_DeferredGeometryBatches;
	list<ParticleBatch> m_DeferredParticleBatches;
	list<lptr<GeometryBatch>> m_ShadowGeometryBatches;
	list<lptr<ParticleBatch>> m_ShadowParticleBatches;

	// lighting
	lptr<ShaderPipeline> m_GeometryPassPipeline;
	lptr<ShaderPipeline> m_ParticlePassPipeline;
	lptr<ShaderPipeline> m_GeometryShadowPipeline;
	lptr<ShaderPipeline> m_ParticleShadowPipeline;
	Lighting m_Lighting;
};

inline Renderer g_Renderer = Renderer();


#endif
