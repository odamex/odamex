// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_game.h 1854 2010-09-05 00:44:20Z ladna $
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
//   Duh.
// 
//-----------------------------------------------------------------------------


#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"
#include "d_player.h"

//
// GAME
//
void G_DeathMatchSpawnPlayer(player_t &player);
void G_DoReborn(player_t &player);

void G_DeferedPlayDemo(const char* demo);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame(char* name);

void G_DoLoadGame(void);

// Called by M_Responder.
void G_BuildSaveName(std::string &name, int slot);
void G_SaveGame(int slot, char* description);

// Only called by startup code.
bool G_RecordDemo(const char* name);

void G_BeginRecording(void);

void G_PlayDemo(char* name);
void G_DoPlayDemo(bool justStreamInput = false);
void G_TimeDemo(char* name);
BOOL G_CheckDemoStatus(void);

void G_WorldDone(void);

void G_Ticker(void);
BOOL G_Responder(event_t*	ev);

void G_ScreenShot(char *filename);

void G_PlayerFinishLevel(player_t &player);

extern std::vector<level_pwad_info_t> wadlevelinfos;
extern std::vector<cluster_info_t> wadclusterinfos;
extern int mapchange;

#endif
