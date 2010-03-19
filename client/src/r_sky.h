// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#ifndef __R_SKY_H__
#define __R_SKY_H__

#include "c_cvars.h"

// SKY, store the number for name.
extern char SKYFLATNAME[8];

extern int		sky1shift,		sky2shift;

extern int 		sky1texture,	sky2texture;
extern fixed_t	sky1pos,		sky2pos;
extern fixed_t	skytexturemid;
extern int		skystretch;
extern fixed_t	skyiscale;
extern fixed_t	skyscale;
extern fixed_t	skyheight;

EXTERN_CVAR (r_stretchsky)

// Called whenever the sky changes.
void R_InitSkyMap		();

#endif //__R_SKY_H__

