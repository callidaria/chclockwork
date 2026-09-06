#ifndef CORE_GPUINTERFACE_HEADER
#define CORE_GPUINTERFACE_HEADER


#include "base.h"


// ----------------------------------------------------------------------------------------------------
// Uniform Buffer

struct ObjectTransformation
{
	mat4 view __attribute__((aligned(16)));
	mat4 proj __attribute__((aligned(16)));
};

struct SpriteTransformation
{
	mat4 view __attribute__((aligned(16)));
	mat4 proj __attribute__((aligned(16)));
};

struct UniformBufferMemory
{
	ObjectTransformation otrafo;
	SpriteTransformation strafo;
};


#endif
