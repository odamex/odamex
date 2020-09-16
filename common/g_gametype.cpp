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

#include "g_gametype.h"

#include "c_dispatch.h"
#include "cmdlib.h"
#include "g_levelstate.h"
#include "m_wdlstats.h"

EXTERN_CVAR(g_rounds)
EXTERN_CVAR(g_survival)
EXTERN_CVAR(g_winlimit)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_scorelimit)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(sv_timelimit)

void SV_SetWinPlayer(byte playerId);

/**
 * @brief Returns a string containing the name of the gametype.
 *
 * @return String with the gametype name.
 */
const std::string& G_GametypeName()
{
	static std::string name;
	if (sv_gametype == GM_COOP && g_survival)
		name = "Survival";
	else if (sv_gametype == GM_COOP)
		name = "Cooperative";
	else if (sv_gametype == GM_DM && g_survival)
		name = "Last Marine Standing";
	else if (sv_gametype == GM_DM && sv_maxplayers <= 2)
		name = "Duel";
	else if (sv_gametype == GM_DM)
		name = "Deathmatch";
	else if (sv_gametype == GM_TEAMDM && g_survival)
		name = "Team Last Marine Standing";
	else if (sv_gametype == GM_TEAMDM)
		name = "Team Deathmatch";
	else if (sv_gametype == GM_CTF && g_survival)
		name = "LMS Capture The Flag";
	else if (sv_gametype == GM_CTF)
		name = "Capture The Flag";
	return name;
}

/**
 * @brief Check if the round should be allowed to end.
 */
bool G_CanEndGame()
{
	return ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Check if the player should be able to fire their weapon.
 */
bool G_CanFireWeapon()
{
	return ::levelstate.getState() == LevelState::INGAME ||
	       ::levelstate.getState() == LevelState::WARMUP;
}

/**
 * @brief Check if a player should be allowed to join the game.
 */
bool G_CanJoinGame()
{
	if (g_survival && ::levelstate.getState() == LevelState::INGAME)
	{
		// Joining in the middle of a survival round needs special
		// permission from the jointimer.
		if (::levelstate.getJoinTimeLeft() <= 0)
			return false;
		else
			return true;
	}

	return ::levelstate.getState() != LevelState::ENDGAME_COUNTDOWN;
}

/**
 * @brief Check if a player's lives should be allowed to change.
 */
bool G_CanLivesChange()
{
	return ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Check to see if we should allow players to toggle their ready state.
 */
bool G_CanReadyToggle()
{
	return ::levelstate.getState() != LevelState::WARMUP_FORCED_COUNTDOWN &&
	       ::levelstate.getState() != LevelState::ENDGAME_COUNTDOWN;
}

/**
 * @brief Check if a non-win score should be allowed to change.
 */
bool G_CanScoreChange()
{
	return ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Check if obituaries are allowed to be shown.
 */
bool G_CanShowObituary()
{
	return ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Check if we're allowed to "tick" gameplay systems.
 */
bool G_CanTickGameplay()
{
	return ::levelstate.getState() == LevelState::WARMUP ||
	       ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Calculate the tic that the level ends on.
 */
int G_EndingTic()
{
	return sv_timelimit * 60 * TICRATE + 1;
}

/**
 * @brief Check if timelimit should end the game.
 */
void G_TimeCheckEndGame()
{
	if (!::serverside || !G_CanEndGame())
		return;

	if (sv_timelimit <= 0.0 || level.flags & LEVEL_LOBBYSPECIAL) // no time limit in lobby
		return;

	// Check to see if we have any time left.
	if (G_EndingTic() > level.time)
		return;

	// If nobody is in the game, just end the game and move on.
	if (P_NumPlayersInGame() == 0)
	{
		::levelstate.endRound();
		return;
	}

	if (sv_gametype == GM_DM)
	{
		PlayerResults pr;
		P_PlayerQuery(&pr, PQ_MAXFRAGS);
		if (pr.empty())
		{
			// Something has seriously gone sideways...
			::levelstate.endRound();
			return;
		}

		// Need to pick someone for the queue
		SV_SetWinPlayer(pr.front()->id);

		if (pr.size() != 1)
			SV_BroadcastPrintf(PRINT_HIGH, "Time limit hit. Game is a draw!\n");
		else
			SV_BroadcastPrintf(PRINT_HIGH, "Time limit hit. Game won by %s!\n",
			                   pr.front()->userinfo.netname.c_str());
	}
	else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		TeamResults tr;
		P_TeamQuery(&tr, TQ_MAXPOINTS);

		if (tr.size() != 1)
			SV_BroadcastPrintf(PRINT_HIGH, "Time limit hit. Game is a draw!\n");
		else
			SV_BroadcastPrintf(PRINT_HIGH, "Time limit hit. %s team wins!\n",
			                   tr.front()->ColorStringUpper.c_str());
	}

	M_CommitWDLLog();
	::levelstate.endRound();
}

/**
 * @brief Check for an endgame condition on individual frags.
 */
void G_FragsCheckEndGame()
{
	if (!::serverside || !G_CanEndGame())
		return;

	if (sv_fraglimit <= 0.0)
		return;

	PlayerResults pr;
	P_PlayerQuery(&pr, PQ_MAXFRAGS);
	if (!pr.empty())
	{
		player_t* top = pr.front();
		if (top->fragcount >= sv_fraglimit)
		{
			top->roundwins += 1;

			// [ML] 04/4/06: Added !sv_fragexitswitch
			SV_BroadcastPrintf(PRINT_HIGH, "Frag limit hit. Game won by %s!\n",
			                   top->userinfo.netname.c_str());
			SV_SetWinPlayer(top->id);
			::levelstate.endRound();
		}
	}
}

/**
 * @brief Check for an endgame condition on team frags.
 */
void G_TeamFragsCheckEndGame()
{
	if (!::serverside || !G_CanEndGame())
		return;

	if (sv_fraglimit <= 0.0)
		return;

	TeamResults tr;
	P_TeamQuery(&tr, TQ_MAXPOINTS);
	if (!tr.empty())
	{
		TeamInfo* team = tr.front();
		if (team->Points >= sv_fraglimit)
		{
			team->RoundWins += 1;
			SV_BroadcastPrintf(PRINT_HIGH, "Frag limit hit. %s team wins!\n",
			                   team->ColorString.c_str());
			::levelstate.endRound();
			return;
		}
	}
}

/**
 * @brief Check for an endgame condition on team score.
 */
void G_TeamScoreCheckEndGame()
{
	if (!::serverside || !G_CanEndGame())
		return;

	if (sv_scorelimit <= 0.0)
		return;

	TeamResults tr;
	P_TeamQuery(&tr, TQ_MAXPOINTS);
	if (!tr.empty())
	{
		TeamInfo* team = tr.front();
		if (team->Points >= sv_scorelimit)
		{
			team->RoundWins += 1;
			SV_BroadcastPrintf(PRINT_HIGH, "Score limit hit. %s team wins!\n",
			                   team->ColorString.c_str());
			::levelstate.endRound();
			M_CommitWDLLog();
			return;
		}
	}
}

/**
 * @brief Check to see if we should end the game on lives.
 */
void G_LivesCheckEndGame()
{
	if (!::serverside)
		return;

	PlayerResults pr;

	if (!g_survival || !G_CanEndGame())
		return;

	if (sv_gametype == GM_COOP)
	{
		// Everybody losing their lives in coop is a failure.
		if (P_PlayerQuery(NULL, PQ_HASLIVES).result == 0)
		{
			SV_BroadcastPrintf(PRINT_HIGH, "All players have run out of lives.\n");
			::levelstate.endRound();
		}
	}
	else if (sv_gametype == GM_DM)
	{
		pr.clear();

		// One person being alive is success, nobody alive is a draw.
		PlayerCounts pc = P_PlayerQuery(&pr, PQ_HASLIVES);
		if (pc.result == 0 || pr.empty())
		{
			SV_BroadcastPrintf(PRINT_HIGH, "All players have run out of lives.\n");
			::levelstate.endRound();
		}
		else if (pc.result == 1)
		{
			pr.front()->roundwins += 1;
			SV_BroadcastPrintf(PRINT_HIGH, "%s wins as the last player standing!\n",
			                   pr.front()->userinfo.netname.c_str());
			::levelstate.endRound();
		}

		// Nobody won the game yet - keep going.
	}
	else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		// If someone has configured TeamDM improperly, just don't do anything.
		int teamsinplay = sv_teamsinplay.asInt();
		if (teamsinplay < 1 || teamsinplay > NUMTEAMS)
			return;

		pr.clear();

		// One person alive on a single team is success, nobody alive is a draw.
		PlayerCounts pc = P_PlayerQuery(&pr, PQ_HASLIVES);
		int aliveteams = 0;
		for (int i = 0; i < teamsinplay; i++)
		{
			if (pc.teamresult[i] > 0)
				aliveteams += 1;
		}

		if (aliveteams == 0 || pr.empty())
		{
			SV_BroadcastPrintf(PRINT_HIGH, "All teams have run out of lives.\n");
			::levelstate.endRound();
		}
		else if (aliveteams == 1)
		{
			TeamInfo* teamInfo = GetTeamInfo(pr.front()->userinfo.team);
			teamInfo->RoundWins += 1;
			SV_BroadcastPrintf(PRINT_HIGH, "%s team wins as the last team standing!\n",
			                   teamInfo->ColorString.c_str());
			::levelstate.endRound();
		}

		// Nobody won the game yet - keep going.
	}
}

/**
 * @brief Check to see if we should end the game on won rounds...or just rounds total.
 *        Note that this function does not actually end the game - that's the job
 *        of the caller.
 */
bool G_RoundsShouldEndGame()
{
	if (!::serverside)
		return false;

	// Coop doesn't have rounds to speak of - though perhaps in the future
	// rounds might be used to limit the number of tries a map is attempted.
	if (sv_gametype == GM_COOP)
		return true;

	if (sv_gametype == GM_DM)
	{
		PlayerResults pr;
		P_PlayerQuery(&pr, 0);
		for (PlayerResults::const_iterator it = pr.begin(); it != pr.end(); ++it)
		{
			if ((*it)->roundwins >= g_winlimit)
			{
				SV_BroadcastPrintf(PRINT_HIGH, "Win limit hit. Match won by %s!\n",
				                   (*it)->userinfo.netname.c_str());
				return true;
			}
		}
	}
	else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		for (size_t i = 0; i < NUMTEAMS; i++)
		{
			TeamInfo* team = GetTeamInfo((team_t)i);
			if (team->RoundWins >= g_winlimit)
			{
				SV_BroadcastPrintf(PRINT_HIGH, "Win limit hit. %s team wins!\n",
				                   team->ColorString.c_str());
				return true;
			}
		}
	}

	return false;
}

static void LMSHelp()
{
	Printf(PRINT_HIGH,
	       "lms - Configures some settings for a basic game of Last Man Standing\n\n"
	       "Usage:\n"
	       "  ] lms wins <ROUNDS>\n"
	       "  Configure LMS so a player needs to win ROUNDS number of rounds to win the "
	       "game\n\n");
}

BEGIN_COMMAND(lms)
{
	if (argc < 2)
	{
		LMSHelp();
		return;
	}

	if (stricmp(argv[1], "wins") == 0)
	{
		std::string str;
		StrFormat(
		    str,
		    "sv_gametype 1; sv_nomonsters 1; g_survival 1; g_rounds 1; g_winlimit %s",
		    argv[2]);
		Printf(PRINT_HIGH, "Configuring Last Man Standing...\n%s\n", str.c_str());
		AddCommandString(str.c_str());
		return;
	}

	LMSHelp();
}
END_COMMAND(lms)
