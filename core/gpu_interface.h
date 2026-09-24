#ifndef CORE_GPUINTERFACE_HEADER
#define CORE_GPUINTERFACE_HEADER


#include "base.h"


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

struct SpriteTransformation
{
	mat4 view __attribute__((aligned(16)));
	mat4 proj __attribute__((aligned(16)));
};

struct ObjectTransformation
{
	mat4 view __attribute__((aligned(16)));
	mat4 proj __attribute__((aligned(16)));
};

struct UniformBufferMemory
{
	SpriteTransformation strafo;
	ObjectTransformation otrafo;
};

constexpr size_t INTERFACE_COMBINED_GLOBAL_MEMSIZE = sizeof(UniformBufferMemory);


#endif
