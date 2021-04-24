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
//  Internally used data structures for virtually everything,
//   key definitions, lots of other stuff.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMDEF_H__
#define __DOOMDEF_H__

#include <stdio.h>
#include <cstring>

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

// [AM] These macros exist so function calls to CL/SV functions can exist
//      without having to create a stub for them.

#if defined(CLIENT_APP)
/**
 * @brief The passed expression only appears on the client.
 */
#define CLIENT_ONLY(expr) expr
#else
/**
 * @brief The passed expression only appears on the client.
 */
#define CLIENT_ONLY(expr)
#endif

#if defined(SERVER_APP)
/**
 * @brief The passed expression only appears on the server.
 */
#define SERVER_ONLY(expr) expr
#else
/**
 * @brief The passed expression only appears on the server.
 */
#define SERVER_ONLY(expr)
#endif

// 
// Environment Platform
// 
enum gameplatform_t 
{
	PF_PC,
	PF_XBOX,
	PF_WII,			//	Wii/vWii
	PF_SWITCH,		// 	Nintendo Switch
	PF_UNKNOWN,		//	Unknown platform yet
};

extern gameplatform_t platform;

//
// Global parameters/defines.
//

// Game mode handling - identify IWAD version
//	to handle IWAD dependend animations etc.
enum GameMode_t
{
  shareware,			// DOOM 1 shareware, E1, M9
  registered,			// DOOM 1 registered, E3, M27
  commercial,			// DOOM 2 retail, E1 M34
						// DOOM 2 german edition not handled
  retail,				// DOOM 1 retail, E4, M36
  retail_chex,			// Chex Quest
  retail_bfg,			// Doom 1 BFG Edition
  commercial_bfg,		// Doom 2 BFG Edition
  undetermined			// Well, no IWAD found.
};


// Mission packs - might be useful for TC stuff?
enum GameMission_t
{
  doom, 				// DOOM 1
  doom2,				// DOOM 2
  pack_tnt, 			// TNT mission pack
  pack_plut,			// Plutonia pack
  chex,					// Chex Quest
  retail_freedoom,
  commercial_freedoom,	// FreeDoom
  commercial_hacx,		// HACX
  none
};

// If rangecheck is undefined,
// most parameter validation debugging code will not be compiled
#define RANGECHECK

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS				255
#define MAXPLAYERS_VANILLA		4

// Margin of error used when calculating percentages against player numbers.
#define MPEPSILON				(float)1 / (MAXPLAYERS * 2)

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
	GS_CONNECTING,		// denis - replace the old global "tryingtoconnect"
	GS_CONNECTED,       // [ML] - For that brief time before GS_LEVEL But after GS_CONNECTING should be done

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
	sk_ntm = 0, // no things mode
	sk_baby,
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

enum ItemEquipVal
{
	IEV_NotEquipped, //was not equipped, item should stay
	IEV_EquipStay, //equipped, item should stay
	IEV_EquipRemove //equipped, item should be removed
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

// [ML] 1/5/10: Move input defs to doomkeys.h
#include "doomkeys.h"

// [ML] Default intermission length
#define DEFINTSECS	10

// phares 3/20/98:
//
// Player friction is variable, based on controlling
// linedefs. More friction can create mud, sludge,
// magnetized floors, etc. Less friction can create ice.

#define MORE_FRICTION_MOMENTUM	15000	// mud factor based on momentum
#define ORIG_FRICTION			0xE800	// original value
#define ORIG_FRICTION_FACTOR	2048	// original value
#define FRICTION_FLY			0xeb00

#ifndef __BIG_ENDIAN__
#define MAKE_ID(a,b,c,d)	((a)|((b)<<8)|((c)<<16)|((d)<<24))
#else
#define MAKE_ID(a,b,c,d)	((d)|((c)<<8)|((b)<<16)|((a)<<24))
#endif

static inline void UNMAKE_ID(char* out, uint32_t id)
{
	out[0] = id & 0xFF;
	out[1] = (id >> 8) & 0xFF;
	out[2] = (id >> 16) & 0xFF;
	out[3] = (id >> 24) & 0xFF;
}

//==========================================================================
//
// BinarySearch
//
// Searches an array sorted in ascending order for an element matching
// the desired key.
//
// Template parameters:
//		ClassType -		The class to be searched
//		KeyType -		The type of the key contained in the class
//
// Function parameters:
//		first -			Pointer to the first element in the array
//		max -			The number of elements in the array
//		keyptr -		Pointer to the key member of ClassType
//		key -			The key value to look for
//
// Returns:
//		A pointer to the element with a matching key or NULL if none found.
//==========================================================================

template<class ClassType, class KeyType>
inline
const ClassType *BinarySearch (const ClassType *first, int max,
	const KeyType ClassType::*keyptr, const KeyType key)
{
	int min = 0;
	--max;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		const ClassType *probe = &first[mid];
		const KeyType &seekey = probe->*keyptr;
		if (seekey == key)
		{
			return probe;
		}
		else if (seekey < key)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
// BinarySearchFlexible
//
// THIS DOES NOT WORK RIGHT WITH VISUAL C++
// ONLY ONE COMPTYPE SEEMS TO BE USED FOR ANY INSTANCE OF THIS FUNCTION
// IN A DEBUG BUILD. RELEASE MIGHT BE DIFFERENT--I DIDN'T BOTHER TRYING.
//
// Similar to BinarySearch, except another function is used to copmare
// items in the array.
//
// Template parameters:
//		IndexType -		The type used to index the array (int, size_t, etc.)
//		KeyType -		The type of the key
//		CompType -		A class with a static DoCompare(IndexType, KeyType) method.
//
// Function parameters:
//		max -			The number of elements in the array
//		key -			The key value to look for
//		noIndex -		The value to return if no matching element is found.
//
// Returns:
//		The index of the matching element or noIndex.
//==========================================================================

template<class IndexType, class KeyType, class CompType>
inline
IndexType BinarySearchFlexible (IndexType max, const KeyType key, IndexType noIndex)
{
	IndexType min = 0;
	--max;

	while (min <= max)
	{
		IndexType mid = (min + max) / 2;
		int lexx = CompType::DoCompare (mid, key);
		if (lexx == 0)
		{
			return mid;
		}
		else if (lexx < 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return noIndex;
}


#endif	// __DOOMDEF_H__
