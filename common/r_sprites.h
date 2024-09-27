// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	Loading sprites, skins.
//
//-----------------------------------------------------------------------------


#pragma once

extern int MaxVisSprites;

extern vissprite_t *vissprites;

extern spritedef_t* sprites;
extern int numsprites;

#define MAX_SPRITE_FRAMES 29 // [RH] Macro-ized as in BOOM.

// variables used to look up
//	and range check thing_t sprites patches
extern spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
extern int maxframe;

extern vissprite_t* lastvissprite;

void R_CacheSprite(spritedef_t *sprite);
void R_InitSprites(const char** namelist);
