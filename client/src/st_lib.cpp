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

#include "i_video.h"

#include "w_wad.h"

#include "r_local.h"
#include "st_lib.h"
#include "st_stuff.h"

// [RH] Routines to stretch status bar graphics depending on st_scale cvar.
EXTERN_CVAR(st_scale)

//
// StatusBarWidget::clearRect
//
// Copies a rectangular portion of the status bar background surface onto
// the drawing surface. This is used prior to drawing a widget (number or patch)
// onto the surface.
//
void StatusBarWidget::clearRect(int x, int y, int w, int h)
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
// StatusBarWidget::drawPatch
//
// Draws a patch onto the drawing surface. STlib_ClearRect should be used to
// draw a fresh copy of the background prior to drawing the patch.
//
void StatusBarWidget::drawPatch(int x, int y, const patch_t* p)
{
	if (st_scale)
	{
		// draw onto stnum_surface, which will be stretched and blitted
		// onto the rendering surface at the end of the frame.
		const DCanvas* canvas = stnum_surface->getDefaultCanvas();
		canvas->DrawPatch(p, x, y);
	}
	else
	{
		// draw directly onto the rendering surface since stretching isn't being used.
		const DCanvas* canvas = R_GetRenderingSurface()->getDefaultCanvas();
		canvas->DrawPatch(p, x + ST_X, y + ST_Y);
	}
}

void st_number_t::init(int x_, int y_, lumpHandle_t* pl, int* num_, bool* on_,
                       int maxdigits_)
{
	x = x_;
	y = y_;
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
void st_number_t::update(bool force_refresh, bool cleararea)
{
	if (!on)
		return;

	// [jsd]: prevent null references as hard as possible
	if (num == NULL || on == NULL || p == NULL)
		return;

	// only draw if the number is different or refresh is forced
	if (!(force_refresh || oldnum != *num) || !*on)
		return;

	int number = *num;

	const patch_t* p0 = W_ResolvePatchHandle(p[0]);
	const int w = p0->width();
	const int h = p0->height();

	oldnum = *num;

	const bool negative = number < 0;

	if (negative)
	{
		if (maxdigits == 2 && number < -9)
			number = -9;
		else if (maxdigits == 3 && number < -99)
			number = -99;

		number = -number;
	}

	// clear the area
	if (cleararea)
		clearRect(x - w * maxdigits, y, w * maxdigits, h);

	// if non-number, do not draw it
	if (number == ST_DONT_DRAW_NUM)
		return;

	int drawx = x;

	// in the special case of 0, you draw 0
	if (number == 0)
		drawPatch(drawx - w, y, p0);

	// draw the new number
	for (int numdigits = maxdigits; number && numdigits; numdigits--)
	{
		const patch_t* pnum = W_ResolvePatchHandle(p[number % 10]);
		drawx -= w;
		drawPatch(drawx, y, pnum);
		number /= 10;
	}

	// draw a minus sign if necessary
	if (negative)
		drawPatch(drawx - 8, y, W_ResolvePatchHandle(negminus));
}

void st_percent_t::init(int x, int y, lumpHandle_t* pl, int* num, bool* on,
                        lumpHandle_t percent_patch)
{
	n.init(x, y, pl, num, on, 3);
	p = percent_patch;
}

void st_percent_t::update(bool force_refresh)
{
	if (force_refresh && *n.on)
		drawPatch(n.x, n.y, W_ResolvePatchHandle(p));

	n.update(force_refresh);
}

void st_multicon_t::init(int x_, int y_, lumpHandle_t* il, int* inum_, bool* on_)
{
	x = x_;
	y = y_;
	oldinum = -1;
	inum = inum_;
	on = on_;
	p = il;
}

void st_multicon_t::update(bool force_refresh)
{
	if (on && (force_refresh || oldinum != *inum) && (*inum != -1))
	{
		// clear the background area
		if (oldinum != -1)
		{
			const patch_t* oldnum = W_ResolvePatchHandle(p[oldinum]);
			const int drawx = x - oldnum->leftoffset();
			const int drawy = y - oldnum->topoffset();
			const int w = oldnum->width();
			const int h = oldnum->height();

			clearRect(drawx, drawy, w, h);
		}

		const patch_t* drawinum = W_ResolvePatchHandle(p[*inum]);
		drawPatch(x, y, drawinum);
		oldinum = *inum;
	}
}

VERSION_CONTROL (st_lib_cpp, "$Id$")
