#ifndef CORE_GPUINTERFACE_HEADER
#define CORE_GPUINTERFACE_HEADER


#include "base.h"


// ----------------------------------------------------------------------------------------------------
// Basic Structures

struct SunLight
{
	vec3 position;
	vec3 colour;
} __attribute__((aligned(64)));

struct PointLight
{
	vec3 position;
	vec3 colour;
	f32 constant;
	f32 linear;
	f32 quadratic;
} __attribute__((aligned(64)));
// TODO maybe outsource into its own special lighting component


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

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

struct UniformBufferMemory
{
	SpriteTransformation strafo;
	ObjectTransformation otrafo;
	CameraAttributes camera;
	Lighting lighting;
} __attribute__((aligned(64)));
// TODO it should be possible to save quite some memory here!

constexpr size_t INTERFACE_UNIFORM_BUFFER_MEMSIZE = sizeof(UniformBufferMemory);
//= (sizeof(UniformBufferMemory)+(size_t)0x111111)&(size_t)0x111111;


#endif
