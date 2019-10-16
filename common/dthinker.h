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


#ifndef __DTHINKER_H__
#define __DTHINKER_H__

#include <stdlib.h>
#include "dobject.h"

class AActor;
class player_s;
struct pspdef_s;

typedef void (*actionf_v)();
typedef void (*actionf_p1)( AActor* );
typedef void (*actionf_p2)( player_s*, pspdef_s* );

typedef union
{
	void *acvoid;
	actionf_p1	acp1;
	actionf_v	acv;
	actionf_p2	acp2;
} actionf_t;

// Historically, "think_t" is yet another
// function pointer to a routine to handle an actor
typedef actionf_t  think_t;

class FThinkerIterator;

// Doubly linked list of thinkers
class DThinker : public DObject
{
	DECLARE_SERIAL (DThinker, DObject)

public:
	DThinker ();
	void Orphan();
	virtual void Destroy ();
	virtual ~DThinker ();
	virtual void RunThink () {}

	void *operator new (size_t size);
	void operator delete (void *block);

	// Both the head and tail of the thinker list.
	static DThinker *FirstThinker;
	static DThinker *LastThinker;
	static void RunThinkers ();
	static void DestroyAllThinkers ();
	static void DestroyMostThinkers ();
	static void SerializeAll (FArchive &arc, bool keepPlayers, bool noStorePlayers);

	bool WasDestroyed();

	size_t refCount;

private:
	DThinker *m_Next, *m_Prev;
	bool destroyed;

	friend class FThinkerIterator;
};

class FThinkerIterator
{
private:
	TypeInfo *m_ParentType;
	DThinker *m_CurrThinker;

public:
	FThinkerIterator (TypeInfo *type)
	{
		m_ParentType = type;
		m_CurrThinker = DThinker::FirstThinker;
	}
	DThinker *Next ()
	{
		while (m_CurrThinker)
		{
			if (m_CurrThinker->IsKindOf (m_ParentType))
			{
				DThinker *res = m_CurrThinker;
				m_CurrThinker = m_CurrThinker->m_Next;
				return res;
			}
			m_CurrThinker = m_CurrThinker->m_Next;
		}
		m_CurrThinker = DThinker::FirstThinker;
		return NULL;
	}
};

template <class T> class TThinkerIterator : public FThinkerIterator
{
public:
	TThinkerIterator () : FThinkerIterator (RUNTIME_CLASS(T))
	{
	}
	T *Next ()
	{
		return static_cast<T *>(FThinkerIterator::Next ());
	}
};

#endif //__DTHINKER_H__

