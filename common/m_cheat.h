// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	Cheat code checking.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#ifndef __M_CHEAT_H__
#define __M_CHEAT_H__

// [RH] Functions that actually perform the cheating
class player_s;

#define COUNT_CHEATS(l) (sizeof(l)/sizeof(l[0]))

enum ECheatFlags {
	CHT_GOD = 0,
	CHT_NOCLIP,
	CHT_NOTARGET,
	CHT_CHAINSAW,
	CHT_IDKFA,
	CHT_IDFA,
	CHT_BEHOLDV,
	CHT_BEHOLDS,
	CHT_BEHOLDI,
	CHT_BEHOLDR,
	CHT_BEHOLDA,
	CHT_BEHOLDL,
	CHT_IDDQD,		// Same as CHT_GOD but sets health
	CHT_MASSACRE,
	CHT_CHASECAM,
	CHT_FLY,
};

struct cheatseq_t
{
	unsigned char *Sequence;
	unsigned char *Pos;
	unsigned char DontCheck;
	unsigned char CurrentArg;
	unsigned char Args[2];
	bool(*Handler)(cheatseq_t *);
};

struct CheatManager {
public:

	int AutoMapCheat;

	bool AddKey(cheatseq_t *cheat, unsigned char key, bool *eat);
	void DoCheat(player_s *player, ECheatFlags cheat, bool silent=false);
	void GiveTo(player_s *player, const char *item);
	bool AreCheatsEnabled(void);
	void SuicidePlayer(player_s *player);

#ifdef CLIENT_APP
	void SendCheatToServer(int cheats);
	void SendGiveCheatToServer(const char *item);
#endif
};

extern CheatManager cht;

#ifdef CLIENT_APP
// keycheat handlers
bool CHEAT_AutoMap(cheatseq_t *cheat);
bool CHEAT_ChangeLevel(cheatseq_t *cheat);
bool CHEAT_IdMyPos(cheatseq_t *cheat);
bool CHEAT_BeholdMenu(cheatseq_t *cheat);
bool CHEAT_ChangeMusic(cheatseq_t *cheat);
bool CHEAT_SetGeneric(cheatseq_t *cheat);
#endif

#endif


