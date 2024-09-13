#include "odamex.h"

#include "mobjinfo.h"
#include "odamex_objects.h"

//----------------------------------------------------------------------------------------------
template <>
DoomObjectContainer<mobjinfo_t, mobjtype_t>::~DoomObjectContainer()
{
	if (this->container != nullptr)
	{
		M_Free_Ref(this->container);
	}
}
//----------------------------------------------------------------------------------------------

void D_ResetMobjInfo(int from, int to);
DoomObjectContainer<mobjinfo_t, mobjtype_t> mobjinfo(::NUMMOBJTYPES, &D_ResetMobjInfo);
size_t num_mobjinfo_types()
{
	return mobjinfo.capacity();
}


void D_ResetMobjInfo(int from, int to)
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
	int newCapacity = mobjinfo.capacity();
	while (limit >= newCapacity)
	{
		newCapacity *= 2;
	}
	mobjinfo.resize(newCapacity);
}
