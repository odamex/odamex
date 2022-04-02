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

#include "st_stuff.h"
#include "i_video.h"	// IWindowSurface
#include "st_lib.h"
#include "r_local.h"	// R_GetRenderingSurface()
#include "resources/res_texture.h"

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
void StatusBarWidget_Base::drawPatch(int x, int y, const Texture* t)
{
	if (st_scale)
	{
		// draw onto stnum_surface, which will be stretched and blitted
		// onto the rendering surface at the end of the frame.
		const DCanvas* canvas = stnum_surface->getDefaultCanvas();
		canvas->DrawTexture(t, x, y);
	}
	else
	{
		// draw directly onto the rendering surface since stretching isn't being used.
		const DCanvas* canvas = R_GetRenderingSurface()->getDefaultCanvas();
		canvas->DrawTexture(t, x + ST_X, y + ST_Y);
	}
}

void StatusBarWidgetNumber::init(int x_, int y_, const Texture** pl, int* num_, bool* on_,
                                 int maxdigits_)
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

	const bool negative = number < 0;

	if (negative)
	{
		if (maxdigits == 2 && number < -9)
			number = -9;
		else if (maxdigits == 3 && number < -99)
			number = -99;

		number = -number;
	}

	const Texture* p0 = p[0];
	const int w = p0->mWidth;
	const int h = p0->mHeight;

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
		drawx -= w;
		drawPatch(drawx, m_y, p[number % 10]);
		number /= 10;
	}

	// draw a minus sign if necessary
	if (negative)
		drawPatch(drawx - 8, m_y, negminus);
}

void StatusBarWidgetPercent::init(int x, int y, const Texture** pl, int* num, bool* on,
                                  const Texture* percent_patch)
{
	StatusBarWidgetNumber::init(x, y, pl, num, on, 3);
	m_percentTex = percent_patch;
}

void StatusBarWidgetPercent::update(bool force_refresh)
{
	if (force_refresh && *on)
		drawPatch(getX(), getY(), m_percentTex);

	StatusBarWidgetNumber::update(force_refresh);
}

void StatusBarWidgetMultiIcon::init(int x_, int y_, const Texture** il, int* inum_,
                                    bool* on_)
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
			const int drawx = m_x - p[oldinum]->mOffsetX;
			const int drawy = m_y - p[oldinum]->mOffsetY;
			const int w = p[oldinum]->mWidth;
			const int h = p[oldinum]->mHeight;

			clearRect(drawx, drawy, w, h);
		}

		drawPatch(m_x, m_y, p[*inum]);
		oldinum = *inum;
	}
}

VERSION_CONTROL (st_lib_cpp, "$Id$")
