// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//	Data objects (?)
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "dobject.h"
#include "m_alloc.h"		// Ideally, DObjects can be used independant of Doom.
#include "d_player.h"		// See p_user.cpp to find out why this doesn't work.
#include "z_zone.h"
#include "m_stacktrace.h"

ClassInit::ClassInit (TypeInfo *type)
{
	type->RegisterType ();
}

TypeInfo **TypeInfo::m_Types;
unsigned short TypeInfo::m_NumTypes;
unsigned short TypeInfo::m_MaxTypes;

void TypeInfo::RegisterType ()
{
	if (m_NumTypes == m_MaxTypes)
	{
		m_MaxTypes = m_MaxTypes ? m_MaxTypes*2 : 32;
		m_Types = (TypeInfo **)M_Realloc (m_Types, m_MaxTypes * sizeof(*m_Types));
	}
	m_Types[m_NumTypes] = this;
	TypeIndex = m_NumTypes;
	m_NumTypes++;
}

const TypeInfo *TypeInfo::FindType (const char *name)
{
	unsigned short i;

	for (i = 0; i != m_NumTypes; i++)
		if (!strcmp (name, m_Types[i]->Name))
			return m_Types[i];

	return NULL;
}

TypeInfo DObject::_StaticType("DObject", NULL, sizeof(DObject));

DObject::~DObject ()
{
	if (!Inactive)
	{
		if (!(ObjectFlags & OF_Cleanup) && (ObjectFlags & OF_Destroyed))
		{
			// object is queued for deletion, but is not being deleted
			// by the destruction process, so remove it from the
			// ToDestroy array and do other necessary stuff.
			for (auto& obj : OUtil::reverse(ToDestroy))
			{
				if (obj == this)
				{
					obj = nullptr;
					break;
				}
			}
		}
	}
}

void DObject::Destroy ()
{
	if (!Inactive)
	{
		if (!(ObjectFlags & OF_Destroyed))
		{
			ObjectFlags |= OF_Destroyed;
			ToDestroy.push_back(this);
		}
	}
	else
		delete this;
}

void DObject::BeginFrame ()
{
}

void DObject::EndFrame ()
{
	for (DObject* obj : ToDestroy)
	{
		if (obj)
		{
			obj->ObjectFlags |= OF_Cleanup;
			delete obj;
		}
	}
	ToDestroy.clear();
}

void STACK_ARGS DObject::StaticShutdown ()
{
	Inactive = true;

	// denis - thinkers should be destroyed, but possibly not here?
	DThinker::DestroyAllThinkers();
}

VERSION_CONTROL (dobject_cpp, "$Id$")

