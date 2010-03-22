// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	GI
//
//-----------------------------------------------------------------------------


#ifndef __GI_H__
#define __GI_H__

#include "doomtype.h"
#include "st_stuff.h"

#define GI_MAPxx				0x00000001
#define GI_PAGESARERAW			0x00000002
#define GI_SHAREWARE			0x00000004
#define GI_NOLOOPFINALEMUSIC	0x00000008
#define GI_INFOINDEXED			0x00000010
#define GI_MENUHACK				0x00000060
#define GI_MENUHACK_RETAIL		0x00000020
#define GI_MENUHACK_EXTENDED	0x00000040	// (Heretic)
#define GI_MENUHACK_COMMERCIAL	0x00000060
#define GI_NOCRAZYDEATH			0x00000080

#ifndef EGAMETYPE
#define EGAMETYPE
enum EGameType
{
	GAME_Any	 = 0,
	GAME_Doom	 = 1,
	GAME_Heretic = 2,
	GAME_Hexen	 = 4,
	GAME_Raven	 = 6
};
#endif

typedef struct
{
	byte offset;
	byte size;
	char tl[8];
	char t[8];
	char tr[8];
	char l[8];
	char r[8];
	char bl[8];
	char b[8];
	char br[8];
} gameborder_t;

typedef struct
{
	int flags;					// Game mode flags
	char titlePage[8];			// Opening graphic
	char creditPage1[8];		// First "Credit" page
	char creditPage2[8];		// Second "Credit" page (for demo rotation)
	char titleMusic[8];			// Music lump to play for opening
	float titleTime;			// length of time to show title
	float advisoryTime;			// for heretic (?) len to show advisory
	float pageTime;				// length of general demo state pages
	char chatSound[16];			// Sound lump to play when sending a message
	char consoleBack[8];		// Default console background

	char finaleMusic[8];		// Victory music
	char finaleFlat[8];			// Victory splash background
	char finalePage1[8];		//
	char finalePage2[8];		//
	char finalePage3[8];		//
	union						// Information pages
	{							//
		char infoPage[3][8];
		struct
		{
			char basePage[8];
			int numPages;
		} indexed;
	} info;
	const char **quitSounds;	// Pointer to list of quit sound lumps
	int maxSwitch;
	char borderFlat[8];			// Flat to use when changing screen size
	char endoom[8];				// ENDOOM lump
	stbarfns_t *StatusBar;     	// status bar function set
	gameborder_t *border;
	EGameType gametype;			// Indicates which game this is
} gameinfo_t;

extern gameinfo_t gameinfo;

#endif //__GI_H__
