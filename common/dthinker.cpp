// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2007 by The Odamex Team.
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

#include "dthinker.h"
#include "z_zone.h"
#include "stats.h"
#include "p_local.h"

IMPLEMENT_SERIAL (DThinker, DObject)

DThinker *DThinker::FirstThinker = NULL;
DThinker *DThinker::LastThinker = NULL;

void DThinker::Serialize (FArchive &arc)
{
	Super::Serialize (arc);

	// We do not serialize m_Next or m_Prev, because the DThinker
	// constructor handles them for us.
}

void DThinker::SerializeAll (FArchive &arc, bool hubLoad)
{
	DThinker *thinker;

	if (arc.IsStoring ())
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
	// Add a new thinker at the end of the list.
	m_Prev = LastThinker;
	m_Next = NULL;
	if (LastThinker)
		LastThinker->m_Next = this;
	if (!FirstThinker)
		FirstThinker = this;
	LastThinker = this;
}

DThinker::~DThinker ()
{
	if (FirstThinker == this)
		FirstThinker = m_Next;
	if (LastThinker == this)
		LastThinker = m_Prev;
	if (m_Next)
		m_Next->m_Prev = m_Prev;
	if (m_Prev)
		m_Prev->m_Next = m_Next;
}

void DThinker::Destroy ()
{
	if (FirstThinker == this)
		FirstThinker = m_Next;
	if (LastThinker == this)
		LastThinker = m_Prev;
	if (m_Next)
		m_Next->m_Prev = m_Prev;
	if (m_Prev)
		m_Prev->m_Next = m_Next;
	m_Next = m_Prev = NULL;
	Super::Destroy ();
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

CVAR (sv_speedhackfix, "0", CVAR_SERVERINFO)

void DThinker::RunThinkers ()
{
	DThinker *currentthinker;

	BEGIN_STAT (ThinkCycles);
	currentthinker = FirstThinker;
	while (currentthinker)
	{
		DThinker *next = currentthinker->m_Next;
		
		if ( currentthinker->IsKindOf (RUNTIME_CLASS (AActor))
				   && static_cast<AActor *>(currentthinker)->player
				   && static_cast<AActor *>(currentthinker)->player->playerstate != PST_DEAD
				   && !sv_speedhackfix && !demoplayback)
			;
		else
			currentthinker->RunThink ();
		
		currentthinker = next;
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

VERSION_CONTROL (dthinker_cpp, "$Id:$")

