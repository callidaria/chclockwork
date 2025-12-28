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
