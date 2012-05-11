// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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

#define GI_MAPxx				0x00000001
#define GI_PAGESARERAW			0x00000002
#define GI_SHAREWARE			0x00000004
#define GI_NOLOOPFINALEMUSIC	0x00000008
#define GI_INFOINDEXED			0x00000010
#define GI_MENUHACK				0x00000060
#define GI_MENUHACK_RETAIL		0x00000020
#define GI_MENUHACK_COMMERCIAL	0x00000060
#define GI_NOCRAZYDEATH			0x00000080

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
	int flags;
	char titlePage[8];
	char creditPage1[8];
	char creditPage2[8];
	char titleMusic[8];
	float titleTime;
	float advisoryTime;
	float pageTime;
	char chatSound[16];
	char finaleMusic[8];
	char finaleFlat[8];
	char finalePage1[8];
	char finalePage2[8];
	char finalePage3[8];
	union
	{
		char infoPage[3][8];
		struct
		{
			char basePage[8];
			int numPages;
		} indexed;
	} info;
	const char **quitSounds;
	int maxSwitch;
	char borderFlat[8];
	gameborder_t *border;
} gameinfo_t;

extern gameinfo_t gameinfo;

#endif //__GI_H__

