#ifndef CORE_WHEEL_HEADER
#define CORE_WHEEL_HEADER


#include "base.h"


struct UpdateRoutine
{
	void (*update)(void*);
	void* memory;
};

struct Wheel
{
	// utility
	lptr<UpdateRoutine> call(UpdateRoutine routine);
	void update();

	// data
	list<UpdateRoutine> routines;
};

inline Wheel g_Wheel;


#endif
