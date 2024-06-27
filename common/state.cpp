#include "odamex.h"

#include "state.h"

state_t* states;
int num_state_t_types;

static void D_ResetStates(int from, int to)
{
    state_t s;
    for(int i = from; i < to; i++)
    {
        s = states[i];
        s.sprite = SPR_TNT1;
        s.tics = -1;
        s.nextstate = S_TNT1;
    }
}

void D_Initialize_states(state_t* source, int count)
{
    states = (state_t*) M_Calloc(count, sizeof(*states));
    // [CMB] TODO: for testing purposes only
    if(source) {
        for(int i = 0; i < count; i++)
        {
            states[i] = source[i];
        }
    }
    num_state_t_types = count;
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Initialize_states:: allocated %d states.\n", count);
#endif
}
void D_EnsureStateCapacity(int limit)
{
    while(limit > num_state_t_types)
    {
        int old_num_state_t_types = num_state_t_types;
        num_state_t_types *= 2;
        states = (state_t*) M_Realloc(states, num_state_t_types * sizeof(*states));
        D_ResetStates(old_num_state_t_types, num_state_t_types);
    }
}
