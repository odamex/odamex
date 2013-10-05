// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

#define SCRAMBLE(a) \
((((a)&1)<<7) + (((a)&2)<<5) + ((a)&4) + (((a)&8)<<1) \
 + (((a)&16)>>1) + ((a)&32) + (((a)&64)>>5) + (((a)&128)>>7))

#define CHT_GOD				0
#define CHT_NOCLIP			1
#define CHT_NOTARGET		2
#define CHT_CHAINSAW		3
#define CHT_IDKFA			4
#define CHT_IDFA			5
#define CHT_BEHOLDV			6
#define CHT_BEHOLDS			7
#define CHT_BEHOLDI			8
#define CHT_BEHOLDR			9
#define CHT_BEHOLDA			10
#define CHT_BEHOLDL			11
#define CHT_IDDQD			12	// Same as CHT_GOD but sets health
#define CHT_MASSACRE		13
#define CHT_CHASECAM		14
#define CHT_FLY				15

typedef struct
{
	unsigned char *sequence;
	unsigned char *p;
	
} cheatseq_t;

int cht_CheckCheat (cheatseq_t *cht, char key);

void cht_GetParam (cheatseq_t *cht, char *buffer);

// [RH] Functions that actually perform the cheating
class player_s;
void cht_DoCheat (player_s *player, int cheat);
void cht_Give (player_s *player, const char *item);
void cht_Suicide (player_s *player);

#endif


