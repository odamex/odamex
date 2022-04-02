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
//		The status bar widget code.
//		[RH] Widget coordinates are now relative to status bar
//			 instead of the game screen.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "z_zone.h"
#include "v_video.h"
#include "i_video.h"

#include "st_stuff.h"
#include "i_video.h"	// IWindowSurface
#include "st_lib.h"
#include "r_local.h"	// R_GetRenderingSurface()

#include "c_cvars.h"
#include "resources/res_texture.h"


//
// Hack display negative frags.
//	Loads and store the stminus lump.
//
const Texture* sttminus;

void STlib_init(void)
{
	sttminus = Res_CacheTexture("STTMINUS", PATCH, PU_STATIC);
}


// [RH] Routines to stretch status bar graphics depending on st_scale cvar.
EXTERN_CVAR(st_scale)

//
// StatusBarWidget_Base::clearRect
//
// Copies a rectangular portion of the status bar background surface onto
// the drawing surface. This is used prior to drawing a widget (number or patch)
// onto the surface.
//
void StatusBarWidget_Base::clearRect(int x, int y, int w, int h)
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
		R_GetRenderingSurface()->blit(stbar_surface, x, y, w, h, x + ST_X, y + ST_Y, w,
		                              h);
	}
}

//
// STlib_DrawTexture
//
// Draws a patch onto the drawing surface. STlib_ClearRect should be used to
// draw a fresh copy of the background prior to drawing the patch.
//
static void STlib_DrawTexture(int x, int y, const Texture* texture)
{
	if (st_scale)
	{
		// draw onto stnum_surface, which will be stretched and blitted
		// onto the rendering surface at the end of the frame.
		DCanvas* canvas = stnum_surface->getDefaultCanvas();
		canvas->DrawTexture(texture, x, y);
	}
	else
	{
		// draw directly onto the rendering surface since stretching isn't being used.
		DCanvas* canvas = R_GetRenderingSurface()->getDefaultCanvas();
		canvas->DrawTexture(texture, x + ST_X, y + ST_Y);
	}
}

void STlib_initNum(st_number_t* n, int x, int y, const Texture** pl, int* num, bool* on, int maxdigits)
{
	m_x = x_;
	m_y = y_;
	oldnum = 0;
	maxdigits = maxdigits_;
	num = num_;
	on = on_;
	p = pl;
}

//
// st_number_s::update
//
// A fairly efficient way to draw a number based on differences from the
// old number. Worth the trouble?
//
void StatusBarWidgetNumber::update(bool force_refresh, bool cleararea)
{
	// [jsd]: prevent null references as hard as possible
	if (num == NULL || on == NULL || p == NULL)
		return;

	// only draw if the number is different or refresh is forced
	if (!(force_refresh || oldnum != *num) || !*on)
		return;

	int number = *num;
	oldnum = *num;

	int 		w = n->p[0]->mWidth;
	int 		h = n->p[0]->mHeight;
	int 		x = n->x;

	const bool negative = number < 0;

	if (negative)
	{
		if (maxdigits == 2 && number < -9)
			number = -9;
		else if (maxdigits == 3 && number < -99)
			number = -99;

		number = -number;
	}

	const patch_t* p0 = W_ResolvePatchHandle(p[0]);
	const int w = p0->width();
	const int h = p0->height();

	// clear the area
	if (cleararea)
		clearRect(m_x - w * maxdigits, m_y, w * maxdigits, h);

	// if non-number, do not draw it
	if (number == ST_DONT_DRAW_NUM)
		return;

	int drawx = m_x;

	// in the special case of 0, you draw 0
	if (num == 0)
		STlib_DrawTexture(x - w, n->y, n->p[0]);

	// draw the new number
	for (int numdigits = maxdigits; number && numdigits; numdigits--)
	{
		x -= w;
		STlib_DrawTexture(x, n->y, n->p[num % 10]);
		num /= 10;
	}

	// draw a minus sign if necessary
	if (negative)
		STlib_DrawTexture(x - 8, n->y, sttminus);
}


void STlib_updateNum(st_number_t* n, bool force_refresh, bool cleararea)
{
	if (*n->on)
		STlib_drawNum(n, force_refresh, cleararea);
}


void STlib_initPercent(st_percent_t* p, int x, int y, const Texture** pl, int* num, bool* on, const Texture* percent_texture)
{
	STlib_initNum(&p->n, x, y, pl, num, on, 3);
	p->p = percent_texture;
}

void StatusBarWidgetPercent::update(bool force_refresh)
{
	if (force_refresh && *percent->n.on)
		STlib_DrawTexture(percent->n.x, percent->n.y, percent->p);

	StatusBarWidgetNumber::update(force_refresh);
}



void STlib_initMultIcon(st_multicon_t* icon, int x, int y, const Texture** il, int* inum, bool* on)
{
	m_x = x_;
	m_y = y_;
	oldinum = -1;
	inum = inum_;
	on = on_;
	p = il;
}

void StatusBarWidgetMultiIcon::update(bool force_refresh)
{
	if (on && (force_refresh || oldinum != *inum) && (*inum != -1))
	{
		// clear the background area
		if (oldinum != -1)
		{
			int x = icon->x - icon->p[icon->oldinum]->mOffsetX;
			int y = icon->y - icon->p[icon->oldinum]->mOffsetY;
			int w = icon->p[icon->oldinum]->mWidth;
			int h = icon->p[icon->oldinum]->mHeight;

			STlib_ClearRect(x, y, w, h);
		}

		STlib_DrawTexture(icon->x, icon->y, icon->p[*icon->inum]);
		icon->oldinum = *icon->inum;
	}
}



void STlib_initBinIcon(st_binicon_t* icon, int x, int y, const Texture* texture, bool* val, bool* on)
{
	icon->x			= x;
	icon->y			= y;
	icon->oldval	= 0;
	icon->val		= val;
	icon->on		= on;
	icon->p			= texture;
}



void STlib_updateBinIcon(st_binicon_t* icon, bool force_refresh)
{
	if (*icon->on && (force_refresh || icon->oldval != *icon->val))
	{
		int x = icon->x - icon->p->mOffsetX;
		int y = icon->y - icon->p->mOffsetY;
		int w = icon->p->mWidth;
		int h = icon->p->mHeight;

		if (*icon->val)
			STlib_DrawTexture(icon->x, icon->y, icon->p);
		else
			STlib_ClearRect(x, y, w, h);

		icon->oldval = *icon->val;
	}
}

VERSION_CONTROL (st_lib_cpp, "$Id$")
