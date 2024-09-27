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
//		The status bar widget code.
//		[RH] Widget coordinates are now relative to status bar
//			 instead of the game screen.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "st_stuff.h"
#include "i_video.h"	// IWindowSurface
#include "st_lib.h"
#include "r_local.h"	// R_GetRenderingSurface()

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
// StatusBarWidget_Base::drawPatch
//
// Draws a patch onto the drawing surface. STlib_ClearRect should be used to
// draw a fresh copy of the background prior to drawing the patch.
//
void StatusBarWidget_Base::drawPatch(int x, int y, const patch_t* p)
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

void StatusBarWidgetNumber::init(int x_, int y_, lumpHandle_t* pl, int* num_,
                                 int maxdigits_)
{
	m_x = x_;
	m_y = y_;
	oldnum = 0;
	maxdigits = maxdigits_;
	num = num_;
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
	if (num == NULL || p == NULL)
		return;

	// only draw if the number is different or refresh is forced
	if (!(force_refresh || oldnum != *num))
		return;

	int number = *num;
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
	if (number == 0)
		drawPatch(drawx - w, m_y, p0);

	// draw the new number
	for (int numdigits = maxdigits; number && numdigits; numdigits--)
	{
		const patch_t* pnum = W_ResolvePatchHandle(p[number % 10]);
		drawx -= w;
		drawPatch(drawx, m_y, pnum);
		number /= 10;
	}

	// draw a minus sign if necessary
	if (negative)
		drawPatch(drawx - 8, m_y, W_ResolvePatchHandle(negminus));
}

void StatusBarWidgetPercent::init(int x, int y, lumpHandle_t* pl, int* num,
                                  lumpHandle_t percent_patch)
{
	StatusBarWidgetNumber::init(x, y, pl, num, 3);
	m_percentLump = percent_patch;
}

void StatusBarWidgetPercent::update(bool force_refresh)
{
	if (force_refresh)
		drawPatch(getX(), getY(), W_ResolvePatchHandle(m_percentLump));

	StatusBarWidgetNumber::update(force_refresh);
}

void StatusBarWidgetMultiIcon::init(int x_, int y_, lumpHandle_t* il, int* inum_)
{
	m_x = x_;
	m_y = y_;
	oldinum = -1;
	inum = inum_;
	p = il;
}

void StatusBarWidgetMultiIcon::update(bool force_refresh)
{
	if ((force_refresh || oldinum != *inum) && (*inum != -1))
	{
		// clear the background area
		if (oldinum != -1)
		{
			const patch_t* oldnum = W_ResolvePatchHandle(p[oldinum]);
			const int drawx = m_x - oldnum->leftoffset();
			const int drawy = m_y - oldnum->topoffset();
			const int w = oldnum->width();
			const int h = oldnum->height();

			clearRect(drawx, drawy, w, h);
		}

		drawPatch(m_x, m_y, W_ResolvePatchHandle(p[*inum]));
		oldinum = *inum;
	}
}

VERSION_CONTROL (st_lib_cpp, "$Id$")
