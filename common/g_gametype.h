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
//   Common gametype-related functionality.
//
//-----------------------------------------------------------------------------

#ifndef __G_GAMETYPE_H__
#define __G_GAMETYPE_H__

#include <string>

#include "teaminfo.h"

enum JoinResult
{
	JOIN_OK,
	JOIN_ENDGAME,
	JOIN_GAMEFULL,
	JOIN_JOINTIMER
};

typedef JoinResult(*JoinTest)();

const std::string& G_GametypeName();
bool G_CanEndGame();
bool G_CanFireWeapon();
JoinResult G_CanJoinGame();
JoinResult G_CanJoinGameStart();
bool G_CanLivesChange();
bool G_CanPickupObjective(team_t team);
bool G_CanReadyToggle();
bool G_CanScoreChange();
bool G_CanShowObituary();
bool G_CanTickGameplay();
bool G_IsDefendingTeam(team_t team);
bool G_IsCoopGame();
bool G_IsFFAGame();
bool G_IsTeamGame();
bool G_IsRoundsGame();
bool G_IsLivesGame();
bool G_UsesWinlimit();
bool G_UsesRoundlimit();
bool G_UsesScorelimit();
bool G_UsesFraglimit();
int G_EndingTic();
void G_AssertValidPlayerCount();
void G_TimeCheckEndGame();
void G_FragsCheckEndGame();
void G_TeamFragsCheckEndGame();
void G_TeamScoreCheckEndGame();
void G_LivesCheckEndGame();
bool G_RoundsShouldEndGame();

#endif // __G_GAMETYPE_H__
