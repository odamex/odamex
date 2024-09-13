#include "odamex.h"

#include "state.h"
#include "odamex_objects.h"

//----------------------------------------------------------------------------------------------
template<>
DoomObjectContainer<state_t, statenum_t>::~DoomObjectContainer()
{
	M_Free_Ref(this->container);
}
//----------------------------------------------------------------------------------------------

static void D_ResetStates(int from, int to);
DoomObjectContainer<state_t, statenum_t> states(::NUMSTATES, &D_ResetStates);

size_t num_state_t_types()
{
	return states.capacity();
}

void D_ResetStates(int from, int to)
{
    state_t *s;
    for(int i = from; i < to; i++)
    {
        s = &states[i];
        s->sprite = SPR_TNT1;
        s->tics = -1;
        s->nextstate = (statenum_t) i;
    }
}

void D_Initialize_States(state_t* source, int count)
{
	states.clear();
	states.resize(count);
    if (source) {
        for(int i = 0; i < count; i++)
        {
            states[i] = source[i];
        }
    }
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Initialize_states:: allocated %d states.\n", count);
#endif
}
void D_EnsureStateCapacity(int limit)
{
	int newCapacity = states.capacity();
    while (limit >= newCapacity)
    {
		newCapacity *= 2;
    }
	states.resize(newCapacity);
}
