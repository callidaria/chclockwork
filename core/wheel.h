#ifndef CORE_WHEEL_HEADER
#define CORE_WHEEL_HEADER


#include "base.h"


struct UpdateRoutine
{
	void (*update)(void*);
	void* memory;
	template<typename T> static inline void _update(void* mem)
	{
		T* p = (T*)p; p->update();
	}
};

struct Wheel
{
	// utility
	template<typename T> inline lptr<UpdateRoutine> call(T* mem)
	{
		routines.push_back(UpdateRoutine { &UpdateRoutine::template _update<T>,(void*)mem });
		return std::prev(routines.end());
	}
	void update();

	// data
	list<UpdateRoutine> routines;
};

inline Wheel g_Wheel;


#endif
