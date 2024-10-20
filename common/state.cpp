#include "odamex.h"

#include "state.h"
#include "doom_obj_container.h"

void D_ResetState(state_t* s, int32_t idx);
DoomObjectContainer<state_t*> states(::NUMSTATES, &D_ResetState);

size_t num_state_t_types()
{
	return states.size();
}

void D_ResetState(state_t* s, int32_t idx)
{
    s->sprite = SPR_TNT1;
    s->frame = 0;
    s->tics = -1;
    // s->action = NULL;
    s->nextstate = (statenum_t) idx;
    s->misc1 = 0;
    s->misc2 = 0;

    // mbf21 flags
    s->flags = STATEF_NONE;
    s->args[0] = 0;
    s->args[1] = 0;
    s->args[2] = 0;
    s->args[3] = 0;
    s->args[4] = 0;
    s->args[5] = 0;
    s->args[6] = 0;
    s->args[7] = 0;
}

void D_Initialize_States(state_t* source, int count)
{
	states.clear();
    states.reserve(count);
    if (source) {

    	state_t state_source;
    	int32_t idx = source->statenum;
        for(int i = 0; i < count; i++)
        {
        	state_source = source[i];
        	state_t* newstate = (state_t*) M_Calloc(1, sizeof(state_t));
        	*newstate = state_source;
			states.insert(newstate, idx);
        	++idx;
        }
    }
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Initialize_states:: allocated %d states.\n", count);
#endif
}
