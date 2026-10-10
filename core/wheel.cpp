#include "wheel.h"


/**
 *	update registered routines
 */
void Wheel::update()
{
	lptr<UpdateRoutine> p_Routine = routines.begin();
	while (p_Routine!=routines.end())
	{
		lptr<UpdateRoutine> p_Next = std::next(p_Routine);
		p_Routine->update(p_Routine->memory);
		p_Routine = p_Next;
	}
}

/**
 *	stop & deregister previously registered routines
 *	\param routine: routine, that will be stopped and deregistered
 */
void Wheel::stop(lptr<UpdateRoutine> routine)
{
	routine->end(routine->memory);
	routines.erase(routine);
}
