// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Thing frame/state LUT,
//	generated by multigen utilitiy.
//	This one is the original DOOM version, preserved.
//
//-----------------------------------------------------------------------------

#pragma once

#include "m_vectors.h"

#define FX_ROCKET			0x00000001
#define FX_GRENADE			0x00000002
#define FX_VISIBILITYPULSE	0x00000040

#define FX_FOUNTAINMASK		0x00070000
#define FX_FOUNTAINSHIFT	16
#define FX_REDFOUNTAIN		0x00010000
#define FX_GREENFOUNTAIN	0x00020000
#define FX_BLUEFOUNTAIN		0x00030000
#define FX_YELLOWFOUNTAIN	0x00040000
#define FX_PURPLEFOUNTAIN	0x00050000
#define FX_BLACKFOUNTAIN	0x00060000
#define FX_WHITEFOUNTAIN	0x00070000


struct particle_s;

struct particle_s *JitterParticle (int ttl);

void P_ThinkParticles (void);
void P_InitEffects (void);
void P_RunEffects (void);

void P_RunEffect (AActor *actor, int effects);

void P_DrawRailTrail(v3double_t &start, v3double_t &end);
void P_DrawSplash (int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int kind);
void P_DrawSplash2 (int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int updown, int kind);
void P_DisconnectEffect (AActor *actor);
