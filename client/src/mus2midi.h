// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005 by Simon Howard
// Copyright (C) 2006 by Ben Ryves 2006
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
//	mus2mid.cpp - http://benryves.com - benryves@benryves.com
//	Use to convert a MUS file into a single track, type 0 MIDI file.
// 
//	[Russell] - Minor modifications to make it compile
//
//-----------------------------------------------------------------------------


#ifndef MUS2MID_H
#define MUS2MID_H

#include "memio.h"

int mus2mid(MEMFILE *musinput, MEMFILE *midioutput);

#endif /* #ifndef MUS2MID_H */

