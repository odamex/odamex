// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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


#pragma once

//
// CHEAT SEQUENCE PACKAGE
//

#ifdef CLIENT_APP

struct cheatseq_t
{
	unsigned char* Sequence;
	unsigned char* Pos;
	unsigned char DontCheck;
	unsigned char CurrentArg;
	unsigned char Args[2];
	bool (*Handler)(cheatseq_t*);
};

// keycheat handlers
bool CHEAT_AddKey(cheatseq_t* cheat, unsigned char key, bool* eat);

bool CHEAT_AutoMap(cheatseq_t* cheat);
bool CHEAT_ChangeLevel(cheatseq_t* cheat);
bool CHEAT_IdMyPos(cheatseq_t* cheat);
bool CHEAT_BeholdMenu(cheatseq_t* cheat);
bool CHEAT_ChangeMusic(cheatseq_t* cheat);
bool CHEAT_SetGeneric(cheatseq_t* cheat);
#endif

#define COUNT_CHEATS(l) (sizeof(l) / sizeof(l[0]))


//
// CHEAT TYPES
//
enum ECheatFlags
{
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
	CHT_IDDQD, // Same as CHT_GOD but sets health
	CHT_MASSACRE,
	CHT_CHASECAM,
	CHT_FLY,
	CHT_BUDDHA,
	CHT_MDK,	// He has revolver eyes...
};

// [RH] Functions that actually perform the cheating
class player_s;
void CHEAT_DoCheat (player_s *player, int cheat, bool silentmsg=false);
void CHEAT_GiveTo (player_s *player, const char *item);

// Heretic code (unused)
#if 0
void CHEAT_Suicide (player_s *player);
#endif

bool CHEAT_AreCheatsEnabled();
