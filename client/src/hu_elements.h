// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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
//   HUD elements.
//
//-----------------------------------------------------------------------------

#pragma once


namespace hud {

std::string HelpText();
std::string SpyPlayerName();
std::string IntermissionTimer();
std::string Timer();
std::string PersonalSpread();
std::string PersonalScore();
std::string PersonalMatchDuelPlacement();
std::string NetdemoElapsed(void);
std::string NetdemoMaps(void);
std::string ClientsSplit(void);
std::string PlayersSplit(void);
int CountTeamPlayers(byte team);
int CountSpectators(void);
std::string TeamPlayers(int& color, byte team);
std::string TeamName(int& color, byte team);
std::string TeamFrags(int& color, byte team);
std::string TeamPoints(int& color, byte team);
void TeamLives(std::string& str, int& color, byte team);
std::string TeamKD(int& color, byte team);
std::string TeamPing(int& color, byte team);

void EleBar(const int x, const int y, const int w, const float scale,
            const x_align_t x_align, const y_align_t y_align, const x_align_t x_origin,
            const y_align_t y_origin, const float pct, const EColorRange color);
void EAPlayerColors(int x, int y,
                    const unsigned short w, const unsigned short h,
                    const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit);
void EATeamPlayerColors(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const short padding, const short limit,
                        const byte team);
void EAPlayerNames(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque = false);
void EATeamPlayerNames(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque = false);
void EASpectatorNames(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const short padding, short skip, const short limit,
                      const bool force_opaque = false);
void EAPlayerRoundWins(int x, int y, const float scale, const x_align_t x_align,
                       const y_align_t y_align, const x_align_t x_origin,
                       const y_align_t y_origin, const short padding, const short limit,
                       const bool force_opaque);
void EAPlayerLives(int x, int y, const float scale, const x_align_t x_align,
                   const y_align_t y_align, const x_align_t x_origin,
                   const y_align_t y_origin, const short padding, const short limit,
                   const bool force_opaque);
void EATeamPlayerLives(int x, int y, const float scale, const x_align_t x_align,
                       const y_align_t y_align, const x_align_t x_origin,
                       const y_align_t y_origin, const short padding, const short limit,
                       const byte team, const bool force_opaque);
void EAPlayerFrags(int x, int y, const float scale, const x_align_t x_align,
                   const y_align_t y_align, const x_align_t x_origin,
                   const y_align_t y_origin, const short padding, const short limit,
                   const bool force_opaque = false);
void EATeamPlayerFrags(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque = false);
void EAPlayerDamage(int x, int y, const float scale, const x_align_t x_align,
                    const y_align_t y_align, const x_align_t x_origin,
                    const y_align_t y_origin, const short padding, const short limit,
                    const bool force_opaque);
void EAPlayerKills(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque = false);
void EAPlayerDeaths(int x, int y, const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit,
                    const bool force_opaque = false);
void EATeamPlayerPoints(int x, int y, const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const short padding, const short limit,
                        const byte team, const bool force_opaque = false);
void EAPlayerKD(int x, int y, const float scale,
                const x_align_t x_align, const y_align_t y_align,
                const x_align_t x_origin, const y_align_t y_origin,
                const short padding, const short limit,
                const bool force_opaque = false);
void EATeamPlayerKD(int x, int y, const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit,
                    const byte team, const bool force_opaque = false);
void EAPlayerTimes(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque = false);
void EATeamPlayerTimes(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque);
void EAPlayerPings(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque = false);
void EATeamPlayerPings(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque = false);
void EASpectatorPings(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const short padding, short skip, const short limit,
                      const bool force_opaque = false);
void EATargets(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const short padding, const short limit,
               const bool force_opaque = false);
}
