#include "odamex.h"

#include "mobjinfo.h"

int num_mobjinfo_types;
mobjinfo_t* mobjinfo;

static void D_ResetMobjInfo(int from, int to)
{
    mobjinfo_t m;
    for(int i = 0; i < to; i++)
    {
        m = mobjinfo[i];
        m.droppeditem = MT_NULL;
        m.infighting_group = IG_DEFAULT;
        m.projectile_group = PG_DEFAULT;
        m.splash_group = SG_DEFAULT;
        m.altspeed = NO_ALTSPEED;
        m.meleerange = MELEERANGE; // MELEERANGE
    }
}

void D_Initialize_mobjinfo(mobjinfo_t* source, int count) {
    // [CMB] Pre-allocate mobjinfo to support current limit on types
    mobjinfo = (mobjinfo_t*) M_Calloc (count, sizeof(*mobjinfo));
    for(int i = 0; i < count; i++)
    {
        mobjinfo[i] = doom_mobjinfo[i];
    }
    num_mobjinfo_types = count;
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Allocate_mobjinfo:: allocated %d actors.\n", limit);
#endif
}

void D_EnsureMobjInfoCapacity(int limit)
{
    while(limit >= ::num_mobjinfo_types)
    {
        int old_num_mobjinfo_types = ::num_mobjinfo_types;
        ::num_mobjinfo_types *= 2;
        mobjinfo = (mobjinfo_t*) M_Realloc(mobjinfo, ::num_spritenum_t_types * sizeof(*mobjinfo));
        // Reset mobjinfo structs according to DSDHacked spec
        D_ResetMobjInfo(old_num_mobjinfo_types, ::num_mobjinfo_types);
    }
}
