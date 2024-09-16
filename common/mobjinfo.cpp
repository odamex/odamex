#include "odamex.h"

#include "mobjinfo.h"
#include "doom_obj_container.h"

void D_ResetMobjInfo(mobjinfo_t* m, mobjtype_t idx);
DoomObjectContainer<mobjinfo_t, mobjtype_t> mobjinfo(::NUMMOBJTYPES, &D_ResetMobjInfo);
size_t num_mobjinfo_types()
{
	return mobjinfo.size();
}


void D_ResetMobjInfo(mobjinfo_t* m, mobjtype_t idx)
{
    m->droppeditem = MT_NULL;
    m->infighting_group = IG_DEFAULT;
    m->projectile_group = PG_DEFAULT;
    m->splash_group = SG_DEFAULT;
    m->altspeed = NO_ALTSPEED;
    m->meleerange = MELEERANGE;
	m->translucency = 0x10000;
}

void D_Initialize_Mobjinfo(mobjinfo_t* source, int count) 
{
	mobjinfo.clear();
	mobjinfo.resize(count);
	if (source)
	{
		for (int i = 0; i < count; i++)
		{
			mobjinfo[i] = source[i];
		}
	}
#if defined _DEBUG
    Printf(PRINT_HIGH,"D_Allocate_mobjinfo:: allocated %d actors.\n", count);
#endif
}

void D_EnsureMobjInfoCapacity(int limit)
{
	int newSize = mobjinfo.size();
	while (limit >= newSize)
	{
		newSize *= 2;
	}
	mobjinfo.resize(newSize);
}
