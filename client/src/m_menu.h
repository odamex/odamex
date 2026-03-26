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

#include <string>
#include <string_view>

#include "d_event.h"

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
bool M_OpenGeneratedOptionsMenu(const std::string& menuId);
bool M_PrepareGeneratedOptionsMenu(const std::string& menuId, struct menu_s*& menu);
bool M_OpenMenuTarget(const std::string& target);
bool M_OpenMenuEntrypoint(const std::string& name);
const char* M_LocalizedMenuString(const char* key);
const patch_t* M_MenuConfConfiguredPatch(const std::string& name, const char* context);
void M_WarnMenuConf(const std::string& message);
void M_PlayMenuSound(std::string_view role,
                     const std::string* overrideSound = nullptr,
                     std::string_view menuId = std::string_view());

// [RH] Handle keys for options menu
void M_OptResponder(const event_t& ev);

// [RH] Draw options menu
void M_OptDrawer (void);

// [RH] Initialize options menu
void M_OptInit (void);
void M_OpenVideoModeScreen(void);
void M_OpenPlayerSetupScreen(void);

struct menu_s;
void M_SwitchMenu (struct menu_s *menu);
void M_PushNewMenu(struct menu_s* menu, bool drawIndicator = false);

void M_PopMenuStack (void);

// [RH] Called whenever the display mode changes
void M_RefreshModesList ();
int M_MenuCursorOffsetY();
const patch_t* M_MenuCursorPatch();
void M_DrawSlider(int x, int y, float leftval, float rightval, float cur, float step);
void M_DrawColoredSlider(int x, int y, float leftval, float rightval, float cur, argb_t color);

//
// MENU TYPEDEFS
//
typedef enum {
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
} itemtype;

typedef void (*cvarfunc)(cvar_t *cvar, float newval);
typedef void (*voidfunc)(void);
typedef void (*intfunc)(int);

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

typedef struct menu_s {
	OLumpName		title;
	int				lastOn;
	int				numitems;
	int				indent;
	menuitem_t	   *items;
	int				scrolltop;
	int				scrollpos;
	void			(*refreshfunc)();	// Callback func for M_OptResponder
} menu_t;

typedef struct
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
} oldmenuitem_t;

typedef struct oldmenu_s
{
	short				numitems;		// # of menu items
	oldmenuitem_t		*menuitems;		// menu items
	void				(*routine)(void);	// draw routine
	short				x;
	short				y;				// x,y of menu
	short				lastOn; 		// last item user was on in menu
} oldmenu_t;

typedef struct
{
	union {
		menu_t *newmenu;
		oldmenu_t *old;
		int builtin;
	} menu;
	bool isNewStyle;
	bool isBuiltin;
	bool drawIndicator;
} menustack_t;

void M_BuildKeyList(menuitem_t* item, int numitems);
int M_FindCurVal(float cur, value_t* values, int numvals);

extern menustack_t MenuStack[16];
extern int MenuStackDepth;

extern menu_t  *CurrentMenu;
extern int		CurrentItem;
extern bool     CanScrollUp;
extern bool     CanScrollDown;
extern int      VisBottom;

extern short	 itemOn;
extern oldmenu_t *currentMenu;

size_t M_FindCvarInMenu(cvar_t &cvar, menuitem_t *menu, size_t length);
