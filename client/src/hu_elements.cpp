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


#include "odamex.h"

#include <algorithm>
#include <sstream>

#include "c_bind.h"
#include "cl_demo.h"
#include "m_fixed.h" // This should probably go into d_netinf.h
#include "d_netinf.h"
#include "d_player.h"
#include "g_gametype.h"
#include "g_levelstate.h"
#include "hu_drawers.h"
#include "p_ctf.h"
#include "v_text.h"
#include "i_video.h"
#include "cmdlib.h"

size_t P_NumPlayersInGame(void);
argb_t CL_GetPlayerColor(player_t*);

extern NetDemo netdemo;
extern fixed_t FocalLengthX;
extern byte* Ranges;

extern lumpHandle_t line_leftempty;
extern lumpHandle_t line_leftfull;
extern lumpHandle_t line_centerempty;
extern lumpHandle_t line_centerleft;
extern lumpHandle_t line_centerright;
extern lumpHandle_t line_centerfull;
extern lumpHandle_t line_rightempty;
extern lumpHandle_t line_rightfull;

EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_gametype)
EXTERN_CVAR (sv_maxclients)
EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (sv_maxplayersperteam)
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR (g_lives)
EXTERN_CVAR (g_rounds)
EXTERN_CVAR(g_winlimit)

EXTERN_CVAR(hud_targetnames)
EXTERN_CVAR(hud_targethealth_debug)
EXTERN_CVAR(hud_hidespyname)
EXTERN_CVAR(sv_allowtargetnames)
EXTERN_CVAR(hud_timer)

size_t P_NumPlayersInGame();
size_t P_NumPlayersOnTeam(team_t team);

namespace hud {

// Player sorting functions
static bool cmpFrags(const player_t* arg1, const player_t* arg2)
{
	return arg2->fragcount < arg1->fragcount;
}

static bool cmpDamage(const player_t* arg1, const player_t* arg2)
{
	return arg2->monsterdmgcount < arg1->monsterdmgcount;
}

static bool cmpKills(const player_t* arg1, const player_t* arg2)
{
	return arg2->killcount < arg1->killcount;
}

static bool cmpPoints(const player_t* arg1, const player_t* arg2)
{
	return arg2->points < arg1->points;
}

static bool cmpRoundWins(const player_t* arg1, const player_t* arg2)
{
	return arg2->roundwins < arg1->roundwins;
}

static bool cmpLives(const player_t* arg1, const player_t* arg2)
{
	return arg2->lives < arg1->lives;
}

static bool cmpQueue(const player_t* arg1, const player_t* arg2)
{
	return arg1->QueuePosition < arg2->QueuePosition;
}

// Returns true if a player is ingame.
bool ingamePlayer(player_t* player)
{
	return (player->ingame() && player->spectator == false);
}

// Returns true if a player is ingame and on a specific team
bool inTeamPlayer(player_t* player, const byte team)
{
	return (player->ingame() && player->userinfo.team == team &&
	        player->spectator == false);
}

// Returns true if a player is a spectator
bool spectatingPlayer(player_t* player)
{
	return (!player->ingame() || player->spectator == true);
}

// Returns a sorted player list.  Calculates at most once a gametic.
const PlayersView& sortedPlayers()
{
	static int sp_tic = -1;
	static PlayersView sortedplayers;
	static PlayersView inGame;
	static PlayersView specInQueue;
	static PlayersView specNormal;

	if (sp_tic == ::gametic)
		return sortedplayers;

	sortedplayers.clear();
	inGame.clear();
	specInQueue.clear();
	specNormal.clear();

	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;

		if (it->spectator)
		{
			if (it->QueuePosition > 0)
			{
				specInQueue.push_back(&(*it));
			}
			else
			{
				specNormal.push_back(&(*it));
			}
		}
		else
		{
			inGame.push_back(&(*it));
		}
	}

	if (G_IsCoopGame())
	{
		std::sort(inGame.begin(), inGame.end(), cmpDamage);
	}
	else if (sv_gametype == GM_DM)
	{
		if (G_IsLivesGame())
		{
			if (G_IsRoundsGame())
			{
				std::sort(inGame.begin(), inGame.end(), cmpRoundWins);
			}
			else
			{
				std::sort(inGame.begin(), inGame.end(), cmpLives);
			}
		}
		else
		{
			std::sort(inGame.begin(), inGame.end(), cmpFrags);
		}
	}
	else
	{
		std::sort(inGame.begin(), inGame.end(), cmpFrags);
		if (sv_gametype == GM_CTF)
		{
			std::sort(inGame.begin(), inGame.end(), cmpPoints);
		}
	}

	std::sort(specInQueue.begin(), specInQueue.end(), cmpQueue);

	for (std::vector<player_t*>::iterator it = inGame.begin(); it != inGame.end(); it++)
		sortedplayers.push_back(*it);

	for (std::vector<player_t*>::iterator it = specInQueue.begin();
	     it != specInQueue.end(); it++)
		sortedplayers.push_back(*it);

	for (std::vector<player_t*>::iterator it = specNormal.begin(); it != specNormal.end();
	     it++)
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

/**
 * @brief Return a "help" string.
 */
std::string HelpText()
{
	std::string str;
	int queuePos = consoleplayer().QueuePosition;

	std::string joinmsg;
	JoinResult join = G_CanJoinGame();
	switch (join)
	{
	case JOIN_ENDGAME:
		joinmsg = "Game is ending";
		break;
	case JOIN_GAMEFULL:
		joinmsg = "Game is full";
		break;
	case JOIN_JOINTIMER:
		joinmsg = "Too late to join round";
		break;
	default:
		break;
	}

	if (join != JOIN_OK)
	{
		if (queuePos > 0)
		{
			StrFormat(str, "%s - " TEXTCOLOR_GREEN "%d%s" TEXTCOLOR_NORMAL " in line",
			          joinmsg.c_str(), queuePos, ordinal(queuePos));
			return str;
		}

		StrFormat(str, "%s - Press " TEXTCOLOR_GOLD "%s" TEXTCOLOR_NORMAL " to queue",
		          joinmsg.c_str(), ::Bindings.GetKeynameFromCommand("+use").c_str());
		return str;
	}

	if (G_CanShowJoinTimer())
	{
		StrFormat(str,
		          "Press " TEXTCOLOR_GOLD "%s" TEXTCOLOR_NORMAL
		          " to join - " TEXTCOLOR_GREEN "%d" TEXTCOLOR_NORMAL " secs left",
		          ::Bindings.GetKeynameFromCommand("+use").c_str(),
		          ::levelstate.getJoinTimeLeft());
		return str;
	}

	StrFormat(str, "Press " TEXTCOLOR_GOLD "%s" TEXTCOLOR_NORMAL " to join",
	          ::Bindings.GetKeynameFromCommand("+use").c_str());
	return str;
}

/**
 * @brief Return a string that contains the name of the player being spectated,
 *        or a blank string if you are looking out of your own viewpoint.
 */
std::string SpyPlayerName()
{
	const player_t& plyr = displayplayer();
	if (plyr.id == consoleplayer().id || hud_hidespyname)
	{
		return "";
	}

	const char* color = TEXTCOLOR_GREY;
	if (G_IsTeamGame())
	{
		color = GetTeamInfo(plyr.userinfo.team)->TextColor.c_str();
	}

	std::string str;
	StrFormat(str, "%s%s", color, plyr.userinfo.netname.c_str());
	return str;
}

/**
 * @brief Return a string that contains the amount of time left in the map,
 *        or a blank string if there is no timer needed.  Can also display
 *        warmup information if it exists.
 *
 * @return Time to render.
 */
std::string Timer()
{
	const char* color = TEXTCOLOR_NORMAL;

	if (sv_timelimit <= 0.0f)
	{
		return "";
	}

	// Do NOT display if in a lobby
	if (level.flags & LEVEL_LOBBYSPECIAL)
	{
		return "";
	}


	OTimespan tspan;
	if (::levelstate.getState() == LevelState::WARMUP || 
		::levelstate.getState() == LevelState::WARMUP_COUNTDOWN ||
		::levelstate.getState() == LevelState::WARMUP_FORCED_COUNTDOWN )
	{
		int timeleft = G_GetEndingTic()-1;
		TicsToTime(tspan, timeleft, true);
	}
	else
	{
		if (hud_timer == 2)
		{
			// Timer counts up.
			TicsToTime(tspan, level.time);
		}
		else if (hud_timer == 1)
		{
			// Timer counts down.
			int timeleft = G_GetEndingTic() - level.time;
			TicsToTime(tspan, timeleft, true);
		}

		// If we're in the danger zone flip the color.
		int warning = G_GetEndingTic() - (60 * TICRATE);
		if (level.time > warning)
		{
			color = TEXTCOLOR_BRICK;
		}
	}

	std::string str;
	if (tspan.hours)
	{
		StrFormat(str, "%s%02d:%02d:%02d", color, tspan.hours, tspan.minutes,
		          tspan.seconds);
	}
	else
	{
		StrFormat(str, "%s%02d:%02d", color, tspan.minutes, tspan.seconds);
	}
	return str;
}

std::string IntermissionTimer()
{
	if (gamestate != GS_INTERMISSION)
	{
		return "";
	}

	int timeleft = level.inttimeleft * TICRATE;

	if (timeleft < 0)
		timeleft = 0;

	OTimespan tspan;
	TicsToTime(tspan, timeleft);

	std::string str;
	if (tspan.hours)
	{
		StrFormat(str, "%02d:%02d:%02d", tspan.hours, tspan.minutes, tspan.seconds);
	}
	else
	{
		StrFormat(str, "%02d:%02d", tspan.minutes, tspan.seconds);
	}
	return str;
}

/**
 * @brief Return a "spread" of personal frags or team points that the current
 *        player or team is ahead or behind by.
 * 
 * @return Colored string to render in the HUD.
 */
std::string PersonalSpread()
{
	std::string str;
	player_t& plyr = displayplayer();

	if (G_IsFFAGame())
	{
		// Seek the highest number of rounds or frags.
		PlayerQuery pq = PlayerQuery().filterSortMax();
		if (g_rounds && !G_IsMatchDuelGame())
			pq.sortWins();
		else
			pq.sortFrags();
		PlayerResults max_players = pq.execute();

		if (max_players.total <= 1)
		{
			// With only one player, this is the only reasonable thing to show.
			return TEXTCOLOR_GREY "+0";
		}

		if (max_players.count < 1)
		{
			// How did we get here?
			return TEXTCOLOR_GREY "+0";
		}

		// The interesting thing changes based on rounds.

		int top_number = g_rounds && !G_IsMatchDuelGame()
		                     ? max_players.players.at(0)->roundwins
		                     : max_players.players.at(0)->fragcount;

		int plyr_number =
		    g_rounds && !G_IsMatchDuelGame() ? plyr.roundwins : plyr.fragcount;

		if (max_players.players.size() > 1 && top_number == plyr_number)
		{
			// Share the throne with someone else.
			return TEXTCOLOR_GREY "+0";
		}

		if (max_players.players.at(0)->id == plyr.id)
		{
			// Do a second query without a filter.
			PlayerQuery pq = PlayerQuery().filterSortNotMax();
			if (g_rounds && !G_IsMatchDuelGame())
				pq.sortWins();
			else
				pq.sortFrags();
			PlayerResults other_players = pq.execute();

			// The interesting thing changes based on rounds.
			int next_number = g_rounds && !G_IsMatchDuelGame()
			                      ? other_players.players.at(0)->roundwins
			                      : other_players.players.at(0)->fragcount;

			// Player is on top.
			int diff = plyr_number - next_number;
			StrFormat(str, TEXTCOLOR_GREEN "+%d", diff);
			return str;
		}

		// Player is behind.
		int diff = top_number - plyr_number;
		StrFormat(str, TEXTCOLOR_BRICK "-%d", diff);
		return str;
	}
	else if (G_IsTeamGame())
	{
		const TeamInfo& plyr_team = *GetTeamInfo(plyr.userinfo.team);
		if (plyr_team.Team >= NUMTEAMS)
		{
			// Something has gone sideways.
			return "";
		}

		// Seek the highest amount of wins or score.
		TeamQuery tq = TeamQuery().filterSortMax();
		if (g_rounds)
			tq.sortWins();
		else
			tq.sortScore();
		TeamsView max_teams = tq.execute();

		if (max_teams.size() < 1)
		{
			// How did we get here?
			return TEXTCOLOR_GREY "+0";
		}

		// The interesting thing changes based on rounds.
		int top_number = g_rounds ? max_teams.at(0)->RoundWins : max_teams.at(0)->Points;
		int plyr_number = g_rounds ? plyr_team.RoundWins : plyr_team.Points;

		if (max_teams.size() > 1 && top_number == plyr_number)
		{
			// Share the throne with someone else.
			return TEXTCOLOR_GREY "+0";
		}

		if (max_teams.at(0)->Team == plyr_team.Team)
		{
			// Do a second query without a filter.
			tq = TeamQuery().filterSortNotMax();
			if (g_rounds)
				tq.sortWins();
			else
				tq.sortScore();
			TeamsView other_teams = tq.execute();

			// The interesting thing changes based on rounds.
			int next_number =
			    g_rounds ? other_teams.at(0)->RoundWins : other_teams.at(0)->Points;

			// Player team is on top.
			int diff = plyr_number - next_number;
			StrFormat(str, TEXTCOLOR_GREEN "+%d", diff);
			return str;
		}

		// Player team is behind.
		int diff = top_number - plyr_number;
		StrFormat(str, TEXTCOLOR_BRICK "-%d", diff);
		return str;
	}

	// We're not in an appropriate gamemode.
	return "";
}

/**
 * @brief Return a string that contains the current team score or personal
 *        frags of the individual player.  Optionally returns the "limit"
 *        as well.
 * 
 * @return Colorized string to render to the HUD.
 */
std::string PersonalScore()
{
	std::string str;
	const player_t& plyr = displayplayer();

	if (G_IsTeamGame())
	{
		const TeamInfo& plyr_team = *GetTeamInfo(plyr.userinfo.team);
		if (G_IsRoundsGame())
		{
			if (g_winlimit)
			{
				StrFormat(str, "%s%d/%d", plyr_team.TextColor.c_str(),
				          plyr_team.RoundWins, g_winlimit.asInt());
			}
			else
			{
				StrFormat(str, "%s%d", plyr_team.TextColor.c_str(), plyr.roundwins);
			}
		}
		else
		{
			if (G_UsesFraglimit() && sv_fraglimit > 0)
			{
				StrFormat(str, "%s%d/%d", plyr_team.TextColor.c_str(), plyr_team.Points,
				          sv_fraglimit.asInt());
			}
			else if (!G_UsesFraglimit() && sv_scorelimit > 0)
			{
				StrFormat(str, "%s%d/%d", plyr_team.TextColor.c_str(), plyr_team.Points,
				          sv_scorelimit.asInt());
			}
			else
			{
				StrFormat(str, "%s%d", plyr_team.TextColor.c_str(), plyr.fragcount);
			}
		}
	}
	else if (G_IsFFAGame())
	{
		if (G_IsRoundsGame() && !G_IsMatchDuelGame())
		{
			if (g_winlimit)
			{
				StrFormat(str, TEXTCOLOR_GREY "%d/%d", plyr.roundwins,
				          g_winlimit.asInt());
			}
			else
			{
				StrFormat(str, TEXTCOLOR_GREY "%d", plyr.roundwins);
			}
		}
		else
		{
			if (sv_fraglimit)
			{
				StrFormat(str, TEXTCOLOR_GREY "%d/%d", plyr.fragcount,
				          sv_fraglimit.asInt());
			}
			else
			{
				StrFormat(str, TEXTCOLOR_GREY "%d", plyr.fragcount);
			}
		}
	}

	return str;
}

/**
 * @brief Return a string that contains the current player's round placement
 * expressed in a short, colorized string.
 *
 * @return Colorized string to render to the HUD.
 */
std::string PersonalMatchDuelPlacement()
{
	std::string str;
	const player_t& plyr = displayplayer();

	if (g_winlimit)
	{
		StrFormat(str, TEXTCOLOR_GREY "%d/%d W", plyr.roundwins,
				    g_winlimit.asInt());
	}
	else
	{
		StrFormat(str, TEXTCOLOR_GREY "%d W", plyr.roundwins);
	}

	return str;
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

/**
 * @brief Calculate the number of team lives for the HUD.
 * 
 * @param str Output string buffer.
 * @param color Output color.
 * @param team Team to construct for.
*/
void TeamLives(std::string& str, int& color, byte team)
{
	if (team >= NUMTEAMS)
	{
		color = CR_GREY;
		str = "---";
		return;
	}

	TeamInfo* teamInfo = GetTeamInfo((team_t)team);
	color = V_GetTextColor(teamInfo->TextColor.c_str());

	PlayerResults results = PlayerQuery().hasLives().onTeam(static_cast<team_t>(team)).execute();
	int lives = 0;
	PlayersView::const_iterator it = results.players.begin();
	for (; it != results.players.end(); ++it)
		lives += (*it)->lives;

	StrFormat(str, "%d", lives);
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

void EleBar(const int x, const int y, const int w, const float scale,
            const x_align_t x_align, const y_align_t y_align, const x_align_t x_origin,
            const y_align_t y_origin, const float pct, const EColorRange color)
{
	patch_t* leftempty = W_ResolvePatchHandle(::line_leftempty);
	patch_t* leftfull = W_ResolvePatchHandle(::line_leftfull);
	patch_t* centerempty = W_ResolvePatchHandle(::line_centerempty);
	patch_t* centerleft = W_ResolvePatchHandle(::line_centerleft);
	patch_t* centerright = W_ResolvePatchHandle(::line_centerright);
	patch_t* centerfull = W_ResolvePatchHandle(::line_centerfull);
	patch_t* rightempty = W_ResolvePatchHandle(::line_rightempty);
	patch_t* rightfull = W_ResolvePatchHandle(::line_rightfull);

	// We assume that all parts of the bar are identical width.
	const int UNIT_WIDTH = centerfull->width();

	// Number of things to draw.
	const int UNITS = w / UNIT_WIDTH;

	// Actual width - rounded down from input w.
	const int ACTUAL_WIDTH = UNIT_WIDTH * UNITS;

	if (UNITS <= 2)
	{
		// We want at least one bar of progress, since the far left/right
		// ends are short-circuited.
		return;
	}

	std::vector<patch_t*> lineHandles;
	lineHandles.reserve(UNITS);
	for (int i = 0; i < UNITS; i++)
	{
		if (i == 0)
		{
			if (pct <= (0.0 + FLT_EPSILON))
			{
				// Completely empty.
				lineHandles.push_back(leftempty);
			}
			else
			{
				// The smallest amount of progress.
				lineHandles.push_back(leftfull);
			}
		}
		else if (i == UNITS - 1)
		{
			if (pct >= (1.0 - FLT_EPSILON))
			{
				// Completely full.
				lineHandles.push_back(rightfull);
			}
			else
			{
				// Anything short of 100% - epsilon.
				lineHandles.push_back(rightempty);
			}
		}
		else
		{
			// i0 is short-circuited, so remove it.
			const int idx = i - 1;

			// Each unit 
			const float scaled = pct * (UNITS - 2);

			if (scaled < static_cast<float>(idx) + 0.5f)
			{
				// Empty.
				lineHandles.push_back(centerempty);
			}
			else if (scaled < static_cast<float>(idx) + 1.0f)
			{
				// Half full.
				lineHandles.push_back(centerleft);
			}
			else
			{
				// Full.
				lineHandles.push_back(centerfull);
			}
		}
	}

	// Draw the finished bar.  Passing through the X-Align leads to the bar
	// being drawn backwards, so we have to draw the bar in reverse.
	int drawX;
	if (x_align == hud::X_RIGHT)
	{
		drawX = x + ((lineHandles.size() - 1) * UNIT_WIDTH);
	}
	else
	{
		drawX = x;
	}

	for (size_t i = 0; i < lineHandles.size(); i++)
	{
		patch_t* patch = lineHandles.at(i);
		hud::DrawTranslatedPatch(drawX, y, scale, x_align, y_align, x_origin, y_origin,
		                         patch, ::Ranges + color * 256);

		if (x_align == hud::X_RIGHT)
		{
			drawX -= UNIT_WIDTH;
		}
		else
		{
			drawX += UNIT_WIDTH;
		}
	}
}

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
			if (player->id == displayplayer().id)
			{
				color = CR_GOLD;
			}
			else if (g_lives && player->lives <= 0)
			{
				color = CR_DARKGRAY;
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
	else if (g_lives && player->lives <= 0)
		return CR_DARKGRAY;

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
			if (G_IsTeamGame())
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
				if (G_IsTeamGame()) {
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

// Draw a list of round wins in the game.  Lines up with player names.
void EAPlayerRoundWins(int x, int y, const float scale, const x_align_t x_align,
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
			StrFormat(buffer, "%d", player->roundwins);

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
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
	int frags = 0;

	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];

		if (G_IsRoundsGame() && !G_IsDuelGame())
		{
			frags = player->totalpoints;
		}
		else
		{
			frags = player->fragcount;
		}

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
	int frags = 0;

	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];

		if (G_IsRoundsGame() && G_IsLivesGame())
		{
			frags = player->totalpoints;
		}
		else
		{
			frags = player->fragcount;
		}


		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << frags;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of kills in the game.  Lines up with player names.
void EAPlayerDamage(int x, int y, const float scale, const x_align_t x_align,
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
			std::ostringstream buffer;
			buffer << player->monsterdmgcount;

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
	int deaths = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}
		player_t* player = sortedPlayers()[i];

		if (G_IsRoundsGame() && !G_IsDuelGame())
		{
			deaths = player->totaldeaths;
		}
		else
		{
			deaths = player->deathcount;
		}


		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << deaths;

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
	int points = 0;

	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];

		if (G_IsRoundsGame())
		{
			points = player->totalpoints;
		}
		else
		{
			points = player->points;
		}

		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << points;

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

	const bool netdemoplaying = ::netdemo.isPlaying() || ::netdemo.isPaused();
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

		// The server doesn't want us to see enemy target names.
		if (!netdemoplaying && !::sv_allowtargetnames && !consoleplayer().spectator &&
		    !P_AreTeammates(displayplayer(), *it))
		{
			continue;
		}

		// Pick a decent color for the player name.
		int color;
		if (G_IsTeamGame()) {
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
	for (size_t i = 0; i < Targets.size(); i++)
	{
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit)
		{
			break;
		}

		if (Targets[i].PlayPtr == &(consoleplayer()))
		{
			// You're looking at yourself.
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin, "You",
			              Targets[i].Color);
		}
		else
		{
			// Figure out if we should be showing this player's health.
			std::string nameplate;
			if (netdemoplaying || (::hud_targethealth_debug &&
			                       P_AreTeammates(*Targets[i].PlayPtr, consoleplayer())))
			{
				int health = Targets[i].PlayPtr->health;

				const char* color;
				if (health < 25)
					color = TEXTCOLOR_RED;
				else if (health < 50)
					color = TEXTCOLOR_ORANGE;
				else if (health < 75)
					color = TEXTCOLOR_GOLD;
				else if (health < 110)
					color = TEXTCOLOR_GREEN;
				else
					color = TEXTCOLOR_LIGHTBLUE;

				StrFormat(nameplate, "%s %s%d",
				          Targets[i].PlayPtr->userinfo.netname.c_str(), color, health);
			}
			else
			{
				nameplate = Targets[i].PlayPtr->userinfo.netname;
			}
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              nameplate.c_str(), Targets[i].Color);
		}

		y += 7 + padding;
		drawn += 1;
	}
}

}
