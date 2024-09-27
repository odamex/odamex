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
// 	The status bar widget code.
//
//-----------------------------------------------------------------------------

#pragma once

//
// Background and foreground screen numbers
//
// [RH] Status bar is another screen allocated
// by status bar code instead of video code.
extern IWindowSurface* stbar_surface;
extern IWindowSurface* stnum_surface;

//
// Typedefs of widgets
//

class StatusBarWidget_Base
{
  protected:
	int m_x;
	int m_y;

	void clearRect(int x, int y, int w, int h);
	void drawPatch(int x, int y, const patch_t* p);

  public:
	int getX() const { return m_x; }
	int getY() const { return m_y; }
	
};

// Number widget
class StatusBarWidgetNumber : StatusBarWidget_Base
{
  protected:

	void drawPatch(int x, int y, const patch_t* p)
	{
		StatusBarWidget_Base::drawPatch(x, y, p);
	}

  public:
	// upper right-hand corner
	//	of the number (right-justified)
	int getX() const { return StatusBarWidget_Base::getX(); }
	int getY() const { return StatusBarWidget_Base::getY(); }

	int maxdigits;

	// last number value
	int oldnum;

	// pointer to current value
	int* num;

	// list of patches for 0-9
	lumpHandle_t* p;

	// Number widget routines
	void init(int x, int y, lumpHandle_t* pl, int* num, int maxdigits);

	void update(bool refresh, bool cleararea = true);
};

// Percent widget ("child" of number widget,
//	or, more precisely, contains a number widget.)
class StatusBarWidgetPercent : StatusBarWidgetNumber
{
	// percent sign graphic
	lumpHandle_t m_percentLump;

  public:
	// Percent widget routines
	void init(int x, int y, lumpHandle_t* pl, int* num, lumpHandle_t percent);

	void update(bool refresh);
};

// Multiple Icon widget
class StatusBarWidgetMultiIcon : StatusBarWidget_Base
{
  public:
	// center-justified location of icons
	//int x;
	//int y;

	// last icon number
	int oldinum;

	// pointer to current icon
	int* inum;

	// list of icons
	lumpHandle_t* p;

	// Multiple Icon widget routines
	void init(int x, int y, lumpHandle_t* il, int* inum);

	void update(bool refresh);
};

#define ST_DONT_DRAW_NUM 1994 // means "n/a"
