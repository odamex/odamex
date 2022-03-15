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
//	Head up display
//
//-----------------------------------------------------------------------------


#pragma once

#include "d_event.h"


//
// Globally visible constants.
//
#define HU_FONTSTART	'!' 	// the first font characters
#define HU_FONTEND		'_' 	// the last font characters

// Calculate # of glyphs in font.
#define HU_FONTSIZE 	(HU_FONTEND - HU_FONTSTART + 1) 

void HU_Init();
void STACK_ARGS HU_Shutdown();

void HU_Ticker();
BOOL HU_Responder (event_t* ev);
void HU_Drawer (void);

enum chatmode_t
{
	CHAT_INACTIVE,
	CHAT_NORMAL,
	CHAT_TEAM
};

chatmode_t HU_ChatMode();
void HU_SetChatMode();
void HU_SetTeamChatMode();
void HU_UnsetChatMode();
void HU_ReleaseKeyStates();

void OdamexEffect (int xa, int ya, int xb, int yb);

// [RH] Draw deathmatch scores

class player_s;
void HU_DrawScores (player_s *me);
void HU_DisplayTimer (int x, int y, bool scale = true);
