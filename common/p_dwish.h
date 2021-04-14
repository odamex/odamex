// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
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
//  "Death Wish" game mode.
//
//-----------------------------------------------------------------------------

#ifndef __P_DWISH__
#define __P_DWISH__

#include "actor.h"
#include "doomdata.h"

void P_AddDWSpawnPoint(AActor* mo);
void P_AddHealthPool(AActor* mo);
void P_RemoveHealthPool(AActor* mo);

void P_RunDWTics();
bool P_IsDWMode();

#endif // __G_DEATHWISH__
