// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   Menu widget stuff, episode selection and such.
//
//-----------------------------------------------------------------------------


#pragma once

#include <array>
#include <span>

#include "d_event.h"
#include "olumpname.h"
#include "c_cvars.h"

// Some defines...
#define LINEHEIGHT	16
#define SKULLXOFF	-32

#define SLIDER_TRACK_X		5
#define SLIDER_TRACK_WIDTH	78

//
// MENUS
//
// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
bool M_Responder(const event_t& ev);

// Called by main loop,
// only used for menu (skull cursor) animation.
void M_Ticker (void);

// Called by main loop,
// draws the menus directly into the screen buffer.
void M_Drawer (void);

// Called by D_DoomMain,
// loads the config file.
void M_Init (void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.
void M_StartControlPanel (void);

// [RH] Setup options menu
bool M_StartOptionsMenu (void);

// [RH] Handle keys for options menu
void M_OptResponder(const event_t& ev);

// [RH] Draw options menu
void M_OptDrawer (void);

// Move the options menu selection to whatever the mouse is hovering over
void M_OptUpdateMouseItem();

// [RH] Initialize options menu
void M_OptInit();

void M_PlayerSetup(int choice);

struct menu_t;
void M_SwitchMenu(menu_t* menu);

void M_PopMenuStack();

// [RH] Called whenever the display mode changes
void M_RefreshModesList();

//
// MENU TYPEDEFS
//
enum itemtype
{
	whitetext,
	redtext,
	yellowtext,
	orangetext,
	more,
	slider,
	redslider,
	blueslider,
	greenslider,
	discrete,
	cdiscrete,
	svdiscrete,		// Ch0wW : serverside discrete
	control,
	mapcontrol,		// Ch0wW : Automap bindings
	netdemocontrol,	// Ch0wW : Netdemo bindings
	screenres,
	bitflag,
	listelement,
	joyactive,
	joyaxis,
	nochoice
};

using cvarfunc = void (*)(cvar_t *cvar, float newval);
using voidfunc = void (*)();
using intfunc = void (*)(int);

struct value_t {
	float		value;
	const char	*name;
};

// TODO: this is barely functional in c++
// almost the entire menu is undefined behavior
// replace with std::variant maybe?
struct menuitem_t {
	itemtype		  type;
	const char			 *label;
	union {
		cvar_t			 *cvar;
		int				  selmode;
		int				  flagmask;
	} a;
	union {
		float			  leftval;		/* aka numvalues aka invflag */
		int				  key1;
		char			 *res1;
	} b;
	union {
		float			  rightval;
		int				  key2;
		char			 *res2;
	} c;
	union {
		float			  step;
		char			 *res3;
	} d;
	union {
		value_t*    values;
		const char* command;
		cvarfunc    cfunc;
		voidfunc    mfunc;
		intfunc     lfunc;
		int         highlight;
		int*        flagint;
	} e;
};

struct menu_t {
	OLumpName             title;
	int                   lastOn;
	int                   indent;
	std::span<menuitem_t> items;
	int                   scrolltop;
	int                   scrollpos;
	void                  (*refreshfunc)(); // Callback func for M_OptResponder
};

struct oldmenuitem_t
{
	// -1 = no cursor here, 1 = ok, 2 = arrows ok
	short		status;

	OLumpName	name;
	char		textname[32];

	// choice = menu item #.
	// if status = 2,
	//	 choice=0:leftarrow,1:rightarrow
	void		(*routine)(int choice);

	// hotkey in menu
	char		alphaKey;
};

struct oldmenu_t
{
	short				numitems;		// # of menu items
	oldmenuitem_t		*menuitems;		// menu items
	void				(*routine)(void);	// draw routine
	short				x;
	short				y;				// x,y of menu
	short				lastOn; 		// last item user was on in menu
};

struct menustack_t
{
	union {
		menu_t *newmenu;
		oldmenu_t *old;
	} menu;
	bool isNewStyle;
	bool drawSkull;
};

extern std::array<value_t, 2> YesNo;
extern std::array<value_t, 2> NoYes;
extern std::array<value_t, 2> OnOff;
extern std::array<value_t, 2> OffOn;
extern std::array<value_t, 3> OnOffAuto;

extern menustack_t MenuStack[16];
extern int MenuStackDepth;

extern menu_t* CurrentMenu;
extern int     CurrentItem;

extern short	 itemOn;
extern oldmenu_t *currentMenu;

size_t M_FindCvarInMenu(cvar_t &cvar, menuitem_t *menu, size_t length);
