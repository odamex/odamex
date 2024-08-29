#include "odamex.h"

#include "mobjinfo.h"

int num_mobjinfo_types;
mobjinfo_t* mobjinfo;

static void D_ResetMobjInfo(int from, int to)
{
    mobjinfo_t *m;
    for(int i = from; i < to; i++)
    {
        m = &mobjinfo[i];
        m->droppeditem = MT_NULL;
        m->infighting_group = IG_DEFAULT;
        m->projectile_group = PG_DEFAULT;
        m->splash_group = SG_DEFAULT;
        m->altspeed = NO_ALTSPEED;
        m->meleerange = MELEERANGE;
		m->translucency = 0x10000;
    }
}

void D_Initialize_Mobjinfo(mobjinfo_t* source, int count) {
	if (mobjinfo != nullptr)
	{
		free(mobjinfo);
		mobjinfo = nullptr;
	}
    mobjinfo = (mobjinfo_t*) M_Calloc (count, sizeof(*mobjinfo));
	if (source)
	{
		for (int i = 0; i < count; i++)
		{
			mobjinfo[i] = source[i];
		}
		num_mobjinfo_types = count;
	}
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Allocate_mobjinfo:: allocated %d actors.\n", count);
#endif
}

void D_EnsureMobjInfoCapacity(int limit)
{
	int old_num_mobjinfo_types = num_mobjinfo_types;
	while (limit >= num_mobjinfo_types)
	{
		num_mobjinfo_types *= 2;
	}
    
	if (old_num_mobjinfo_types < num_mobjinfo_types)
	{
		mobjinfo =
		    (mobjinfo_t*)M_Realloc(mobjinfo, num_mobjinfo_types * sizeof(*mobjinfo));
        // dsdhacked spec says anything not set to a default should be 0/null
		memset(mobjinfo + old_num_mobjinfo_types, 0,
		       (num_mobjinfo_types - old_num_mobjinfo_types) * sizeof(*mobjinfo));
        // Reset mobjinfo structs according to DSDHacked spec
        D_ResetMobjInfo(old_num_mobjinfo_types, num_mobjinfo_types);
    }
}
