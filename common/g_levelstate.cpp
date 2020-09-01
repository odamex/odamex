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
//   Manage state for warmup and complicated gametype flows.
//
//-----------------------------------------------------------------------------

#include "g_levelstate.h"

#include <cmath>

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "d_player.h"
#include "g_game.h"
#include "g_level.h"
#include "i_net.h"
#include "i_system.h"
#include "m_wdlstats.h"

EXTERN_CVAR(g_survival)
EXTERN_CVAR(g_survival_jointimer)
EXTERN_CVAR(sv_countdown)
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_warmup_autostart)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_scorelimit)
EXTERN_CVAR(g_rounds)
EXTERN_CVAR(g_winlimit)

LevelState levelstate;

void SV_SetWinPlayer(byte playerId);

/**
 * @brief State getter.
 */
LevelState::States LevelState::getState() const
{
	return _state;
}

/**
 * @brief Get state as string.
 */
const char* LevelState::getStateString() const
{
	switch (_state)
	{
	case LevelState::WARMUP:
		return "Warmup";
	case LevelState::WARMUP_COUNTDOWN:
		return "Warmup Countdown";
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		return "Warmup Forced Countdown";
	case LevelState::PREROUND_COUNTDOWN:
		return "Pre-round Countdown";
	case LevelState::INGAME:
		return "In-game";
	case LevelState::ENDROUND_COUNTDOWN:
		return "Endgame Countdown";
	case LevelState::ENDGAME_COUNTDOWN:
		return "Endgame Countdown";
	default:
		return "Unknown";
	}
}

/**
 * @brief Countdown getter.
 */
int LevelState::getCountdown() const
{
	if (_state == LevelState::WARMUP || _state == LevelState::INGAME)
		return 0;

	return ceil((_countdown_done_time - level.time) / (float)TICRATE);
}

int LevelState::getRound() const
{
	return _round;
}

/**
 * @brief Amount of time left for a player to join the game.
 */
int LevelState::getJoinTimeLeft() const
{
	int end_time = _ingame_start_time + g_survival_jointimer * TICRATE;
	return ceil((end_time - level.time) / (float)TICRATE);
}

/**
 * @brief Set callback to call when changing state.
 */
void LevelState::setStateCB(LevelState::SetStateCB cb)
{
	_set_state_cb = cb;
}

/**
 * @brief Reset levelstate to "factory defaults" for the level.
 */
void LevelState::reset(level_locals_t& level)
{
	if (level.flags & LEVEL_LOBBYSPECIAL)
	{
		// Lobbies are all warmup, all the time.
		setState(LevelState::WARMUP);
	}
	else if (g_survival && sv_gametype != GM_COOP)
	{
		// We need a warmup state when playing competitive survival modes,
		// so people have a safe period of time to switch teams and join
		// the game without the game trying to end prematurely.
		//
		// However, if we reset while we're playing survival coop, it will
		// destroy players' inventory, so don't do that.
		setState(LevelState::WARMUP);
	}
	else if (sv_warmup)
	{
		// This is the kind of "ready up" warmup used in certain gametypes.
		setState(LevelState::WARMUP);
	}
	else
	{
		// We can go straight in-game.
		setState(LevelState::INGAME);
	}

	_countdown_done_time = 0;
}

/**
 * @brief We want to restart the map, so initialize a countdown that we can't
 *        bail out of.
 */
void LevelState::restart()
{
	setState(LevelState::WARMUP_FORCED_COUNTDOWN);
}

/**
 * @brief Force the start of the game.
 */
void LevelState::forceStart()
{
	// Useless outside of warmup.
	if (_state != LevelState::WARMUP)
		return;

	setState(LevelState::WARMUP_FORCED_COUNTDOWN);
}

/**
 * @brief Start or stop the countdown based on if players are ready or not.
 */
void LevelState::readyToggle()
{
	// No useful work can be done with sv_warmup disabled.
	if (!sv_warmup)
		return;

	// No useful work can be done unless we're either in warmup or taking
	// part in the normal warmup countdown.
	if (_state == LevelState::WARMUP || _state == LevelState::WARMUP_COUNTDOWN)
		return;

	// Check to see if we satisfy our autostart setting.
	size_t ready = P_NumReadyPlayersInGame();
	size_t total = P_NumPlayersInGame();
	size_t needed;

	// We want at least one ingame ready player to start the game.  Servers
	// that start automatically with no ready players are handled by
	// Warmup::tic().
	if (ready == 0 || total == 0)
		return;

	float f_calc = total * sv_warmup_autostart;
	size_t i_calc = (int)floor(f_calc + 0.5f);
	if (f_calc > i_calc - MPEPSILON && f_calc < i_calc + MPEPSILON)
	{
		needed = i_calc + 1;
	}
	needed = (int)ceil(f_calc);

	if (ready >= needed)
	{
		if (_state == LevelState::WARMUP)
			setState(LevelState::WARMUP_COUNTDOWN);
	}
	else
	{
		if (_state == LevelState::WARMUP_COUNTDOWN)
		{
			setState(LevelState::WARMUP);
			SV_BroadcastPrintf(PRINT_HIGH, "Countdown aborted: Player unreadied.\n");
		}
	}
}

/**
 * @brief Depending on if we're using rounds or not, either kick us to an
 *        "end of round" state or just end the game right here.
 */
void LevelState::endRound()
{
	if (g_rounds)
	{
		setState(LevelState::ENDROUND_COUNTDOWN);
	}
	else
	{
		endGame();
	}
}

/**
 * @brief Switch to the endgame state if we're in the proper position to do so.
 */
void LevelState::endGame()
{
	if (sv_gametype == GM_COOP)
	{
		// A normal coop exit bypasses LevelState completely, so if we're
		// here, the mission was a failure and needs to be restarted.
		setState(LevelState::WARMUP);
	}
	else
	{
		setState(LevelState::ENDGAME_COUNTDOWN);
	}
}

/**
 * @brief Handle tic-by-tic maintenance of the level state.
 */
void LevelState::tic()
{
	// [AM] Clients are not authoritative on levelstate.
	if (!serverside)
		return;

	// If there aren't any more active players, go back to warm up mode [tm512 2014/04/08]
	if (_state != LevelState::WARMUP && P_NumPlayersInGame() == 0)
	{
		// [AM] Warmup is for obvious reasons, but survival needs a clean
		//      slate to function properly.
		if (sv_warmup || g_survival)
		{
			setState(LevelState::WARMUP);
			return;
		}
	}

	// Depending on our state, do something.
	switch (_state)
	{
	case LevelState::UNKNOWN:
		I_FatalError("Tried to tic unknown LevelState.\n");
		break;
	case LevelState::PREROUND_COUNTDOWN:
		// Once the timer has run out, start the round without a reset.
		if (level.time >= _countdown_done_time)
		{
			setState(LevelState::INGAME);
			SV_BroadcastPrintf(PRINT_HIGH, "The round has started.\n");
			return;
		}
		break;
	case LevelState::INGAME:
		// Nothing special happens.
		// [AM] Maybe tic the gametype-specific logic here?
		break;
	case LevelState::ENDROUND_COUNTDOWN:
		// Once the timer has run out, reset and go to the next round.
		if (level.time >= _countdown_done_time)
		{
			_round += 1;
			setState(LevelState::INGAME);
			G_DeferedReset();
			return;
		}
		break;
	case LevelState::ENDGAME_COUNTDOWN:
		// Once the timer has run out, go to intermission.
		if (level.time >= _countdown_done_time)
		{
			G_ExitLevel(0, 1);
			return;
		}
		break;
	case LevelState::WARMUP: {
		if (!sv_warmup)
		{
			// We are in here for gametype reasons.  Auto-start once we
			// have enough players.
			PlayerCounts pc = P_PlayerQuery(NULL, 0);
			if (sv_gametype == GM_COOP && pc.result >= 1)
			{
				// Coop needs one player.
				setState(LevelState::WARMUP_FORCED_COUNTDOWN);
				return;
			}
			else if (sv_gametype == GM_DM && pc.result >= 2)
			{
				// DM needs two players.
				setState(LevelState::WARMUP_FORCED_COUNTDOWN);
				return;
			}
			else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			{
				// We need at least one person on at least two different teams.
				int ready = 0;
				for (int i = 0; i < NUMTEAMS; i++)
				{
					if (pc.teamresult[i] > 0)
						ready += 1;
				}

				if (ready >= 2)
				{
					setState(LevelState::WARMUP_FORCED_COUNTDOWN);
					return;
				}
			}
		}

		// If autostart is zeroed out, start immediately.
		if (sv_warmup_autostart == 0.0f)
		{
			setState(LevelState::WARMUP_COUNTDOWN);
			return;
		}
		break;
	}
	case LevelState::WARMUP_COUNTDOWN:
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		// Once the timer has run out, start the game.
		if (level.time >= _countdown_done_time)
		{
			_round += 1;
			setState(LevelState::INGAME);
			G_DeferedFullReset();

			if (g_rounds)
				SV_BroadcastPrintf(PRINT_HIGH, "Round %d has started.\n", _round);
			else
				SV_BroadcastPrintf(PRINT_HIGH, "The match has started.\n");

			return;
		}
		break;
	}
}

/**
 * @brief Serialize level state into a struct.
 *
 * @return Serialzied level state.
 */
SerializedLevelState LevelState::serialize() const
{
	SerializedLevelState serialized;
	serialized.state = _state;
	serialized.countdown_done_time = _countdown_done_time;
	serialized.ingame_start_time = _ingame_start_time;
	serialized.round = _round;
	return serialized;
}

/**
 * @brief Unserialize variables into levelstate.  Usually comes from a server.
 *
 * @param serialized New level state to set.
 */
void LevelState::unserialize(SerializedLevelState serialized)
{
	_state = serialized.state;
	_countdown_done_time = serialized.countdown_done_time;
	_ingame_start_time = serialized.ingame_start_time;
}

/**
 * @brief Set a new state.  You probably should not be making this method
 *        public, since you probably don't want to be able to set any
 *        arbitrary state at any arbitrary point from outside the class.
 *
 * @param new_state The state to set.
 */
void LevelState::setState(LevelState::States new_state)
{
	_state = new_state;

	if (_state == LevelState::WARMUP_COUNTDOWN ||
	    _state == LevelState::WARMUP_FORCED_COUNTDOWN ||
	    _state == LevelState::PREROUND_COUNTDOWN)
	{
		// Most countdowns use the countdown cvar.
		_countdown_done_time = level.time + (sv_countdown.asInt() * TICRATE);
	}
	else if (_state == LevelState::ENDROUND_COUNTDOWN ||
	         _state == LevelState::ENDGAME_COUNTDOWN)
	{
		// Endgame is always two seconds, like the old "shotclock" variable.
		_countdown_done_time = level.time + 2 * TICRATE;
	}
	else
	{
		_countdown_done_time = 0;
	}

	if (_state == LevelState::INGAME)
	{
		_ingame_start_time = level.time;
	}

	// Rounds need to be set to zero here, but don't automatically increment
	// the round counter on PREGAME | INGAME.
	if (_state == LevelState::WARMUP)
	{
		_round = 0;
	}

	if (_set_state_cb)
	{
		_set_state_cb(serialize());
	}
}

BEGIN_COMMAND(forcestart)
{
	::levelstate.forceStart();
}
END_COMMAND(forcestart)

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
 * @brief Check if timeleft should advance.
 */
bool G_CanTimeLeftAdvance()
{
	return ::levelstate.getState() == LevelState::INGAME;
}

/**
 * @brief Check if timelimit should end the game.
 */
void G_TimeCheckEndGame()
{
	if (!sv_timelimit || level.flags & LEVEL_LOBBYSPECIAL) // no time limit in lobby
		return;

	level.timeleft = (int)(sv_timelimit * TICRATE * 60);

	// Don't substract the proper amount of time unless we're actually ingame.
	if (G_CanTimeLeftAdvance())
		level.timeleft -= level.time; // in tics

	if (level.timeleft > 0 || !G_CanEndGame() || gamestate == GS_INTERMISSION)
		return;

	if (P_NumPlayersInGame() == 0)
	{
		// Just end the game and move on.
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

	static PlayerResults pr;

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
 */
void G_RoundsCheckEndGame()
{
	if (!::serverside)
		return;

	// Coop doesn't have rounds to speak of - though perhaps in the future
	// rounds might be used to limit the number of tries a map is attempted.
	if (sv_gametype == GM_COOP)
		return;

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
				::levelstate.endGame();
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
				::levelstate.endGame();
			}
		}
	}
}

VERSION_CONTROL(g_levelstate, "$Id$")
