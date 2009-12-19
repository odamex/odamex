// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Server-side CTF Implementation
//
//-----------------------------------------------------------------------------


#ifndef __SV_CTF_H__
#define __SV_CTF_H__

#include "p_local.h"
#include "d_netinf.h"

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
	
	// Flag actors when not being carried
	AActor::AActorPtr actor;
	
	// Integer representation of WHO has each flag (player id)
	size_t flagger;
	int	pickup_time;
	
	// Flag locations
	int x, y, z;
	
	// Flag Timout Counters
	size_t timeout;
	
	// True when a flag has been dropped
	flag_state_t state;
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
void			SV_CTFEvent				(flag_t f, flag_score_t event, player_t &who);
bool			SV_FlagTouch			(player_t &player, flag_t f, bool firstgrab);
void			SV_SocketTouch			(player_t &player, flag_t f);
void			CTF_Connect				(player_t &player);

//	Internal Events
void			CTF_RunTics				(void);
void			CTF_SpawnFlag			(flag_t f);
void			CTF_SpawnDroppedFlag	(flag_t f, int x, int y, int z);
void			CTF_RememberFlagPos		(mapthing2_t *mthing);
void			CTF_CheckFlags			(player_t &player);
void			CTF_Sound				(flag_t f, flag_score_t event);
//void			CTF_TossFlag			(player_t &player);             [ML] 04/4/06: Removed buggy flagtoss
//void			CTF_SpawnPlayer			(player_t &player);	// denis - todo - where's the implementation!?
mapthing2_t    *CTF_SelectTeamPlaySpot	(player_t &player, int selections);

//	Externals
EXTERN_CVAR (scorelimit)

//	Server-Side CTF Game Data
extern flagdata CTFdata[NUMFLAGS];
extern int TEAMpoints[NUMFLAGS];
extern char *team_names[NUMTEAMS+2];

#endif

