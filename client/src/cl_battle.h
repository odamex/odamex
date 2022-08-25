// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2022-2022 by DoomBattle.Zone.
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
//   Query host to connect to battle
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

void CL_BattleInit();
void CL_BattleShutdown();
bool CL_IsBattleActive();
bool CL_StartBattle(std::string const& url);
void CL_RestartBattle();
void CL_StopBattle();
void CL_BattleTick();
void CL_SetBattleOver(bool winner, std::string const& hud_markup, std::string const& client_message);
void CL_ClearBattle();
bool CL_IsBattleOver();
bool CL_IsBattleWinner();
std::string const& CL_GetBattleOverHudMarkup();
std::string const& CL_GetBattleOverClientMessage();
