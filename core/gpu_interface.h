#ifndef CORE_GPUINTERFACE_HEADER
#define CORE_GPUINTERFACE_HEADER


#include "base.h"


// ----------------------------------------------------------------------------------------------------
// Basic Structures

struct SunLight
{
	vec3 position __attribute__((aligned(16))) = vec3(0);
	vec3 colour __attribute__((aligned(16))) = vec3(0);
};

struct PointLight
{
	vec3 position __attribute__((aligned(16))) = vec3(0);
	vec3 colour __attribute__((aligned(16))) = vec3(0);
	f32 constant = 0;
	f32 linear = 0;
	f32 quadratic = 0;
};
// TODO maybe outsource into its own special lighting component


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer Globals

struct SpriteTransformation
{
	mat4 view;
	mat4 proj;
};

struct ObjectTransformation
{
	mat4 view;
	mat4 proj;
};

struct CameraAttributes
{
	vec3 position;
	f32 exposure = 1.f;
	f32 gamma = 1.f/2.2f;
} __attribute__((aligned(64)));

struct Lighting
{
	SunLight sunlights[8] __attribute__((aligned(64)));
	PointLight pointlights[64] __attribute__((aligned(64)));
	u32 sunlights_active = 0;
	u32 pointlights_active = 0;
} __attribute__((aligned(64)));

struct OrthographicShadow
{
	mat4 view;
	mat4 proj;
	vec3 source __attribute__((aligned(16))) = vec3(0);
};

struct UniformBufferMemory
{
	SpriteTransformation strafo;
	ObjectTransformation otrafo;
	CameraAttributes camera;
	Lighting lighting;
	OrthographicShadow orth_shadow;
} __attribute__((aligned(64)));
// TODO it should be possible to save quite some memory here!
//		though it is advisable to respect the alignment conformity, given by the shader data layout!


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer Object Locals

struct ObjectInfo
{
	mat4 model = mat4(1.f);
	f32 texel = 1.f;
} __attribute__((aligned(64)));

struct ObjectMemory
{
	ObjectInfo objinfo;
} __attribute__((aligned(64)));


#endif
