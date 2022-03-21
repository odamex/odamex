// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	GI
//
//-----------------------------------------------------------------------------


#pragma once

#include "olumpname.h"
#include "s_sound.h"

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
	OLumpName tl;
	OLumpName t;
	OLumpName tr;
	OLumpName l;
	OLumpName r;
	OLumpName bl;
	OLumpName b;
	OLumpName br;
} gameborder_t;

typedef struct gameinfo_s
{
	int flags;
	OLumpName titlePage;
	OLumpName creditPages[2];
	OLumpName titleMusic;
	int titleTime;
	int advisoryTime;
	bool noLoopFinaleMusic; 
	int pageTime;
	char chatSound[MAX_SNDNAME + 1];
	OLumpName finaleMusic;
	OLumpName finaleFlat;
	OLumpName finalePage[3];
	OLumpName infoPage[3];
	char quitSound[MAX_SNDNAME + 1];
	int maxSwitch;
	OLumpName borderFlat;
	gameborder_t border;
	bool intermissionCounter; 
	OLumpName intermissionMusic;
	int defKickback;
	OLumpName endoom;
	OLumpName pauseSign;
	float gibFactor;
	int telefogHeight;
	int textScreenX;
	int textScreenY;

	std::string titleString;

	gameinfo_s()
		: flags(0)
		, titlePage("")
		, creditPages()
		, titleMusic("")
		, titleTime(0)
		, advisoryTime(0)
		, noLoopFinaleMusic(false)
		, pageTime(0)
		, chatSound()
		, finaleMusic("")
		, finaleFlat("")
		, finalePage()
		, infoPage()
		, quitSound()
		, maxSwitch(1)
		, borderFlat("")
		, border()
		, intermissionCounter(true)
		, intermissionMusic("")
		, defKickback(100)
		, endoom("")
		, pauseSign("")
		, gibFactor(1.f)
		, telefogHeight(0)
		, textScreenX(0)
		, textScreenY(0)
		, titleString("Unknown IWAD")
	{}

} gameinfo_t;

extern gameinfo_t gameinfo;
