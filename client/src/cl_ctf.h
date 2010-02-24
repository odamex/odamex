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
//	Client-side CTF Implementation
//
//-----------------------------------------------------------------------------


#ifndef __CL_CTF_H__
#define __CL_CTF_H__

#include "p_local.h"
#include "d_netinf.h"

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
	// Actor when being carried by a player, follows player
	AActor::AActorPtr actor;

	// Integer representation of WHO has each flag (player id)
	size_t flagger;
	
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

//	Externals
extern	DCanvas	   *stbarscreen;
extern	DCanvas	   *screen;

extern	int			ST_WIDTH;
extern	int			ST_HEIGHT;

//	Local Function Prototypes
void	CL_CTFEvent		(void);

void	CTF_CheckFlags	(player_t &player);

void	CTF_DrawHud		(void);
void	CTF_CarryFlag	(player_t &who, flag_t flag);
void	CTF_MoveFlags	(void);
// void	CTF_TossFlag	(void);                                 [ML] 04/4/06: Remove buggy flagtossing
void	CTF_RunTics		(void);

// Client-side CTF Game Data
extern flagdata CTFdata[NUMFLAGS];
extern int TEAMpoints[NUMFLAGS];
extern char *team_names[NUMTEAMS+2];

//	Colors
#define	BLUECOLOR		200
#define	REDCOLOR		176

#endif

