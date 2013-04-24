// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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


#ifndef __DOBJECT_H__
#define __DOBJECT_H__

#include <stdlib.h>
#include "tarray.h"
#include "doomtype.h"

class FArchive;

class	DObject;
class		DArgs;
class		DBoundingBox;
class		DCanvas;
class		DConsoleCommand;
class			DConsoleAlias;
class		DSeqNode;
class			DSeqActorNode;
class			DSeqSectorNode;
class		DThinker;
class			AActor;
class			DPusher;
class			DScroller;
class			DSectorEffect;
class				DLighting;
class					DFireFlicker;
class					DFlicker;
class					DGlow;
class					DGlow2;
class					DLightFlash;
class					DPhased;
class					DStrobe;
class				DMover;
class					DElevator;
class					DMovingCeiling;
class						DCeiling;
class						DDoor;
class					DMovingFloor;
class						DFloor;
class						DPlat;
class					DPillar;

struct TypeInfo
{
	TypeInfo ()
	{}

	TypeInfo (const char *inName, const TypeInfo *inParentType, unsigned int inSize)
		: Name (inName),
		  ParentType (inParentType),
		  SizeOf (inSize),
		  TypeIndex(0)
	{}
	TypeInfo (const char *inName, const TypeInfo *inParentType, unsigned int inSize, DObject *(*inNew)())
		: Name (inName),
		  ParentType (inParentType),
		  SizeOf (inSize),
		  CreateNew (inNew),
		  TypeIndex(0)
	{}

	const char *Name;
	const TypeInfo *ParentType;
	unsigned int SizeOf;
	DObject *(*CreateNew)();
	unsigned short TypeIndex;

	void RegisterType ();

	// Returns true if this type is an ansector of (or same as) the passed type.
	bool IsAncestorOf (const TypeInfo *ti) const
	{
		while (ti)
		{
			if (this == ti)
				return true;
			ti = ti->ParentType;
		}
		return false;
	}
	inline bool IsDescendantOf (const TypeInfo *ti) const
	{
		return ti->IsAncestorOf (this);
	}

	static const TypeInfo *FindType (const char *name);

	static unsigned short m_NumTypes, m_MaxTypes;
	static TypeInfo **m_Types;
};

struct ClassInit
{
	ClassInit (TypeInfo *newClass);
};

#define RUNTIME_TYPE(object)	(object->StaticType())
#define RUNTIME_CLASS(cls)		(&cls::_StaticType)

#define _DECLARE_CLASS(cls,parent) \
	virtual TypeInfo *StaticType() const { return RUNTIME_CLASS(cls); } \
private: \
	typedef parent Super; \
	typedef cls ThisClass; \
protected:

#define DECLARE_CLASS(cls,parent) \
public: \
	static TypeInfo _StaticType; \
	_DECLARE_CLASS(cls,parent)

#define _DECLARE_SERIAL(cls,parent) \
	static DObject *CreateObject (); \
public: \
	bool CanSerialize() { return true; } \
	void Serialize (FArchive &); \
	inline friend FArchive &operator>> (FArchive &arc, cls* &object) \
	{ \
		return arc.ReadObject ((DObject* &)object, RUNTIME_CLASS(cls)); \
	}

#define DECLARE_SERIAL(cls,parent) \
	DECLARE_CLASS(cls,parent) \
	_DECLARE_SERIAL(cls,parent)

#define _IMPLEMENT_CLASS(cls,parent,new) \
	TypeInfo cls::_StaticType (#cls, RUNTIME_CLASS(parent), sizeof(cls), new);

#define IMPLEMENT_CLASS(cls,parent) \
	_IMPLEMENT_CLASS(cls,parent,NULL) \

#define _IMPLEMENT_SERIAL(cls,parent) \
	_IMPLEMENT_CLASS(cls,parent,cls::CreateObject) \
	DObject *cls::CreateObject() { return new cls; } \
	static ClassInit _Init##cls(RUNTIME_CLASS(cls));

#define IMPLEMENT_SERIAL(cls,parent) \
	_IMPLEMENT_SERIAL(cls,parent)

enum EObjectFlags
{
	OF_MassDestruction	= 0x00000001,	// Object is queued for deletion
	OF_Cleanup			= 0x00000002	// Object is being deconstructed as a result of a queued deletion
};

class DObject
{
public: \
	static TypeInfo _StaticType; \
	virtual TypeInfo *StaticType() const { return &_StaticType; } \
private: \
	typedef DObject ThisClass;

public:
	DObject ();
	virtual ~DObject ();

	inline bool IsKindOf (const TypeInfo *base) const
	{
		return base->IsAncestorOf (StaticType ());
	}

	inline bool IsA (const TypeInfo *type) const
	{
		return (type == StaticType());
	}

	virtual void Serialize (FArchive &arc) {}
	virtual void Destroy ();

	static void BeginFrame ();
	static void EndFrame ();

	DWORD ObjectFlags;

	static void STACK_ARGS StaticShutdown ();

private:
	static TArray<DObject *> Objects;
	static TArray<size_t> FreeIndices;
	static TArray<DObject *> ToDestroy;

	void RemoveFromArray ();

	static bool Inactive;
	size_t Index;
};

#include "farchive.h"

#endif //__DOBJECT_H__


