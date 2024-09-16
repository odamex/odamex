#include "odamex.h"

#include "state.h"
#include "doom_obj_container.h"

void D_ResetState(state_t* s, statenum_t idx);
DoomObjectContainer<state_t, statenum_t> states(::NUMSTATES, &D_ResetState);

size_t num_state_t_types()
{
	return states.size();
}

void D_ResetState(state_t* s, statenum_t idx)
{
    s->sprite = SPR_TNT1;
    s->tics = -1;
    s->nextstate = (statenum_t) idx;
}

void D_Initialize_States(state_t* source, int count, statenum_t start)
{
	states.clear();
    if (source) {
		statenum_t idx = start;
        for(int i = 0; i < count; i++)
        {
			states.insert(source[i], idx);
			idx = statenum_t(idx + 1);
        }
    }
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Initialize_states:: allocated %d states.\n", count);
#endif
}
void D_EnsureStateCapacity(int limit)
{
	int newSize = states.size();
	while (limit >= newSize)
    {
		newSize *= 2;
    }   
    states.resize(newSize);
}
