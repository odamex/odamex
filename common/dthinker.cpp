// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	MapObj data. Map Objects or mobjs are actors, entities,
//	thinker, take-your-pick... anything that moves, acts, or
//	suffers state changes of more or less violent nature.
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include "dthinker.h"
#include "z_zone.h"
#include "stats.h"
#include "p_local.h"
#include "g_musinfo.h"

IMPLEMENT_SERIAL (DThinker, DObject)

std::vector<DThinker*> DThinker::s_thinkers;

std::vector<DThinker *> LingerDestroy;

void DThinker::Serialize (FArchive &arc)
{
	Super::Serialize (arc);

	// We do not serialize m_Next or m_Prev, because the DThinker
	// constructor handles them for us.
}

void DThinker::SerializeAll (FArchive &arc, bool hubLoad)
{
	if (arc.IsStoring ())
	{
		for (auto& thinker : s_thinkers)
		{
			if (!(arc.IsReset() && P_ThinkerIsPlayerType(thinker)))
			{
				arc << (BYTE)1;
				arc << thinker;
			}
		}
		arc << (BYTE)0;
	}
	else
	{
		if (hubLoad)
			DestroyMostThinkers ();
		else
			DestroyAllThinkers ();

		BYTE more;
		arc >> more;
		while (more)
		{
			DThinker *thinker;
			arc >> thinker;
			arc >> more;
		}

		// killough 3/26/98: Spawn icon landings:
		P_SpawnBrainTargets ();
	}
}

DThinker::DThinker ()
{
	s_thinkers.push_back(this);
	refCount = 0;
	destroyed = false;
}

DThinker::~DThinker ()
{
}

// This method is necessary if you construct the Thinker in an unconventional way,
// like via a copy ctor.  Otherwise, DThinker::Destroy() runs the risk of stomping
// all over the thinker list.
void DThinker::Orphan()
{
	refCount = 0;
}

void DThinker::Destroy()
{
	// denis - allow this function to be safely called multiple times
	if(destroyed)
		return;

	// In isolation, find-erase is almost always slower than unlinking a node from a linked list.
	// However, across the entire frame, the vector's iteration performance advantages ultimately
	// lead to slightly better performance overall.
	auto iterToThis = std::find(s_thinkers.begin(),
	                            s_thinkers.end(),
	                            this);

	DestroyFromContainer(iterToThis);
}

std::vector<DThinker*>::iterator DThinker::DestroyFromContainer (std::vector<DThinker*>::iterator iterToThis)
{
	if (iterToThis != s_thinkers.end())
	{
		DThinker *obj = *iterToThis;
		auto nextIter = s_thinkers.erase(iterToThis);

		obj->destroyed = true;

		if(obj->refCount)
		{
			LingerDestroy.push_back(obj); // something is still finding this pointer useful
		}
		else
			obj->Super::Destroy ();

		size_t l = LingerDestroy.size();
		for(size_t i = 0; i < l; i++)
		{
			obj = LingerDestroy[i];
			if(!obj->refCount)
			{
				obj->ObjectFlags |= OF_Cleanup;
				LingerDestroy.erase(LingerDestroy.begin() + i);
				l--; i--;
				delete obj;
			}
		}
		return nextIter;
	}
	return s_thinkers.end();
}

bool DThinker::WasDestroyed ()
{
	return destroyed;
}

// Destroy every thinker
void DThinker::DestroyAllThinkers ()
{
	while (not s_thinkers.empty())
    {
		// Suboptimal, but eh, this function probably doesn't need to be fast.
		DestroyFromContainer(s_thinkers.end() - 1);
	}
	DObject::EndFrame ();

	for (DThinker *obj : LingerDestroy)
	{
//		if(!obj->refCount)
		{
			obj->ObjectFlags |= OF_Cleanup;
			delete obj;
		}
	}
	LingerDestroy.clear();
}

// Destroy all thinkers except for player-controlled actors
void DThinker::DestroyMostThinkers ()
{
    auto isPlayerActor = [](DThinker* thinker) -> bool
    {
        return thinker->IsKindOf (RUNTIME_CLASS (AActor)) &&
            static_cast<AActor *>(thinker)->player     != nullptr &&
            static_cast<AActor *>(thinker)->player->mo != static_cast<AActor *>(thinker);
    };

    // stable_partition retains the relative order of elements with both partitions.
    auto lastToDestroy = std::stable_partition(s_thinkers.begin(),
                                               s_thinkers.end(),
                                               isPlayerActor);

    // Calling an erase() range would be nicest, but we have special stuff going on in Destroy
    // that normally runs between destructions.  A good refactor would be to allow that to work
    // with the normal destructors.
    //
    // So in the meantime, let's take advantage of the fact that erase() doesn't invalidate
    // preceding iterators.
    if (lastToDestroy != s_thinkers.end())
    {
        auto nextToDestroy = s_thinkers.end() - 1;
        while (nextToDestroy != lastToDestroy)
        {
            DestroyFromContainer(nextToDestroy);
            nextToDestroy = s_thinkers.end() - 1;       // Can't validly decrement here.
        }
        DestroyFromContainer(nextToDestroy);
    }
	DObject::EndFrame ();
}

//
// IndependentThinker
//
// Returns true if a DThinker object is ticked independently elsewhere.
// Returns false if it should be ticked in DThinker::RunThinkers
//
bool IndependentThinker(DThinker *thinker)
{
	// Only have independent thinkers in client/server mode
	if (!multiplayer || demoplayback)
		return false;

	if (thinker->IsKindOf (RUNTIME_CLASS (AActor)))
	{
		AActor *mobj = static_cast<AActor*>(thinker);
		if (!mobj->player || mobj->player->spectator)
			return false;

		// Clientside prediction takes care of ticking
		if (clientside)
			return true;

		// Server ticks players as it processes their ticcmds
		if (serverside)
			return true;
	}

	if (thinker->IsA(RUNTIME_CLASS (DPillar)) ||
		thinker->IsA(RUNTIME_CLASS (DElevator)) ||
		thinker->IsA(RUNTIME_CLASS (DFloor)) ||
		thinker->IsA(RUNTIME_CLASS (DCeiling)) ||
		thinker->IsA(RUNTIME_CLASS (DPlat)) ||
		thinker->IsA(RUNTIME_CLASS (DDoor)))
	{
		// Client ticks movable sectors in prediction code
		if (clientside)
			return true;
	}

	return false;
}


void DThinker::RunThinkers ()
{
	BEGIN_STAT (ThinkCycles);
	for (auto currentthinker : s_thinkers)
	{
		if (!IndependentThinker(currentthinker))
			currentthinker->RunThink();
	}
	END_STAT (ThinkCycles);
	P_CheckMusicChange();
}

void *DThinker::operator new (size_t size)
{
	return Z_Malloc (size, PU_LEVSPEC, 0);
}

// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
void DThinker::operator delete (void *mem)
{
	Z_Free (mem);
}

bool P_ThinkerIsPlayerType(DThinker* thinker)
{
	if (thinker == NULL)
		return false;

	return thinker->IsKindOf(RUNTIME_CLASS(AActor)) &&
	       static_cast<AActor*>(thinker)->type == MT_PLAYER;
}

VERSION_CONTROL (dthinker_cpp, "$Id$")
