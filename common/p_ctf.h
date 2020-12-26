// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	 CTF Implementation
//
//-----------------------------------------------------------------------------

#ifndef __P_CTF_H__
#define __P_CTF_H__

#include "d_netinf.h"
#include "p_local.h"

//	Map ID for flags
#define	ID_BLUE_FLAG	5130
#define	ID_RED_FLAG		5131
// Reserve for maintaining the DOOM CTF standard.
//#define ID_NEUTRAL_FLAG	5132
//#define ID_TEAM3_FLAG	5133
//#define ID_TEAM4_FLAG	5134

// flags can only be in one of these states
enum flag_state_t
{
	flag_home,
	flag_dropped,
	flag_carried,

	NUMFLAGSTATES
};

// data associated with a flag
struct flagdata
{
	// Does this flag have a spawn yet?
	bool flaglocated;

	// Actor when being carried by a player, follows player
	AActor::AActorPtr actor;

	// Integer representation of WHO has each flag (player id)
	byte flagger;
	int	pickup_time;

	// True if the flag is currently grabbed for the first time.
	bool firstgrab;

	// Flag locations
	int x, y, z;

	// Flag Timout Counters
	int timeout;

	// True when a flag has been dropped
	flag_state_t state;

	// Used for the blinking flag indicator on the statusbar
	int sb_tick;
};

// events
enum flag_score_t
{
	SCORE_NONE,
	SCORE_REFRESH,
	SCORE_KILL,
	SCORE_BETRAYAL,
	SCORE_GRAB,
	SCORE_FIRSTGRAB,
	SCORE_CARRIERKILL,
	SCORE_RETURN,
	SCORE_CAPTURE,
	SCORE_DROP,
	SCORE_MANUALRETURN,
	NUM_CTF_SCORE
};

//	Network Events
// [CG] I'm aware having CL_* and SV_* functions in common/ is not great, I'll
//      do more work on CTF and team-related things later.
void CL_CTFEvent(void);
void SV_CTFEvent(flag_t f, flag_score_t event, player_t &who);
ItemEquipVal SV_FlagTouch(player_t &player, flag_t f, bool firstgrab);
void SV_SocketTouch(player_t &player, flag_t f);
void CTF_Connect(player_t &player);

//	Internal Events
void CTF_DrawHud(void);
void CTF_CarryFlag(player_t &who, flag_t flag);
void CTF_MoveFlags(void);
void CTF_RunTics(void);
void CTF_SpawnFlag(flag_t f);
void CTF_SpawnDroppedFlag(flag_t f, int x, int y, int z);
void CTF_RememberFlagPos(mapthing2_t *mthing);
void CTF_CheckFlags(player_t &player);
void CTF_Sound(flag_t f, flag_score_t event);
void CTF_Message(flag_t f, flag_score_t event);
// void CTF_TossFlag(player_t &player);  [ML] 04/4/06: Removed buggy flagtoss
// void CTF_SpawnPlayer(player_t &player);	// denis - todo - where's the implementation!?

//	Externals
// EXTERN_CVAR (sv_scorelimit)

// CTF Game Data
extern flagdata CTFdata[NUMFLAGS];
extern int TEAMpoints[NUMFLAGS];
extern const char *team_names[NUMTEAMS+2];

FArchive &operator<< (FArchive &arc, flagdata &flag);
FArchive &operator>> (FArchive &arc, flagdata &flag);

//	Colors
#define	BLUECOLOR		200
#define	REDCOLOR		176

#endif
