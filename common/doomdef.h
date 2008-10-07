// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Internally used data structures for virtually everything,
//   key definitions, lots of other stuff.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMDEF_H__
#define __DOOMDEF_H__

#include <stdio.h>
#include <string.h>

// GhostlyDeath -- Can't stand the warnings
#if _MSC_VER == 1200
#pragma warning(disable:4786)
#endif

// GhostlyDeath -- MSVC++ 8+, remove "deprecated" warnings
#if _MSC_VER >= 1400
#pragma warning(disable : 4996)
#endif

//
// The packed attribute forces structures to be packed into the minimum 
// space necessary.  If this is not done, the compiler may align structure
// fields differently to optimise memory access, inflating the overall
// structure size.  It is important to use the packed attribute on certain
// structures where alignment is important, particularly data read/written
// to disk.
//

#ifdef __GNUC__
#define PACKEDATTR __attribute__((packed))
#else
#define PACKEDATTR
#endif


#include "farchive.h"

//
// denis
//
// if(clientside) { do visual effects, sounds, etc }
// if(serverside) { spawn/destroy things, change maps, etc }
//
extern bool clientside, serverside;

// [Nes] - Determines which program the user is running.
enum baseapp_t
{
	client,		// Odamex.exe
	server		// Odasrv.exe
};

extern baseapp_t baseapp;

//
// Global parameters/defines.
//

// Game mode handling - identify IWAD version
//	to handle IWAD dependend animations etc.
enum GameMode_t
{
  shareware,	// DOOM 1 shareware, E1, M9
  registered,	// DOOM 1 registered, E3, M27
  commercial,	// DOOM 2 retail, E1 M34
  // DOOM 2 german edition not handled
  retail,		// DOOM 1 retail, E4, M36
  undetermined	// Well, no IWAD found.

};


// Mission packs - might be useful for TC stuff?
enum GameMission_t
{
  doom, 		// DOOM 1
  doom2,		// DOOM 2
  pack_tnt, 	// TNT mission pack
  pack_plut,	// Plutonia pack
  none
};


// Identify language to use, software localization.
enum Language_t
{
  english,
  french,
  german,
  unknown
};


// If rangecheck is undefined,
// most parameter validation debugging code will not be compiled
#define RANGECHECK

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS				255
#define MAXPLAYERS_VANILLA		4

// State updates, number of tics / second.
#define TICRATE 		35

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo.
enum gamestate_t
{
	GS_LEVEL,
	GS_INTERMISSION,
	GS_FINALE,
	GS_DEMOSCREEN,
	GS_FULLCONSOLE,		// [RH]	Fullscreen console
	GS_HIDECONSOLE,		// [RH] The menu just did something that should hide fs console
	GS_STARTUP,			// [RH] Console is fullscreen, and game is just starting
	GS_DOWNLOAD,		// denis - wad downloading
	GS_CONNECTING,		// denis - replace the old global "tryingtoconnect"

	GS_FORCEWIPE = -1
};

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY				1
#define MTF_NORMAL				2
#define MTF_HARD				4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH				8

// Gravity
#define GRAVITY		FRACUNIT

enum skill_t
{
	sk_baby = 1,
	sk_easy,
	sk_medium,
	sk_hard,
	sk_nightmare
};

//
// Key cards.
//
enum card_t
{
	it_bluecard,
	it_yellowcard,
	it_redcard,
	it_blueskull,
	it_yellowskull,
	it_redskull,

	NUMCARDS,

		// GhostlyDeath <August 31, 2008> -- Before this was not = 0 and when
		// the map is loaded the value is just bitshifted so that the values
		// that were here were incorrect, keyed generalized doors work now
        NoKey = 0,
        RCard,
        BCard,
        YCard,
        RSkull,
        BSkull,
        YSkull,

        AnyKey = 100,
        AllKeys = 101,

        CardIsSkull = 128
};

//
//	[Toke - CTF] CTF Flags
//
enum flag_t
{
	it_blueflag,
	it_redflag,
	it_goldflag, // Remove me in 0.5

	NUMFLAGS
};

inline FArchive &operator<< (FArchive &arc, card_t i)
{
	return arc << (BYTE)i;
}
inline FArchive &operator>> (FArchive &arc, card_t &i)
{
	BYTE in; arc >> in; i = (card_t)in; return arc;
}


// The defined weapons,
//	including a marker indicating
//	user has not changed weapon.
enum weapontype_t
{
	wp_fist,
	wp_pistol,
	wp_shotgun,
	wp_chaingun,
	wp_missile,
	wp_plasma,
	wp_bfg,
	wp_chainsaw,
	wp_supershotgun,

	NUMWEAPONS,

	// No pending weapon change.
	wp_nochange

};

inline FArchive &operator<< (FArchive &arc, weapontype_t i)
{
	return arc << (BYTE)i;
}
inline FArchive &operator>> (FArchive &arc, weapontype_t &i)
{
	BYTE in; arc >> in; i = (weapontype_t)in; return arc;
}


// Ammunition types defined.
enum ammotype_t
{
	am_clip,	// Pistol / chaingun ammo.
	am_shell,	// Shotgun / double barreled shotgun.
	am_cell,	// Plasma rifle, BFG.
	am_misl,	// Missile launcher.
	NUMAMMO,
	am_noammo	// Unlimited for chainsaw / fist.

};

inline FArchive &operator<< (FArchive &arc, ammotype_t i)
{
	return arc << (BYTE)i;
}
inline FArchive &operator>> (FArchive &arc, ammotype_t &i)
{
	BYTE in; arc >> in; i = (ammotype_t)in; return arc;
}


// Power up artifacts.
enum powertype_t
{
	pw_invulnerability,
	pw_strength,
	pw_invisibility,
	pw_ironfeet,
	pw_allmap,
	pw_infrared,
	NUMPOWERS
};

inline FArchive &operator<< (FArchive &arc, powertype_t i)
{
	return arc << (BYTE)i;
}
inline FArchive &operator>> (FArchive &arc, powertype_t &i)
{
	BYTE in; arc >> in; i = (powertype_t)in; return arc;
}


//
// Power up durations, how many tics till expiration.
//
#define INVULNTICS	(30*TICRATE)
#define INVISTICS	(60*TICRATE)
#define INFRATICS	(120*TICRATE)
#define IRONTICS	(60*TICRATE)


//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
// SoM: YES I RE-DID ALL OF THIS BY HAND
#define KEY_RIGHTARROW     0x113
#define KEY_LEFTARROW      0x114
#define KEY_UPARROW        0x111
#define KEY_DOWNARROW      0x112
#define KEY_ESCAPE         0x1B
#define KEY_ENTER          0x0D
#define KEY_SPACE          0x20
#define KEY_TAB 				0x09
#define KEY_F1					0x11A
#define KEY_F2					0x11B
#define KEY_F3					0x11C
#define KEY_F4					0x11D
#define KEY_F5					0x11E
#define KEY_F6					0x11F
#define KEY_F7					0x120
#define KEY_F8					0x121
#define KEY_F9					0x122
#define KEY_F10 				0x123
#define KEY_F11 				0x124
#define KEY_F12 				0x125

#define KEY_BACKSPACE		0x08
#define KEY_PAUSE				0x14

#define KEY_EQUALS			0x3D
#define KEY_MINUS				0x2D

#define KEY_LSHIFT			0x130
#define KEY_LCTRL				0x132
#define KEY_LALT				0x134

#define KEY_RSHIFT         KEY_LSHIFT
#define KEY_RCTRL				KEY_LCTRL
#define KEY_RALT				KEY_LALT

#define KEY_INS 				0x115
#define KEY_DEL 				0x7F
#define KEY_END 				0x117
#define KEY_HOME				0x116
#define KEY_PGUP				0x118
#define KEY_PGDN				0x119

// Joystick and mouse buttons are now sent
// in ev_keyup and ev_keydown instead of
// ev_mouse and ev_joystick. This makes
// binding commands to them *much* simpler.

#define KEY_MOUSE1				0x143
#define KEY_MOUSE2				0x144
#define KEY_MOUSE3				0x145
#define KEY_MOUSE4				0x146
#define KEY_MWHEELUP          0x147
#define KEY_MWHEELDOWN        0x148

#define KEY_JOY1				0x14B
#define KEY_JOY2				0x14C
#define KEY_JOY3				0x14D
#define KEY_JOY4				0x14E
#define KEY_JOY5				0x14F
#define KEY_JOY6				0x150
#define KEY_JOY7				0x151
#define KEY_JOY8				0x152
#define KEY_JOY9				0x153
#define KEY_JOY10				0x154
#define KEY_JOY11				0x155
#define KEY_JOY12				0x156
#define KEY_JOY13				0x157
#define KEY_JOY14				0x158
#define KEY_JOY15				0x159
#define KEY_JOY16				0x15A
#define KEY_JOY17				0x15B
#define KEY_JOY18				0x15C
#define KEY_JOY19				0x15D
#define KEY_JOY20				0x15E
#define KEY_JOY21				0x15F
#define KEY_JOY22				0x160
#define KEY_JOY23				0x161
#define KEY_JOY24				0x162
#define KEY_JOY25				0x163
#define KEY_JOY26				0x164
#define KEY_JOY27				0x165
#define KEY_JOY28				0x166
#define KEY_JOY29				0x167
#define KEY_JOY30				0x168
#define KEY_JOY31				0x169
#define KEY_JOY32				0x16A
#define NUM_KEYS				0x16B

// phares 3/20/98:
//
// Player friction is variable, based on controlling
// linedefs. More friction can create mud, sludge,
// magnetized floors, etc. Less friction can create ice.

#define MORE_FRICTION_MOMENTUM	15000	// mud factor based on momentum
#define ORIG_FRICTION			0xE800	// original value
#define ORIG_FRICTION_FACTOR	2048	// original value

#endif	// __DOOMDEF_H__



