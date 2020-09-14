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
//   HUD elements.  Holds 'variable' information for the HUD such
//   as player data, formatted text strings and patches.
//
//-----------------------------------------------------------------------------

#include <algorithm>
#include <sstream>

#include "c_bind.h"
#include "c_cvars.h"
#include "cl_demo.h"
#include "m_fixed.h" // This should probably go into d_netinf.h
#include "d_netinf.h"
#include "d_player.h"
#include "doomstat.h"
#include "g_levelstate.h"
#include "hu_drawers.h"
#include "p_ctf.h"
#include "v_text.h"
#include "i_video.h"
#include "cmdlib.h"

size_t P_NumPlayersInGame(void);
argb_t CL_GetPlayerColor(player_t*);

extern NetDemo netdemo;
extern bool HasBehavior;
extern fixed_t FocalLengthX;


EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_gametype)
EXTERN_CVAR (sv_maxclients)
EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (sv_maxplayersperteam)
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR (g_survival)

EXTERN_CVAR (hud_targetnames)
EXTERN_CVAR (sv_allowtargetnames)
EXTERN_CVAR (hud_timer)

size_t P_NumPlayersInGame();
size_t P_NumPlayersOnTeam(team_t team);

namespace hud {

// Player sorting functions
static bool STACK_ARGS cmpFrags(const player_t *arg1, const player_t *arg2) {
	return arg2->fragcount < arg1->fragcount;
}

static bool STACK_ARGS cmpKills(const player_t *arg1, const player_t *arg2) {
	return arg2->killcount < arg1->killcount;
}

static bool STACK_ARGS cmpPoints (const player_t *arg1, const player_t *arg2) {
	return arg2->points < arg1->points;
}

static bool STACK_ARGS cmpQueue(const player_t *arg1, const player_t *arg2) {
	return arg1->QueuePosition < arg2->QueuePosition;
}


// Returns true if a player is ingame.
bool ingamePlayer(player_t* player) {
	return (player->ingame() && player->spectator == false);
}

// Returns true if a player is ingame and on a specific team
bool inTeamPlayer(player_t* player, const byte team) {
	return (player->ingame() && player->userinfo.team == team && player->spectator == false);
}

// Returns true if a player is a spectator
bool spectatingPlayer(player_t* player) {
	return (!player->ingame() || player->spectator == true);
}

// Returns a sorted player list.  Calculates at most once a gametic.
std::vector<player_t *> sortedPlayers(void) {
	static int sp_tic = -1;
	static std::vector<player_t *> sortedplayers(players.size());
	static std::vector<player_t*> inGame;
	static std::vector<player_t*> specInQueue;
	static std::vector<player_t*> specNormal;

	if (sp_tic == gametic)
		return sortedplayers;

	sortedplayers.clear();
	inGame.clear();
	specInQueue.clear();
	specNormal.clear();

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (!it->ingame())
			continue;

		if (it->spectator)
		{
			if (it->QueuePosition > 0)
				specInQueue.push_back(&(*it));
			else
				specNormal.push_back(&(*it));
		}
		else
		{
			inGame.push_back(&(*it));
		}
	}

	if (sv_gametype == GM_COOP)
	{
		std::sort(inGame.begin(), inGame.end(), cmpKills);
	}
	else
	{
		std::sort(inGame.begin(), inGame.end(), cmpFrags);
		if (sv_gametype == GM_CTF)
			std::sort(inGame.begin(), inGame.end(), cmpPoints);
	}

	std::sort(specInQueue.begin(), specInQueue.end(), cmpQueue);

	for (std::vector<player_t*>::iterator it = inGame.begin(); it != inGame.end(); it++)
		sortedplayers.push_back(*it);

	for (std::vector<player_t*>::iterator it = specInQueue.begin(); it != specInQueue.end(); it++)
		sortedplayers.push_back(*it);

	for (std::vector<player_t*>::iterator it = specNormal.begin(); it != specNormal.end(); it++)
		sortedplayers.push_back(*it);

	sp_tic = gametic;
	return sortedplayers;
}

// Return a text color based on a ping.
int pingTextColor(unsigned short ping) {
	if (ping <= 80) {
		return CR_GREEN;
	} else if (ping <= 160) {
		return CR_GOLD;
	} else if (ping <= 240) {
		return CR_ORANGE;
	}
	return CR_RED;
}

// generate an ordinal suffix from an integer (1 -> "st", 2 -> "nd", etc)
// used in HelpText() to display position in queue
static const char *ordinal(int n)
{
	n %= 100;

	if (n > 10 && n < 20)
		return "th";

	switch (n % 10) {
		case 1: return "st";
		case 2: return "nd";
		case 3: return "rd";
		default: return "th";
	}
}

//// OLD-STYLE "VARIABLES" ////
//
// These just populate variables with data based on conditions and don't give
// you a ton of flexability at the call-site about how they should be drawn.
//
// Please don't add any more of these.  If you want to draw something, make
// a fully-fledged "draw" function where you pass position, alignment, etc.
// without a useless trampoline back to the parent scope.

// Return a "help" string.
std::string HelpText()
{
	bool isGameFull = false;

	if (P_NumPlayersInGame() >= sv_maxplayers)
	{
		isGameFull = true;
	}
	else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		size_t min_players = MAXPLAYERS;
		for (byte i = 0;i < NUMTEAMS;i++)
		{
			size_t players = P_NumPlayersOnTeam((team_t)i);
			if (players < min_players)
				min_players = players;
		}

		if (sv_maxplayersperteam && min_players >= sv_maxplayersperteam)
			isGameFull = true;
	}

	int queuePos = consoleplayer().QueuePosition;

	if (queuePos > 0)
	{
		std::ostringstream ss;
		ss << "Waiting to play - " << queuePos << ordinal(queuePos) << " in line";
		return ss.str();
	}

	if (isGameFull)
	{
		std::string use("Press ");
		use.append(C_GetKeyStringsFromCommand("+use"));
		use.append(" to join the queue");
		return use;
	}

	std::string use("Press ");
	use.append(C_GetKeyStringsFromCommand("+use"));
	use.append(" to join");
	return use;
}

// Return a string that contains the name of the player being spectated,
// or a blank string if you are looking out of your own viewpoint.
std::string SpyPlayerName(int& color) {
	color = CR_GREY;
	player_t *plyr = &displayplayer();

	if (plyr == &consoleplayer()) {
		return "";
	}

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		color = V_GetTextColor(GetTeamInfo(plyr->userinfo.team)->TextColor.c_str());
	}

	return plyr->userinfo.netname;
}

/**
 * @brief Returns a string that contains the current warmup state.
 * 
 * @param out String to put output data into.
 * @param color Color that string should be colored with.
 */
void Warmup(std::string& out, int& color)
{
	color = CR_GREY;
	player_t* dp = &displayplayer();
	player_t* cp = &consoleplayer();

	LevelState::States state = ::levelstate.getState();

	if (state == LevelState::WARMUP && !sv_warmup)
	{
		// Warmup for a game mode.
		if (dp->spectator)
		{
			out = "Warmup: You are spectating";
			return;
		}
		else
		{
			color = CR_GREEN;
			out = "Warmup: Waiting for players";
			return;
		}
	}
	if (state == LevelState::WARMUP)
	{
		if (dp->spectator)
		{
			out = "Warmup: You are spectating";
			return;
		}

		else if (dp->ready)
		{
			color = CR_GREEN;
			if (dp == cp)
			{
				out = "Warmup: You are ready";
				return;
			}
			else
			{
				out = "Warmup: This player is ready";
				return;
			}
		}
		else
		{
			color = CR_RED;
			if (dp == cp)
			{
				StrFormat(out, "Warmup: Press %s to ready up",
				          C_GetKeyStringsFromCommand("ready").c_str());
				return;
			}
			else
			{
				out = "Warmup: This player is not ready";
				return;
			}
		}
	}
	else if (state == LevelState::WARMUP_COUNTDOWN ||
	         state == LevelState::WARMUP_FORCED_COUNTDOWN)
	{
		color = CR_GOLD;
		out = "Match is about to start...";
		return;
	}
	else if (state == LevelState::INGAME && g_survival && cp->spectator)
	{
		// Display the state of the join timer here:
		int time = ::levelstate.getJoinTimeLeft();
		if (time > 0)
		{
			color = CR_GOLD;
			StrFormat(out, "You have %d second(s) to join...", time);
		}
		else
		{
			color = CR_RED;
			StrFormat(out, "Wait for the next round to join.");
		}
	}
}

// Return a string that contains the amount of time left in the map,
// or a blank string if there is no timer needed.  Can also display
// warmup information if it exists.
void Timer(std::string& str, int& color)
{
	color = CR_GREY;

	if (sv_timelimit <= 0.0f)
	{
		str = "";
		return;
	}

	// Do NOT display if in a lobby
	if (level.flags & LEVEL_LOBBYSPECIAL)
	{
		str = "";
		return;
	}

	OTimespan tspan;
	if (hud_timer == 2)
	{
		// Timer counts up.
		TicsToTime(tspan, level.time);
	}
	else if (hud_timer == 1)
	{
		// Timer counts down.
		int timeleft = G_EndingTic() - level.time;
		TicsToTime(tspan, timeleft, true);
	}

	// If we're in the danger zone flip the color.
	int warning = G_EndingTic() - (60 * TICRATE);
	if (level.time > warning)
		color = CR_BRICK;

	if (tspan.hours)
		StrFormat(str, "%02d:%02d:%02d", tspan.hours, tspan.minutes, tspan.seconds);
	else
		StrFormat(str, "%02d:%02d", tspan.minutes, tspan.seconds);
}

void IntermissionTimer(std::string& str)
{
	if (gamestate != GS_INTERMISSION)
	{
		str = "";
		return;
	}

	int timeleft = level.inttimeleft * TICRATE;

	if (timeleft < 0)
		timeleft = 0;

	OTimespan tspan;
	TicsToTime(tspan, level.inttimeleft);

	if (tspan.hours)
		StrFormat(str, "%02d:%02d:%02d", tspan.hours, tspan.minutes, tspan.seconds);
	else
		StrFormat(str, "%02d:%02d", tspan.minutes, tspan.seconds);
}

static int GetWinningTeamPoints(team_t& winningTeam, int& secondPlaceTeamPoints)
{
	int maxPoints = 0;
	winningTeam = TEAM_BLUE;
	secondPlaceTeamPoints = 0;

	for (int i = 0; i < NUMTEAMS; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);
		if (teamInfo->Points > maxPoints)
		{
			secondPlaceTeamPoints = maxPoints;

			maxPoints = teamInfo->Points;
			winningTeam = (team_t)i;
		}
		else if (teamInfo->Points > secondPlaceTeamPoints)
		{
			secondPlaceTeamPoints = teamInfo->Points;
		}
	}

	return maxPoints;
}

// Return a "spread" of personal frags or team points that the
// current player or team is ahead or behind by.
std::string PersonalSpread(int& color) {
	color = CR_BRICK;
	player_t *plyr = &displayplayer();

	if (sv_gametype == GM_DM) {
		// Seek the highest number of frags.
		byte ingame = 0;
		size_t maxother = 0;
		short maxfrags = -32768;
		for (Players::iterator it = players.begin();it != players.end();++it) {
			if (!validplayer(*it)) {
				continue;
			}

			if (!(it->ingame()) || it->spectator) {
				continue;
			}

			if (it->fragcount == maxfrags) {
				maxother++;
			}

			if (it->fragcount > maxfrags) {
				maxfrags = it->fragcount;
				maxother = 0;
			}

			ingame += 1;
		}

		// A spread needs two players to make sense.
		if (ingame <= 1) {
			return "";
		}

		// Return the correct spread.
		if (maxfrags == plyr->fragcount && maxother > 0) {
			// We have the maximum number of frags but we share the
			// throne with someone else.  But at least we can take
			// a little shortcut here.
			color = CR_GREEN;
			return "+0";
		}

		std::ostringstream buffer;
		if (maxfrags == plyr->fragcount) {
			// We have the maximum number of frags.  Calculate how
			// far above the other players we are.
			short nextfrags = -32768;
			for (Players::iterator it = players.begin();it != players.end();++it) {
				if (!validplayer(*it)) {
					continue;
				}

				if (!(it->ingame()) || it->spectator) {
					continue;
				}

				if (it->id == plyr->id) {
					continue;
				}

				if (it->fragcount > nextfrags) {
					nextfrags = it->fragcount;
				}
			}

			color = CR_GREEN;
			buffer << "+" << maxfrags - nextfrags;
			return buffer.str();
		}

		// We are behind the leader.
		buffer << (plyr->fragcount - maxfrags);
		return buffer.str();
	} 
	else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		std::ostringstream buffer;
		team_t winningTeam;
		int secondPlacePoints;
		int winningPoints = GetWinningTeamPoints(winningTeam, secondPlacePoints);

		int diff;
		
		if (plyr->userinfo.team == winningTeam)
			diff = winningPoints - secondPlacePoints;
		else			
			diff = GetTeamInfo(plyr->userinfo.team)->Points - winningPoints;

		if (diff > 0 && plyr->userinfo.team == winningTeam)
		{
			color = CR_GREEN;
			buffer << "+" << winningPoints - secondPlacePoints;
		}
		else
		{
			if (diff < 0)
				color = CR_RED;
			else
				color = CR_GREY;

			buffer << diff;
		}
		return buffer.str();
	}

	// We're not in an appropriate gamemode.
	return "";
}

// Return a string that contains the current team score or personal
// frags of the individual player.  Optionally returns the "limit"
// as well.
std::string PersonalScore(int& color) {
	color = CR_GREY;
	std::ostringstream buffer;
	player_t *plyr = &displayplayer();

	if (sv_gametype == GM_DM) {
		buffer << plyr->fragcount;
		if (sv_fraglimit.asInt() > 0) {
			buffer << "/" << sv_fraglimit.asInt();
		}
	} else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		color = V_GetTextColor(GetTeamInfo(plyr->userinfo.team)->TextColor.c_str());		
		buffer << GetTeamInfo(plyr->userinfo.team)->Points;

		if (sv_gametype == GM_TEAMDM) {
			if (sv_fraglimit.asInt() > 0) {
				buffer << "/" << sv_fraglimit.asInt();
			}
		} else {
			if (sv_scorelimit.asInt() > 0) {
				buffer << "/" << sv_scorelimit.asInt();
			}
		}
	}

	return buffer.str();
}

// Return the amount of time elapsed in a netdemo.
std::string NetdemoElapsed() {
	if (!(netdemo.isPlaying() || netdemo.isPaused())) {
		return "";
	}

	int timeelapsed = netdemo.calculateTimeElapsed();
	uint8_t hours = timeelapsed / 3600;

	timeelapsed -= hours * 3600;
	uint8_t minutes = timeelapsed / 60;

	timeelapsed -= minutes * 60;
	uint8_t seconds = timeelapsed;

	char str[12];
	if (hours) {
		sprintf(str, "%02d:%02d:%02d", hours, minutes, seconds);
	} else {
		sprintf(str, "%02d:%02d", minutes, seconds);
	}

	return str;
}

// Return current map number/total maps in demo
std::string NetdemoMaps() {
	std::vector<int> maptimes = netdemo.getMapChangeTimes();

	// Single map demo
	if (maptimes.empty()) {
		return "";
	}

	std::ostringstream buffer;
	size_t current_map = 1;

	// See if we're in a tic past one of the map change times.
	for (size_t i = 0;i < maptimes.size();i++) {
		if (maptimes[i] <= netdemo.calculateTimeElapsed()) {
			current_map = (i + 1);
		}
	}

	buffer << current_map << "/" << maptimes.size();
	return buffer.str();
}

// Returns clients connected / max clients.
std::string ClientsSplit() {
	std::ostringstream buffer;

	buffer << players.size() << "/" << sv_maxclients;
	return buffer.str();
}

// Returns players connected / max players.
std::string PlayersSplit() {
	std::ostringstream buffer;

	buffer << P_NumPlayersInGame() << "/" << sv_maxplayers;
	return buffer.str();
}

// Returns the number of players on a team
int CountTeamPlayers(byte team)
{
	int count = 0;
	std::vector<player_t*> sortPlayers = sortedPlayers();
	for (size_t i = 0; i < sortPlayers.size(); i++)
	{
		if (inTeamPlayer(sortPlayers[i], team))
			count++;
	}
	return count;
}

// Returns the number of spectators on a team
int CountSpectators()
{
	int count = 0;
	std::vector<player_t*> sortPlayers = sortedPlayers();
	for (size_t i = 0; i < sortPlayers.size(); i++)
	{
		if (spectatingPlayer(sortPlayers[i]))
			count++;
	}
	return count;
}

std::string TeamPlayers(int& color, byte team)
{
	color = V_GetTextColor(GetTeamInfo((team_t)team)->TextColor.c_str());

	std::ostringstream buffer;
	buffer << (short)CountTeamPlayers(team);
	return buffer.str();
}

std::string TeamName(int& color, byte team)
{
	TeamInfo* teamInfo = GetTeamInfo((team_t)team);
	color = V_GetTextColor(teamInfo->TextColor.c_str());
	std::string name = teamInfo->ColorStringUpper;
	return name.append(" TEAM");
}

std::string TeamFrags(int& color, byte team)
{
	if (CountTeamPlayers(team) == 0) {
		color = CR_GREY;
		return "---";
	}

	color = V_GetTextColor(GetTeamInfo((team_t)team)->TextColor.c_str());

	int fragcount = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			fragcount += player->fragcount;
		}
	}

	std::ostringstream buffer;
	buffer << fragcount;
	return buffer.str();
}

std::string TeamPoints(int& color, byte team) {
	if (team >= NUMTEAMS) {
		color = CR_GREY;
		return "---";
	}

	TeamInfo* teamInfo = GetTeamInfo((team_t)team);
	color = V_GetTextColor(teamInfo->TextColor.c_str());

	std::ostringstream buffer;
	buffer << teamInfo->Points;
	return buffer.str();
}

std::string TeamKD(int& color, byte team) {
	if (CountTeamPlayers(team) == 0) {
		color = CR_GREY;
		return "---";
	}

	color = V_GetTextColor(GetTeamInfo((team_t)team)->TextColor.c_str());

	int killcount = 0;
	unsigned int deathcount = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			killcount += player->fragcount;
			deathcount += player->deathcount;
		}
	}

	std::ostringstream buffer;
	buffer.precision(2);
	buffer << std::fixed;
	float kd;
	if (deathcount == 0)
		kd = (float)killcount / 1.0f;
	else {
		kd = (float)killcount / (float)deathcount;
	}
	buffer << kd;
	return buffer.str();
}

std::string TeamPing(int& color, byte team) {
	if (CountTeamPlayers(team) == 0) {
		color = CR_GREY;
		return "---";
	}

	unsigned int ping = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			ping += player->ping;
		}
	}

	ping = ping / CountTeamPlayers(team);
	color = pingTextColor(ping);

	std::ostringstream buffer;
	buffer << ping;
	return buffer.str();
}

//// NEW-STYLE "ELEMENTS" ////

// "Elemants" are actually all-in-one drawable packages.  There are two types
// of elements: plain old elements and ElementArrays, which draw the same thing
// multiple times.  I'm still trying to figure out the interface for these though.

// Draw a list of player colors in the game.  Lines up with player names.
void EAPlayerColors(int x, int y,
                    const unsigned short w, const unsigned short h,
                    const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit)
{
	byte drawn = 0;
	for (size_t i = 0; i < sortedPlayers().size(); i++)
	{
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit)
			break;

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player))
		{
			argb_t playercolor = CL_GetPlayerColor(player);
			hud::Clear(x, y, w, h, scale, x_align, y_align, x_origin, y_origin, playercolor);

			y += h + padding;
			drawn += 1;
		}
	}
}

// Draw a list of player colors on a team.  Lines up with player names.
void EATeamPlayerColors(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const short padding, const short limit,
                        const byte team)
{
	byte drawn = 0;
	for (size_t i = 0; i < sortedPlayers().size(); i++)
	{
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit)
			break;

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team))
		{
			argb_t playercolor = CL_GetPlayerColor(player);
			hud::Clear(x, y, w, h, scale, x_align, y_align, x_origin, y_origin, playercolor);

			y += h + padding;
			drawn += 1;
		}
	}
}

// Draw a list of players in the game.
void EAPlayerNames(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			int color = CR_GREY;
			if (player->id == displayplayer().id) {
				color = CR_GOLD;
			}
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              player->userinfo.netname.c_str(), color, force_opaque);

			y += 7 + padding;
			drawn += 1;
		}
	}
}

static EColorRange GetTeamPlayerColor(player_t* player)
{
	if (sv_gametype == GM_CTF)
	{
		for (int i = 0; i < NUMTEAMS; i++)
		{
			if (player->flags[i])
				return (EColorRange)V_GetTextColor(GetTeamInfo((team_t)i)->TextColor.c_str());
		}
	}

	if (player->ready)
		return CR_GREEN;
	else if (player->id == displayplayer().id)
		return CR_GOLD;

	return CR_GREY;
}

// Draw a list of players on a team.
void EATeamPlayerNames(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			int color = CR_GREY;
			if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			{
				color = GetTeamPlayerColor(player);
			}
			else if(player->id == displayplayer().id) 
			{	
				color = CR_GOLD;
			}
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              player->userinfo.netname.c_str(), color, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of spectators.
void EASpectatorNames(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const short padding, short skip, const short limit,
                      const bool force_opaque) {
	byte drawn = 0;
	std::vector<player_t*> sortPlayers = sortedPlayers();
	for (size_t i = 0; i < sortPlayers.size(); i++)
	{
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit)
			break;

		player_t* player = sortPlayers[i];
		if (spectatingPlayer(player)) {
			if (skip <= 0) {
				int color = CR_GREY;
				if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
					if (player->ready) {
						color = CR_GREEN;
					} else if (player->id == displayplayer().id) {
						color = CR_GOLD;
					}
				} else {
					if (player->id == displayplayer().id) {
						color = CR_GOLD;
					}
				}
				std::string drawName = player->userinfo.netname;
				if (player->QueuePosition)
				{
					std::ostringstream ss;
					ss << (int)player->QueuePosition << ". " << player->userinfo.netname;
					hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
						ss.str().c_str(), color, force_opaque);
				}
				else
				{
					hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
						player->userinfo.netname.c_str(), color, force_opaque);
				}
				y += 7 + padding;
				drawn += 1;
			} else {
				skip -= 1;
			}
		}
	}
}

// Draw a list of lives in the game.  Lines up with player names.
void EAPlayerLives(int x, int y, const float scale, const x_align_t x_align,
                   const y_align_t y_align, const x_align_t x_origin,
                   const y_align_t y_origin, const short padding, const short limit,
                   const bool force_opaque)
{
	byte drawn = 0;
	for (size_t i = 0; i < sortedPlayers().size(); i++)
	{
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit)
		{
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player))
		{
			std::string buffer;
			StrFormat(buffer, "%d", player->lives);

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of lives on a team.  Lines up with player names.
void EATeamPlayerLives(int x, int y, const float scale, const x_align_t x_align,
                       const y_align_t y_align, const x_align_t x_origin,
                       const y_align_t y_origin, const short padding, const short limit,
                       const byte team, const bool force_opaque)
{
	byte drawn = 0;
	for (size_t i = 0; i < sortedPlayers().size(); i++)
	{
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit)
		{
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team))
		{
			std::string buffer;
			StrFormat(buffer, "%d", player->lives);

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of frags in the game.  Lines up with player names.
void EAPlayerFrags(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->fragcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of frags on a team.  Lines up with player names.
void EATeamPlayerFrags(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->fragcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of kills in the game.  Lines up with player names.
void EAPlayerKills(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->killcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of deaths in the game.  Lines up with player names.
void EAPlayerDeaths(int x, int y, const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit,
                    const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->deathcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of points on a team.  Lines up with player names.
void EATeamPlayerPoints(int x, int y, const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const short padding, const short limit,
                        const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->points;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of K/D ratios in the game.  Lines up with player names.
// FIXME: Nothing in the player struct holds player kills, so we use
//        frags instead.
void EAPlayerKD(int x, int y, const float scale,
                const x_align_t x_align, const y_align_t y_align,
                const x_align_t x_origin, const y_align_t y_origin,
                const short padding, const short limit,
                const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer.precision(2);
			buffer << std::fixed;
			float kd;
			if (player->deathcount == 0)
				kd = (float)player->fragcount / 1.0f;
			else {
				kd = (float)player->fragcount / (float)player->deathcount;
			}
			buffer << kd;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of K/D ratios on a team.  Lines up with player names.
// FIXME: Nothing in the player struct holds player kills, so we use
//        frags instead.
void EATeamPlayerKD(int x, int y, const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit,
                    const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer.precision(2);
			buffer << std::fixed;
			float kd;
			if (player->deathcount == 0)
				kd = (float)player->fragcount / 1.0f;
			else {
				kd = (float)player->fragcount / (float)player->deathcount;
			}
			buffer << kd;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of player times in the game (in minutes).  Lines up with
// player names.
void EAPlayerTimes(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->GameTime / 60;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of player times on a team (in minutes).  Lines up with
// player names.
void EATeamPlayerTimes(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->GameTime / 60;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of pings in the game.  Lines up with player names.
void EAPlayerPings(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->ping;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), pingTextColor(player->ping),
			              force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of pings on a team.  Lines up with player names.
void EATeamPlayerPings(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->ping;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), pingTextColor(player->ping),
			              force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of spectator pings.  Lines up with spectators given the same
// skip and limit.
void EASpectatorPings(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const short padding, short skip, const short limit,
                      const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (spectatingPlayer(player)) {
			if (skip <= 0) {
				std::ostringstream buffer;
				buffer << player->ping;

				hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
				              buffer.str().c_str(), pingTextColor(player->ping),
				              force_opaque);
				y += 7 + padding;
				drawn += 1;
			} else {
				skip -= 1;
			}
		}
	}
}

// Target structure.
typedef struct {
	player_t* PlayPtr;
	int Distance;
	int Color;
} TargetInfo_t;

// Draw a list of targets.  Thanks to GhostlyDeath for the original function.
void EATargets(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const short padding, const short limit,
               const bool force_opaque) {
	if (gamestate != GS_LEVEL) {
		// We're not in the game.
		return;
	}

	if(!displayplayer().mo) {
		// For some reason, the displayed player doesn't have a mobj, so
		// if we let this function run we're going to crash.
		return;
	}

	if (!hud_targetnames) {
		// We don't want to see target names.
		return;
	}

	if (!consoleplayer().spectator && !sv_allowtargetnames) {
		// The server doesn't want us to use target names.
		return;
	}

	std::vector<TargetInfo_t> Targets;

	// What players should be drawn?
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->spectator || !(it->mo) || it->mo->health <= 0)
			continue;

		// We don't care about the player whose eyes we are looking through.
		if (&*it == &(displayplayer()))
			continue;

		if (!P_ActorInFOV(displayplayer().mo, it->mo, 45.0f, 512 * FRACUNIT))
			continue;

		// Pick a decent color for the player name.
		int color;
		if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
			// In teamgames, we want to use team colors for targets.
			color = V_GetTextColor(GetTeamInfo(it->userinfo.team)->TextColor.c_str());
		} else {
			color = CR_GREY;
		}

		// Ok, make the temporary player info then add it
		TargetInfo_t temp = {
			&*it,
			P_AproxDistance2(displayplayer().mo, it->mo) >> FRACBITS,
			color
		};
		Targets.push_back(temp);
	}

	// GhostlyDeath -- Now Sort (hopefully I got my selection sort working!)
	for (size_t i = 0; i < Targets.size(); i++) {
		for (size_t j = i + 1; j < Targets.size(); j++) {
			if (Targets[j].Distance < Targets[i].Distance) {
				player_t* PlayPtr = Targets[i].PlayPtr;
				int Distance = Targets[i].Distance;
				int Color = Targets[i].Color;
				Targets[i].PlayPtr = Targets[j].PlayPtr;
				Targets[i].Distance = Targets[j].Distance;
				Targets[i].Color = Targets[j].Color;
				Targets[j].PlayPtr = PlayPtr;
				Targets[j].Distance = Distance;
				Targets[j].Color = Color;
			}
		}
	}

	// [AM] New ElementArray drawing function
	byte drawn = 0;
	for (size_t i = 0;i < Targets.size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		if (Targets[i].PlayPtr == &(consoleplayer())) {
			// You're looking at yourself.
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              "You", Targets[i].Color);
		} else {
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              Targets[i].PlayPtr->userinfo.netname.c_str(),
			              Targets[i].Color);
		}

		y += 7 + padding;
		drawn += 1;
	}
}

}
