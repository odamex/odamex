// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  Server-specific sound function declarations
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"
#include "m_fixed.h"

class AActor;
class player_t;

void SV_Sound (const AActor *mo, byte channel, const char *name, byte attenuation);
void SV_Sound(player_t& pl, const AActor* mo, const byte channel, const char* name, const byte attenuation);
void SV_Sound (fixed_t x, fixed_t y, byte channel, const char *name, byte attenuation);
void SV_SoundTeam (byte channel, const char* name, byte attenuation, int t);
