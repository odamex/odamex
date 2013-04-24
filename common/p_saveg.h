// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Savegame I/O, archiving, persistence.
//
//-----------------------------------------------------------------------------


#ifndef __P_SAVEG_H__
#define __P_SAVEG_H__

#include "farchive.h"

// Persistent storage/archiving.
// These are the load / save game routines.
// Also see farchive.(h|cpp)
void P_SerializePlayers (FArchive &arc);
void P_SerializeWorld (FArchive &arc);
void P_SerializeThinkers (FArchive &arc, bool, bool);
void P_SerializeRNGState (FArchive &arc);
void P_SerializeSounds (FArchive &arc);
void P_SerializeACSDefereds (FArchive &arc);
void P_SerializePolyobjs (FArchive &arc);

#endif // __P_SAVEG_H__


