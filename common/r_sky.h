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
//	Sky rendering.
//
//-----------------------------------------------------------------------------

#pragma once


// SKY, store the number for name.
extern char SKYFLATNAME[8];

extern int      sky1texture;
extern int      sky2texture;
extern fixed_t	sky2scrollxdelta;
extern fixed_t	sky2columnoffset;
extern int		skystretch;
extern fixed_t	skyiscale;
extern fixed_t	skyheight;

EXTERN_CVAR (r_stretchsky)

// Called whenever the sky changes.
void R_InitSkyMap();
void R_InitSkyDefs();
void R_InitSkiesForLevel();
void R_ClearSkyDefs();
void R_SetDefaultSky(const char* sky);
void R_UpdateSkies();
bool R_IsSkyFlat(int flatnum);
void R_ActivateSkies(const byte* hitlist, std::vector<int>& skytextures);

void R_RenderSkyRange(visplane_t* pl);
