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
//	Cheat code checking.
//
//-----------------------------------------------------------------------------


#ifndef __M_CHEAT_H__
#define __M_CHEAT_H__

//
// CHEAT SEQUENCE PACKAGE
//

#if 0
// Now unused code - left only for DEHacked
#define SCRAMBLE(a) \
((((a)&1)<<7) + (((a)&2)<<5) + ((a)&4) + (((a)&8)<<1) \
 + (((a)&16)>>1) + ((a)&32) + (((a)&64)>>5) + (((a)&128)>>7))
#endif

#define COUNT_CHEATS(l) (sizeof(l) / sizeof(l[0]))

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
};

// [RH] Functions that actually perform the cheating
class player_s;
void CHEAT_DoCheat (player_s *player, int cheat);
void cht_GiveTo (player_s *player, const char *item);
void cht_Suicide (player_s *player);

bool CHEAT_AreCheatsEnabled();

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

#endif


