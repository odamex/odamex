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

void D_Initialize_Mobjinfo(mobjinfo_t* source, int count, mobjtype_t start)
{
	mobjinfo.clear();
	if (source)
	{
		mobjtype_t idx = start;
		for (int i = 0; i < count; i++)
		{
			mobjinfo.insert(source[i], idx);
			idx = mobjtype_t(i + 1);
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
