// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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

#include "c_cvars.h"
#include "r_defs.h"
#include "resources/res_resourceid.h"

class Texture;

// SKY, store the number for name.
inline OLumpName SKYFLATNAME = "F_SKY1";

extern int		sky1shift;
extern int		sky2shift;

extern fixed_t	sky1scrolldelta;
extern fixed_t	sky2scrolldelta;
extern fixed_t	sky1columnoffset;
extern fixed_t	sky2columnoffset;

extern fixed_t	skytexturemid;
extern int		skystretch;
extern fixed_t	skyiscale;
extern fixed_t	skyscale;
extern fixed_t	skyheight;

EXTERN_CVAR (r_stretchsky)

// Called whenever the sky changes.
void R_InitSkyMap();
void R_InitSkyDefs();
void R_InitSkiesForLevel();
void R_ClearSkyDefs();
void R_SetDefaultSky(const OLumpName& sky);
void R_SetSkyTextures(const char* sky1_name, const char* sky2_name);
void R_UpdateSkies();
void R_ActivateSkies();

void R_InterpolateSkyDefs(fixed_t amount);
void R_TicSkyDefInterpolation();
void R_RestoreSkyDefs();

void R_RenderSkyRange(visplane_t* pl);

bool R_ResourceIdIsSkyFlat(const ResourceId res_id);
static inline bool R_IsSkyFlat(const ResourceId res_id) { return R_ResourceIdIsSkyFlat(res_id); }
