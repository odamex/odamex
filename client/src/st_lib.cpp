// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//		The status bar widget code.
//		[RH] Widget coordinates are now relative to status bar
//			 instead of the game screen.
//
//-----------------------------------------------------------------------------



#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"
#include "v_video.h"
#include "i_video.h"

#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

#include "c_cvars.h"
#include "m_swap.h"


//
// Hack display negative frags.
//	Loads and store the stminus lump.
//
patch_t*				sttminus;

void STlib_init(void)
{
	sttminus = wads.CachePatch("STTMINUS", PU_STATIC);
}


// [RH] Routines to stretch status bar graphics depending on st_scale cvar.
EXTERN_CVAR (st_scale)

//
// STlib_ClearRect
//
// Copies a rectangular portion of the status bar background surface onto
// the drawing surface. This is used prior to drawing a widget (number or patch)
// onto the surface.
//
static void STlib_ClearRect(int x, int y, int w, int h)
{
	if (st_scale)
	{
		// draw onto stnum_surface, which will be stretched and blitted
		// onto the rendering surface at the end of the frame.
		stnum_surface->blit(stbar_surface, x, y, w, h, x, y, w, h);
	}
	else
	{
		// draw directly onto the rendering surface since stretching isn't being used.
		R_GetRenderingSurface()->blit(stbar_surface, x, y, w, h, x + ST_X, y + ST_Y, w, h);
	}
}


//
// STlib_DrawPatch
//
// Draws a patch onto the drawing surface. STlib_ClearRect should be used to
// draw a fresh copy of the background prior to drawing the patch.
//
static void STlib_DrawPatch(int x, int y, patch_t* p)
{
	if (st_scale)
	{
		// draw onto stnum_surface, which will be stretched and blitted
		// onto the rendering surface at the end of the frame.
		DCanvas* canvas = stnum_surface->getDefaultCanvas();
		canvas->DrawPatch(p, x, y);
	}
	else
	{
		// draw directly onto the rendering surface since stretching isn't being used.
		DCanvas* canvas = R_GetRenderingSurface()->getDefaultCanvas();
		canvas->DrawPatch(p, x + ST_X, y + ST_Y);
	}
}

void STlib_initNum(st_number_t* n, int x, int y, patch_t** pl, int* num, bool* on, int maxdigits)
{
	n->x			= x;
	n->y			= y;
	n->oldnum		= 0;
	n->maxdigits	= maxdigits;
	n->num			= num;
	n->on			= on;
	n->p			= pl;
}


//
// STlib_drawNum
//
// A fairly efficient way to draw a number based on differences from the
// old number. Worth the trouble?
//
void STlib_drawNum(st_number_t* n, bool force_refresh)
{
	// only draw if the number is different or refresh is forced
	if ((force_refresh || n->oldnum != *n->num) && *n->on)
	{
		int 		num = *n->num;

		int 		w = n->p[0]->width();
		int 		h = n->p[0]->height();
		int 		x = n->x;

		n->oldnum = *n->num;

		bool negative = num < 0;

		if (negative)
		{
			if (n->maxdigits == 2 && num < -9)
				num = -9;
			else if (n->maxdigits == 3 && num < -99)
				num = -99;

			num = -num;
		}

		// clear the area
		STlib_ClearRect(n->x - w * n->maxdigits, n->y, w * n->maxdigits, h);

		// if non-number, do not draw it
		if (num == 1994)
			return;

		x = n->x;

		// in the special case of 0, you draw 0
		if (num == 0)
			STlib_DrawPatch(x - w, n->y, n->p[0]);

		// draw the new number
		for (int numdigits = n->maxdigits; num && numdigits; numdigits--)
		{
			x -= w;
			STlib_DrawPatch(x, n->y, n->p[num % 10]);
			num /= 10;
		}

		// draw a minus sign if necessary
		if (negative)
			STlib_DrawPatch(x - 8, n->y, sttminus);
	}
}


void STlib_updateNum(st_number_t* n, bool force_refresh)
{
	if (*n->on)
		STlib_drawNum(n, force_refresh);
}


void STlib_initPercent(st_percent_t* p, int x, int y, patch_t** pl, int* num, bool* on, patch_t* percent_patch)
{
	STlib_initNum(&p->n, x, y, pl, num, on, 3);
	p->p = percent_patch;
}


void STlib_updatePercent(st_percent_t* percent, bool force_refresh)
{
	if (force_refresh && *percent->n.on)
		STlib_DrawPatch(percent->n.x, percent->n.y, percent->p);

	STlib_updateNum(&percent->n, force_refresh);
}



void STlib_initMultIcon(st_multicon_t* icon, int x, int y, patch_t** il, int* inum, bool* on)
{
	icon->x			= x;
	icon->y			= y;
	icon->oldinum	= -1;
	icon->inum 		= inum;
	icon->on		= on;
	icon->p			= il;
}



void STlib_updateMultIcon(st_multicon_t* icon, bool force_refresh)
{
	if (*icon->on && (force_refresh || icon->oldinum != *icon->inum)
		&& (*icon->inum != -1))
	{
		// clear the background area
		if (icon->oldinum != -1)
		{
			int x = icon->x - icon->p[icon->oldinum]->leftoffset();
			int y = icon->y - icon->p[icon->oldinum]->topoffset();
			int w = icon->p[icon->oldinum]->width();
			int h = icon->p[icon->oldinum]->height();

			STlib_ClearRect(x, y, w, h);
		}

		STlib_DrawPatch(icon->x, icon->y, icon->p[*icon->inum]);
		icon->oldinum = *icon->inum;
	}
}



void STlib_initBinIcon(st_binicon_t* icon, int x, int y, patch_t* patch, bool* val, bool* on)
{
	icon->x			= x;
	icon->y			= y;
	icon->oldval	= 0;
	icon->val		= val;
	icon->on		= on;
	icon->p			= patch;
}



void STlib_updateBinIcon(st_binicon_t* icon, bool force_refresh)
{
	if (*icon->on && (force_refresh || icon->oldval != *icon->val))
	{
		int x = icon->x - icon->p->leftoffset();
		int y = icon->y - icon->p->topoffset();
		int w = icon->p->width();
		int h = icon->p->height();

		if (*icon->val)
			STlib_DrawPatch(icon->x, icon->y, icon->p);
		else
			STlib_ClearRect(x, y, w, h);

		icon->oldval = *icon->val;
	}
}

VERSION_CONTROL (st_lib_cpp, "$Id$")

