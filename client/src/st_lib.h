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
// 	The status bar widget code.
//
//-----------------------------------------------------------------------------


#ifndef __STLIB__
#define __STLIB__


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

// Number widget

struct st_number_s
{
	// upper right-hand corner
	//	of the number (right-justified)
	int 		x;
	int 		y;

	int			maxdigits;

	// last number value
	int 		oldnum;

	// pointer to current value
	int*		num;

	// pointer to bool stating
	//	whether to update number
	bool*	on;

	// list of patches for 0-9
	lumpHandle_t* p;

	// user data
	int data;

	// Number widget routines
	void init
	(int			x,
	 int			y,
	 lumpHandle_t*	pl,
	 int*			num,
	 bool*			on,
	 int			maxdigits);
	
	void update(bool refresh, bool cleararea = true);

	void draw(bool refresh, bool cleararea = true);

};
typedef st_number_s st_number_t;


// Percent widget ("child" of number widget,
//	or, more precisely, contains a number widget.)
struct st_percent_s
{
	// number information
	st_number_t n;

	// percent sign graphic
	lumpHandle_t p;

	// Percent widget routines
	void init
	(int			x,
	 int			y,
	 lumpHandle_t*	pl,
	 int*			num,
	 bool*			on,
	 lumpHandle_t	percent);

	void update(bool refresh);
};
typedef st_percent_s st_percent_t;


// Multiple Icon widget
struct st_multicon_s
{
	 // center-justified location of icons
	int 				x;
	int 				y;

	// last icon number
	int 				oldinum;

	// pointer to current icon
	int*				inum;

	// pointer to bool stating
	//	whether to update icon
	bool*				on;

	// list of icons
	lumpHandle_t* p;

	// user data
	int 				data;

	// Multiple Icon widget routines
	void init
	(int			x,
	 int			y,
	 lumpHandle_t*	il,
	 int*			inum,
	 bool*			on);

	void update(bool refresh);

};
typedef st_multicon_s st_multicon_t;


// Binary Icon widget

struct st_binicon_s
{
	// center-justified location of icon
	int 				x;
	int 				y;

	// last icon value
	bool			oldval;

	// pointer to current icon status
	bool*			val;

	// pointer to bool
	//	stating whether to update icon
	bool*			on;


	lumpHandle_t		p;		// icon
	int 				data;	// user data

	// Binary Icon widget routines
	void init
	(int			x,
	 int			y,
	 lumpHandle_t	i,
	 bool*			val,
	 bool*			on);

	void update(bool refresh);
};
typedef st_binicon_s st_binicon_t;


//
// Widget creation, access, and update routines
//

#define ST_DONT_DRAW_NUM 1994 // means "n/a"

void ST_DrawNum(int x, int y, DCanvas *scrn, int num);

#endif
