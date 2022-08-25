// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2022-2022 by DoomBattle.Zone
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
//	Heads-up display for battles
//
//-----------------------------------------------------------------------------

#pragma once

typedef class player_s player_t;

void HU_BattleInit();
void HU_DrawBattleInfo(player_t* player);
void HU_DrawBattleHud(player_t* player);
