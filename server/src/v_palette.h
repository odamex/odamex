// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2008 by The Odamex Team.
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
//	V_PALETTE
//
//-----------------------------------------------------------------------------


#ifndef __V_PALETTE_H__
#define __V_PALETTE_H__

#include "doomtype.h"

#define MAKERGB(r,g,b)		(((r)<<16)|((g)<<8)|(b))
#define MAKEARGB(a,r,g,b)	(((a)<<24)|((r)<<16)|((g)<<8)|(b))

#define APART(c)			(((c)>>24)&0xff)
#define RPART(c)			(((c)>>16)&0xff)
#define GPART(c)			(((c)>>8)&0xff)
#define BPART(c)			((c)&0xff)

struct palette_s {
	struct palette_s *next, *prev;

	union {
		// Which of these is used is determined by screen.is8bit

		byte		*colormaps;		// Colormaps for 8-bit graphics
		DWORD		*shades;		// ARGB8888 values for 32-bit graphics
	} maps;
	byte			*colormapsbase;
	union {
		char		name[8];
		int			nameint[2];
	} name;
	DWORD			*colors;		// gamma corrected colors
	DWORD			*basecolors;	// non-gamma corrected colors
	unsigned		numcolors;
	unsigned		flags;
	unsigned		shadeshift;
	int				usecount;
};
typedef struct palette_s palette_t;

struct dyncolormap_s {
	byte *maps;
	unsigned int color;
	unsigned int fade;
	struct dyncolormap_s *next;
};
typedef struct dyncolormap_s dyncolormap_t;

// GetDefaultPalette()
//
//	Returns the palette created through InitPalettes()
palette_t *GetDefaultPalette (void);

// Colorspace conversion RGB <-> HSV
void RGBtoHSV (float r, float g, float b, float *h, float *s, float *v);
void HSVtoRGB (float *r, float *g, float *b, float h, float s, float v);

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb);

#endif //__V_PALETTE_H__


