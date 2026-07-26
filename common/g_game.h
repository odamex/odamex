// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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


#pragma once

#include "d_event.h"
#include "d_player.h"

//
// GAME
//
void G_DeathMatchSpawnPlayer(player_t &player);
void G_DoReborn(player_t &player);

void G_DeferedPlayDemo(const char* demo, bool bIsSingleDemo = false);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame(const std::string& name);

void G_DoLoadGame(void);

// Called by M_Responder.
void G_BuildSaveName(std::string &name, int slot);
void G_SaveGame(int slot, std::string_view description);

bool G_RecordDemo(const std::string& mapname, const std::string& basedemoname);

void G_PlayDemo(char* name);
void G_DoPlayDemo(bool justStreamInput = false);
void G_TimeDemo(const char* name);
void G_TestDemo(const char* name);
bool G_CheckDemoStatus(void);
void G_CleanupDemo();

void G_WorldDone(void);

void G_Ticker(void);

void G_LevelTick();
void G_IntermissionTick();
void G_FinaleTick();
void G_DemoScreenTick();

// Core game input responder: finale, key bindings and mouse/joystick movement.
// Invoked at the binding-layer input priority. The demo-attract, HUD and automap
// input slices live in the responders below (each hosted by its own UI layer).
bool G_Responder(const event_t*	ev);

// Demo-attract / title-screen: any non-special key pops the menu. Returns true
// while active (it swallows the event from the rest of the input path).
bool G_DemoAttractResponder(const event_t* ev);

// Net-demo/spectator keys + in-level HUD (chat, status bar). Level/intermission.
bool G_HudResponder(const event_t* ev);

// Automap input, with the gamestate gate that matches its input priority
// (pre-binding when the view is inactive, post-binding when active).
bool G_AutomapResponder(const event_t* ev);

void G_ScreenShot(const char* filename);

void G_PlayerFinishLevel(player_t &player);

void G_AddViewAngle(int yaw);
void G_AddViewPitch(int pitch);
bool G_ShouldIgnoreMouseInput();

extern int mapchange;

inline bool timingdemo; // if true, exit with report on completion
