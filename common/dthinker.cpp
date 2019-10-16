// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2015 by The Odamex Team.
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


#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "dthinker.h"
#include "z_zone.h"
#include "stats.h"
#include "p_local.h"

IMPLEMENT_SERIAL (DThinker, DObject)

DThinker *DThinker::FirstThinker = NULL;
DThinker *DThinker::LastThinker = NULL;

std::vector<DThinker *> LingerDestroy;

void DThinker::Serialize (FArchive &arc)
{
	Super::Serialize (arc);

	// We do not serialize m_Next or m_Prev, because the DThinker
	// constructor handles them for us.
}

void DThinker::SerializeAll (FArchive &arc, bool hubLoad, bool noStorePlayers)
{
	DThinker *thinker;
	if (arc.IsStoring () && noStorePlayers)
	{
		thinker = FirstThinker;
		while (thinker)
		{
			// Don't store player mobjs.
			if (!(thinker->IsKindOf(RUNTIME_CLASS(AActor)) &&
			    static_cast<AActor *>(thinker)->type == MT_PLAYER))
			{
				arc << (BYTE)1;
				arc << thinker;
			}
			thinker = thinker->m_Next;
		}
		arc << (BYTE)0;
	}
	else if (arc.IsStoring ())
	{
		thinker = FirstThinker;
		while (thinker)
		{
			arc << (BYTE)1;
			arc << thinker;
			thinker = thinker->m_Next;
		}
		arc << (BYTE)0;
	}
	else
	{
		if (hubLoad || noStorePlayers)
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
	// Add a new thinker at the end of the list.
	m_Prev = LastThinker;
	m_Next = NULL;
	if (LastThinker)
		LastThinker->m_Next = this;
	if (!FirstThinker)
		FirstThinker = this;
	LastThinker = this;
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
	m_Next = NULL;
	m_Prev = NULL;
	refCount = 0;
}

void DThinker::Destroy ()
{
	// denis - allow this function to be safely called multiple times
	if(destroyed)
		return;

	if (FirstThinker == this)
		FirstThinker = m_Next;
	if (LastThinker == this)
		LastThinker = m_Prev;
	if (m_Next)
		m_Next->m_Prev = m_Prev;
	if (m_Prev)
		m_Prev->m_Next = m_Next;
	
	destroyed = true;
		
	if(refCount)
	{
		LingerDestroy.push_back(this); // something is still finding this pointer useful
	}
	else
		Super::Destroy ();

	size_t l = LingerDestroy.size();	
	for(size_t i = 0; i < l; i++)
	{
		DThinker *obj = LingerDestroy[i];
		if(!obj->refCount)
		{
			obj->ObjectFlags |= OF_Cleanup;
			LingerDestroy.erase(LingerDestroy.begin() + i);
			l--; i--;
			delete obj;
		}
	}
}

bool DThinker::WasDestroyed ()
{
	return destroyed;
}

// Destroy every thinker
void DThinker::DestroyAllThinkers ()
{
	DThinker *currentthinker = FirstThinker;
	while (currentthinker)
	{
		DThinker *next = currentthinker->m_Next;
		currentthinker->Destroy ();
		currentthinker = next;
	}
	DObject::EndFrame ();
	
	size_t l = LingerDestroy.size();	
	for(size_t i = 0; i < l; i++)
	{
		DThinker *obj = LingerDestroy[i];
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
	DThinker *thinker = FirstThinker;
	while (thinker)
	{
		DThinker *next = thinker->m_Next;
		if (!thinker->IsKindOf (RUNTIME_CLASS (AActor)) ||
			static_cast<AActor *>(thinker)->player == NULL ||
			static_cast<AActor *>(thinker)->player->mo
			 != static_cast<AActor *>(thinker))
		{
			thinker->Destroy ();
		}
		thinker = next;
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
	DThinker *currentthinker;

	BEGIN_STAT (ThinkCycles);
	currentthinker = FirstThinker;
	while (currentthinker)
	{
		if (!IndependentThinker(currentthinker))
			currentthinker->RunThink();
		currentthinker = currentthinker->m_Next;
	}
	END_STAT (ThinkCycles);
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

VERSION_CONTROL (dthinker_cpp, "$Id$")

