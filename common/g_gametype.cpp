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


#include "odamex.h"

#include "g_gametype.h"

#include "c_dispatch.h"
#include "cmdlib.h"
#include "d_event.h"
#include "g_levelstate.h"
#include "m_wdlstats.h"
#include "svc_message.h"

EXTERN_CVAR(g_gametypename)
EXTERN_CVAR(g_lives)
EXTERN_CVAR(g_sides)
EXTERN_CVAR(g_roundlimit)
EXTERN_CVAR(g_rounds)
EXTERN_CVAR(g_winlimit)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(sv_maxplayersperteam)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_scorelimit)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_warmup)

/**
 * @brief Returns a string containing the name of the gametype.
 *
 * @return String with the gametype name.
 */
const std::string& G_GametypeName()
{
	static std::string name;
	if (!g_gametypename.str().empty())
		name = g_gametypename.str();
	else if (G_IsHordeMode() && g_lives)
		name = "Survival Horde";
	else if (G_IsHordeMode())
		name = "Horde";
	else if (sv_gametype == GM_COOP && g_lives)
		name = "Survival";
	else if (sv_gametype == GM_COOP && ::multiplayer)
		name = "Cooperative";
	else if (sv_gametype == GM_COOP)
		name = "Single-player";
	else if (sv_gametype == GM_DM && g_lives)
		name = "Last Marine Standing";
	else if (sv_gametype == GM_DM && sv_maxplayers <= 2)
		name = "Duel";
	else if (sv_gametype == GM_DM)
		name = "Deathmatch";
	else if (sv_gametype == GM_TEAMDM && g_lives)
		name = "Team Last Marine Standing";
	else if (sv_gametype == GM_TEAMDM)
		name = "Team Deathmatch";
	else if (sv_gametype == GM_CTF && g_sides)
		name = "Attack & Defend CTF";
	else if (sv_gametype == GM_CTF && g_lives)
		name = "LMS Capture The Flag";
	else if (sv_gametype == GM_CTF)
		name = "Capture The Flag";
	return name;
}

/**
 * @brief Check if the round should be allowed to end.
 *
 * @detail This is not an appropriate function to call on level exit.
 */
bool G_CanEndGame()
{
	// Lobbies never end, they're only exited.
	if (::level.flags & LEVEL_LOBBYSPECIAL)
		return false;

	// Can't end the game while the level is being reset.
	if (::gameaction == ga_fullresetlevel || ::gameaction == ga_resetlevel)
		return false;

	// Can't end the game if we're ingame.
	if (::levelstate.getState() != LevelState::INGAME)
		return false;

	return true;
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
 * @brief Check if a player should be allowed to join the game in the middle
 *        of play.
 */
JoinResult G_CanJoinGame()
{
	// Make sure our usual conditions are satisfied.
	JoinResult join = G_CanJoinGameStart();
	if (join != JOIN_OK)
		return join;

	// Join timer checks.
	if (g_lives && ::levelstate.getState() == LevelState::INGAME)
	{
		if (::levelstate.getJoinTimeLeft() <= 0)
			return JOIN_JOINTIMER;
	}

	return JOIN_OK;
}

/**
 * @brief Check if a player should be allowed to join the game.
 *
 *        This function ignores the join timer which is usually a bad idea,
 *        but is vital for the join queue to work.
 */
JoinResult G_CanJoinGameStart()
{
	// Can't join anytime that's not ingame.
	if (::gamestate != GS_LEVEL)
		return JOIN_ENDGAME;

	// Can't join during the endgame.
	if (::levelstate.getState() == LevelState::ENDGAME_COUNTDOWN)
		return JOIN_ENDGAME;

	// Too many people in the game.
	if (P_NumPlayersInGame() >= sv_maxplayers)
		return JOIN_GAMEFULL;

	// Too many people on either team.
	if (G_IsTeamGame() && sv_maxplayersperteam)
	{
		int teamplayers = sv_maxplayersperteam * sv_teamsinplay;
		if (static_cast<int>(P_NumPlayersInGame()) >= teamplayers)
			return JOIN_GAMEFULL;
	}

	return JOIN_OK;
}

/**
 * @brief Check if a player's lives should be allowed to change.
 */
bool G_CanLivesChange()
{
	return ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Check to see if we're allowed to pick up an objective.
 *
 * @param team Team that the objective belongs to.
 */
bool G_CanPickupObjective(team_t team)
{
	if (g_sides && team != ::levelstate.getDefendingTeam())
	{
		return false;
	}
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
 * @brief Check if we should show a FIGHT message on INGAME.
 */
bool G_CanShowFightMessage()
{
	if (demoplayback)
		return false;

	// Don't show a call-to-action when there's nobody ingame to answer.
	PlayerResults pr = PlayerQuery().execute();
	if (pr.count <= 0)
		return false;

	// Multiple rounds should always show them.
	if (G_IsRoundsGame())
		return true;

	// Using warmup makes the levelstate HUD show up a bunch.
	if (::sv_warmup)
		return true;

	return false;
}

/**
 * @brief Check to see if we should show a join timer.
 */
bool G_CanShowJoinTimer()
{
	return ::g_lives > 0 && ::levelstate.getState() == LevelState::INGAME &&
	       ::levelstate.getJoinTimeLeft() > 0;
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
 * @brief Check to see if level is in specific state.
 *
 * @param state Levelstate to check against.
 */
bool G_IsLevelState(LevelState::States state)
{
	return ::levelstate.getState() == state;
}

/**
 * @brief Check if the passed team is on defense.
 *
 * @param team Team to check.
 * @return True if the team is on defense, or if sides aren't enabled.
 */
bool G_IsDefendingTeam(team_t team)
{
	return g_sides == false || ::levelstate.getDefendingTeam() == team;
}

/**
 * @brief Check if gametype is horde or if the map was detected to be Horde
 *        in single-player.
 */
bool G_IsHordeMode()
{
	// Trust when manually set.
	if (::sv_gametype == GM_HORDE)
		return true;

	// In single-player, trust the detected mode.
	if (!::network_game && ::level.detected_gametype == GM_HORDE)
		return true;

	return false;
}

/**
 * @brief Check if the gametype is purely player versus environment, where
 *        the players win and lose as a whole.
 */
bool G_IsCoopGame()
{
	return sv_gametype == GM_COOP || G_IsHordeMode();
}

/**
 * @brief Check if the gametype typically has a single winner.
 */
bool G_IsFFAGame()
{
	return sv_gametype == GM_DM;
}

/**
 * @brief Check if the gametype is Match Duel, a new take on classic Doom duel.
 */
bool G_IsMatchDuelGame()
{
	return sv_gametype == GM_DM && sv_maxplayers == 2 && g_rounds;
}

/**
 * @brief Check if the gametype is made for Duels.
 */
bool G_IsDuelGame()
{
	return sv_gametype == GM_DM && sv_maxplayers == 2;
}

/**
 * @brief Check if the gametype has teams and players can win as a team.
 */
bool G_IsTeamGame()
{
	return sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF;
}

/**
 * @brief Check if the game consists of multiple rounds.
 */
bool G_IsRoundsGame()
{
	if (g_rounds == false)
	{
		// Not turned on.
		return false;
	}

	if (G_IsCoopGame() && (g_lives < 1 || g_roundlimit < 1))
	{
		// Coop game modes only have rounds if they have lives and a limit.
		// Otherwise, rounds can't really do anything.
		return false;
	}

	return true;
}

/**
 * @brief Check if the game uses lives.
 */
bool G_IsLivesGame()
{
	return g_lives > 0;
}

/**
 * @brief Check if the game uses sides.
 */
bool G_IsSidesGame()
{
	return ::g_sides && G_IsTeamGame();
}

/**
 * @brief Check if the gamemode uses coop spawns.
 */
bool G_UsesCoopSpawns()
{
	return ::sv_gametype == GM_COOP;
}

/**
 * @brief Check if the gametype uses winlimit.
 */
bool G_UsesWinlimit()
{
	return g_rounds;
}

/**
 * @brief Check if the gametype uses roundlimit.
 */
bool G_UsesRoundlimit()
{
	return g_rounds;
}

/**
 * @brief Check if the gametype uses scorelimit.
 */
bool G_UsesScorelimit()
{
	return sv_gametype == GM_CTF;
}

/**
 * @brief Check if the gametype uses fraglimit.
 */
bool G_UsesFraglimit()
{
	return sv_gametype == GM_DM || sv_gametype == GM_TEAMDM;
}

/**
 * @brief Calculate the tic that the level ends on.
 */
int G_GetEndingTic()
{
	return sv_timelimit * 60 * TICRATE + 1;
}

/**
 * @brief Drop everything and end the game right now.
*/
void G_EndGame()
{
	::levelstate.setWinner(WinInfo::WIN_EVERYBODY, 0);
	M_CommitWDLLog();
	::levelstate.endGame();
}

/**
 * @brief Assert that we have enough players to continue the game, otherwise
 *        end the game or reset it.
 */
void G_AssertValidPlayerCount()
{
	if (!::serverside)
		return;

	// We don't need to do anything in non-lives gamemodes.
	if (!::g_lives)
		return;

	// In warmup player counts don't matter, and at the end of the game the
	// check is useless.
	if (::levelstate.getState() == LevelState::WARMUP ||
	    ::levelstate.getState() == LevelState::ENDGAME_COUNTDOWN)
		return;

	// Cooperative game modes have slightly different logic.
	if (G_IsCoopGame() && P_NumPlayersInGame() == 0)
	{
		// Survival modes cannot function with no players in the gamne,
		// so the level must be reset at this point.
		::levelstate.reset();
		return;
	}

	bool valid = true;

	if (P_NumPlayersInGame() == 0)
	{
		// Literally nobody is in the game.
		::levelstate.setWinner(WinInfo::WIN_UNKNOWN, 0);
		valid = false;
	}
	else if (sv_gametype == GM_DM)
	{
		// End the game if the player is by themselves.
		PlayerResults pr = PlayerQuery().execute();
		if (pr.count == 1)
		{
			::levelstate.setWinner(WinInfo::WIN_PLAYER, pr.players.front()->id);
			valid = false;
		}
	}
	else if (G_IsTeamGame())
	{
		// End the game if there's only one team with players in it.
		int hasplayers = TEAM_NONE;
		PlayerResults pr = PlayerQuery().execute();
		for (int i = 0; i < NUMTEAMS; i++)
		{
			if (pr.teamTotal[i] > 0)
			{
				// Does the team has more than one players?  If so, the game continues.
				if (hasplayers != TEAM_NONE)
				{
					G_LivesCheckEndGame(); // Check if Whole team is alive
					return;
				}
				hasplayers = i;
			}
		}

		if (hasplayers != TEAM_NONE)
			::levelstate.setWinner(WinInfo::WIN_TEAM, hasplayers);
		else
			::levelstate.setWinner(WinInfo::WIN_UNKNOWN, 0);
		valid = false;
	}

	// Check if we have still players alive
	G_LivesCheckEndGame();

	// If we haven't signaled an invalid state by now, we're cool.
	if (valid == true)
		return;

	// If we're in warmup, back out before we start.  Otherwise, end the game.
	if (::levelstate.getState() == LevelState::WARMUP_COUNTDOWN ||
	    ::levelstate.getState() == LevelState::WARMUP_FORCED_COUNTDOWN)
		::levelstate.reset();
	else
		::levelstate.endGame();
}

static void GiveWins(player_t& player, int wins)
{
	player.roundwins += wins;

	// If we're not a server, we're done.
	if (!::serverside || ::clientside)
		return;

	// Send information about the new round wins to all players.
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;
		MSG_WriteSVC(&it->client.netbuf, SVC_PlayerMembers(player, SVC_PM_SCORE));
	}
}

static void GiveTeamWins(team_t team, int wins)
{
	TeamInfo* info = GetTeamInfo(team);
	if (info->Team >= NUMTEAMS)
		return;
	info->RoundWins += wins;

	// If we're not a server, we're done.
	if (!::serverside || ::clientside)
		return;

	// Send information about the new team round wins to all players.
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;
		MSG_WriteSVC(&it->client.netbuf, SVC_TeamMembers(team));
	}
}

/**
 * @brief Check if timelimit should end the game.
 */
void G_TimeCheckEndGame()
{
	if (!::serverside || !G_CanEndGame())
		return;

	if (sv_timelimit <= 0.0)
		return;

	// Check to see if we have any time left.
	if (G_GetEndingTic() > level.time)
		return;

	// If nobody is in the game, just end the game and move on.
	if (P_NumPlayersInGame() == 0)
	{
		::levelstate.setWinner(WinInfo::WIN_UNKNOWN, 0);
		::levelstate.endRound();
		return;
	}

	if (sv_gametype == GM_DM)
	{
		if (G_IsLivesGame() && ::sv_fraglimit.asInt() == 0)
		{
			// If fraglimit isn't set, our win condition is number of lives left.
			PlayerResults pr = PlayerQuery().sortLives().filterSortMax().execute();
			if (pr.count == 0)
			{
				// Something has seriously gone sideways...
				::levelstate.setWinner(WinInfo::WIN_UNKNOWN, 0);
				::levelstate.endRound();
				return;
			}
			else if (pr.count >= 2)
			{
				SV_BroadcastPrintf(
				    "Time limit hit. Game is a draw on tied lives left!\n");
				::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
			}
			else
			{
				SV_BroadcastPrintf("Time limit hit. Game won by %s on lives left!\n",
				                   pr.players.front()->userinfo.netname.c_str());
				::levelstate.setWinner(WinInfo::WIN_PLAYER, pr.players.front()->id);
			}
		}
		else
		{
			// We have a fraglimit, that's our wincon.
			PlayerResults pr = PlayerQuery().sortFrags().filterSortMax().execute();
			if (pr.count == 0)
			{
				// Something has seriously gone sideways...
				::levelstate.setWinner(WinInfo::WIN_UNKNOWN, 0);
				::levelstate.endRound();
				return;
			}
			else if (pr.count >= 2)
			{
				SV_BroadcastPrintf("Time limit hit. Game is a draw on tied frags!\n");
				::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
			}
			else
			{
				SV_BroadcastPrintf("Time limit hit. Game won by %s on frags!\n",
				                   pr.players.front()->userinfo.netname.c_str());
				::levelstate.setWinner(WinInfo::WIN_PLAYER, pr.players.front()->id);
			}
		}
	}
	else if (G_IsTeamGame())
	{
		if (G_IsSidesGame())
		{
			// Defense always wins in the event of a timeout.
			TeamInfo& ti = *GetTeamInfo(::levelstate.getDefendingTeam());
			GiveTeamWins(ti.Team, 1);
			SV_BroadcastPrintf(
			    "Time limit hit. %s team wins on successful defense!\n",
			    ti.ColorizedTeamName().c_str());
			::levelstate.setWinner(WinInfo::WIN_TEAM, ti.Team);
		}
		else
		{
			const char* limittype = G_UsesScorelimit() ? "score" : "frags";

			if (G_IsLivesGame() &&
			    ((!G_UsesScorelimit() && ::sv_fraglimit.asInt() == 0) ||
			     (G_UsesScorelimit() && ::sv_scorelimit.asInt() == 0)))
			{
				// If wincon isn't set, our win condition is number of lives left.
				TeamsView tv = TeamQuery().sortLives().filterSortMax().execute();

				if (tv.size() != 1)
				{
					SV_BroadcastPrintf("Time limit hit. Game is a draw on tied %s!\n",
					                   limittype);
					::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
				}
				else
				{
					SV_BroadcastPrintf("Time limit hit. %s team wins on %s!\n",
					                   tv.front()->ColorizedTeamName().c_str(),
					                   limittype);
					::levelstate.setWinner(WinInfo::WIN_TEAM, tv.front()->Team);
				}
			}
			else
			{
				// We have a fraglimit or scorelimit, that's our wincon.
				TeamsView tv = TeamQuery().sortScore().filterSortMax().execute();

				if (tv.size() != 1)
				{
					SV_BroadcastPrintf("Time limit hit. Game is a draw on tied %s!\n",
					                   limittype);
					::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
				}
				else
				{
					SV_BroadcastPrintf("Time limit hit. %s team wins on %s!\n",
					                   tv.front()->ColorizedTeamName().c_str(),
					                   limittype);
					::levelstate.setWinner(WinInfo::WIN_TEAM, tv.front()->Team);
				}
			}
		}
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

	PlayerResults pr = PlayerQuery().sortFrags().filterSortMax().execute();
	if (pr.count > 0)
	{
		player_t* top = pr.players.front();
		if (top->fragcount >= sv_fraglimit)
		{
			GiveWins(*top, 1);

			// [ML] 04/4/06: Added !sv_fragexitswitch
			SV_BroadcastPrintf("Frag limit hit. Game won by %s!\n",
			                   top->userinfo.netname.c_str());
			::levelstate.setWinner(WinInfo::WIN_PLAYER, top->id);
			M_CommitWDLLog();
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

	TeamsView tv = TeamQuery().sortScore().filterSortMax().execute();
	if (!tv.empty())
	{
		TeamInfo* team = tv.front();
		if (team->Points >= sv_fraglimit)
		{
			GiveTeamWins(team->Team, 1);
			SV_BroadcastPrintf("Frag limit hit. %s team wins!\n",
			                   team->ColorString.c_str());
			::levelstate.setWinner(WinInfo::WIN_TEAM, team->Team);
			M_CommitWDLLog();
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

	TeamsView tv = TeamQuery().sortScore().filterSortMax().execute();
	if (!tv.empty())
	{
		TeamInfo* team = tv.front();
		if (team->Points >= sv_scorelimit)
		{
			GiveTeamWins(team->Team, 1);
			SV_BroadcastPrintf("Score limit hit. %s team wins!\n",
			                   team->ColorizedTeamName().c_str());
			::levelstate.setWinner(WinInfo::WIN_TEAM, team->Team);
			M_CommitWDLLog();
			::levelstate.endRound();
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

	if (!g_lives || !G_CanEndGame())
		return;

	if (G_IsCoopGame())
	{
		// Everybody losing their lives in coop is a failure.
		PlayerResults pr = PlayerQuery().hasLives().execute();
		if (pr.count == 0)
		{
			SV_BroadcastPrintf("All players have run out of lives.\n");
			::levelstate.setWinner(WinInfo::WIN_NOBODY, 0);
			M_CommitWDLLog();
			::levelstate.endRound();
		}
	}
	else if (sv_gametype == GM_DM)
	{
		// One person being alive is success, nobody alive is a draw.
		PlayerResults pr = PlayerQuery().hasLives().execute();
		if (pr.count == 0)
		{
			SV_BroadcastPrintf("All players have run out of lives.\n");
			::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
			M_CommitWDLLog();
			::levelstate.endRound();
		}
		else if (pr.count == 1)
		{
			GiveWins(*pr.players.front(), 1);
			SV_BroadcastPrintf("%s wins as the last player standing!\n",
			                   pr.players.front()->userinfo.netname.c_str());
			::levelstate.setWinner(WinInfo::WIN_PLAYER, pr.players.front()->id);
			M_CommitWDLLog();
			::levelstate.endRound();
		}

		// Nobody won the game yet - keep going.
	}
	else if (G_IsTeamGame())
	{
		// One person alive on a single team is success, nobody alive is a draw.
		PlayerResults pr = PlayerQuery().hasLives().execute();
		int aliveteams = 0;
		for (int i = 0; i < sv_teamsinplay.asInt(); i++)
		{
			if (pr.teamCount[i] > 0)
				aliveteams += 1;
		}

		// [AM] This end-of-game logic branch is necessary becuase otherwise
		//      going for objectives in CTF would never be worth it.  However,
		//      side-mode needs a special-case because otherwise in games
		//      with scorelimit > 1 the offense can just score once and
		//      turtle.
		if (aliveteams <= 1 && sv_gametype == GM_CTF && !G_IsSidesGame())
		{
			const char* teams = aliveteams == 1 ? "one team" : "no teams";

			TeamsView tv = TeamQuery().filterSortMax().sortScore().execute();
			if (tv.size() == 1)
			{
				GiveTeamWins(tv.front()->Team, 1);
				SV_BroadcastPrintf(
				    "%s team wins for having the highest score with %s left!\n",
				    tv.front()->ColorizedTeamName().c_str(), teams);
				::levelstate.setWinner(WinInfo::WIN_TEAM, tv.front()->Team);
				M_CommitWDLLog();
				::levelstate.endRound();
			}
			else
			{
				SV_BroadcastPrintf("Score is tied with with %s left. Game is a draw!\n",
				                   teams);
				::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
				M_CommitWDLLog();
				::levelstate.endRound();
			}
		}

		if (aliveteams == 0 || pr.count == 0)
		{
			SV_BroadcastPrintf("All teams have run out of lives.\n");
			::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
			M_CommitWDLLog();
			::levelstate.endRound();
		}
		else if (aliveteams == 1)
		{
			team_t team = pr.players.front()->userinfo.team;
			GiveTeamWins(team, 1);
			SV_BroadcastPrintf("%s team wins as the last team standing!\n",
			                   GetTeamInfo(team)->ColorizedTeamName().c_str());
			::levelstate.setWinner(WinInfo::WIN_TEAM, team);
			M_CommitWDLLog();
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

	if (!g_roundlimit && !g_winlimit)
		return false;

	// Coop doesn't have rounds to speak of - though perhaps in the future
	// rounds might be used to limit the number of tries a map is attempted.
	if (G_IsCoopGame())
	{
		if (g_roundlimit && ::levelstate.getRound() >= g_roundlimit)
		{
			SV_BroadcastPrintf(
			    "Round limit hit. Players were unable to finish the level.\n");
			::levelstate.setWinner(WinInfo::WIN_NOBODY, 0);
			return true;
		}
	}
	else if (sv_gametype == GM_DM)
	{
		PlayerResults pr = PlayerQuery().sortWins().filterSortMax().execute();
		if (pr.count == 1 && g_winlimit && pr.players.front()->roundwins >= g_winlimit)
		{
			SV_BroadcastPrintf("Win limit hit. Match won by %s!\n",
			                   pr.players.front()->userinfo.netname.c_str());
			::levelstate.setWinner(WinInfo::WIN_PLAYER, pr.players.front()->id);
			return true;
		}
		else if (pr.count == 1 && g_roundlimit && ::levelstate.getRound() >= g_roundlimit)
		{
			SV_BroadcastPrintf("Round limit hit. Match won by %s!\n",
			                   pr.players.front()->userinfo.netname.c_str());
			::levelstate.setWinner(WinInfo::WIN_PLAYER, pr.players.front()->id);
			return true;
		}
		else if (g_roundlimit && ::levelstate.getRound() >= g_roundlimit)
		{
			SV_BroadcastPrintf("Round limit hit. Game is a draw!\n");
			::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
			return true;
		}
	}
	else if (G_IsTeamGame())
	{
		TeamsView tv = TeamQuery().sortWins().filterSortMax().execute();
		if (tv.size() == 1 && ::g_winlimit && tv.front()->RoundWins >= ::g_winlimit)
		{
			SV_BroadcastPrintf("Win limit hit. %s team wins!\n",
			                   tv.front()->ColorizedTeamName().c_str());
			::levelstate.setWinner(WinInfo::WIN_TEAM, tv.front()->Team);
			return true;
		}
		else if (tv.size() == 1 && ::g_roundlimit &&
		         ::levelstate.getRound() >= ::g_roundlimit)
		{
			SV_BroadcastPrintf("Round limit hit. %s team wins!\n",
			                   tv.front()->ColorizedTeamName().c_str());
			::levelstate.setWinner(WinInfo::WIN_TEAM, tv.front()->Team);
			return true;
		}
		else if (::g_roundlimit && ::levelstate.getRound() >= ::g_roundlimit)
		{
			SV_BroadcastPrintf("Round limit hit. Game is a draw!\n");
			::levelstate.setWinner(WinInfo::WIN_DRAW, 0);
			return true;
		}
	}

	return false;
}
