#ifndef CORE_WHEEL_HEADER
#define CORE_WHEEL_HEADER


#include "base.h"


struct UpdateRoutine
{
	// data
	void (*update)(void*);
	void (*end)(void*);
	void* memory;

	// logic
	template<typename T> static inline void _update(void* mem)
	{
		T* p = (T*)mem;
		p->update();
	}
	template<typename T> static inline void _stop(void* mem)
	{
		T* p = (T*)mem;
		p->vanish();
	}
};

struct Wheel
{
	// utility
	template<typename T> inline lptr<UpdateRoutine> call(T* mem)
	{
		routines.push_back(UpdateRoutine { &UpdateRoutine::_update<T>,&UpdateRoutine::_stop<T>,(void*)mem });
		return std::prev(routines.end());
	}
	void update();
	void stop(lptr<UpdateRoutine> routine);

	// data
	list<UpdateRoutine> routines;
};

inline Wheel g_Wheel;


#endif
