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

EXTERN_CVAR(g_survival)
EXTERN_CVAR(g_survival_jointimer)
EXTERN_CVAR(sv_countdown)
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_warmup_autostart)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(sv_fraglimit)

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
	case LevelState::INGAME:
		return "In-game";
	case LevelState::PREGAME:
		return "Countdown";
	case LevelState::ENDGAME:
		return "Endgame";
	case LevelState::WARMUP:
		return "Warmup";
	case LevelState::WARMUP_COUNTDOWN:
		return "Warmup Countdown";
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		return "Warmup Forced Countdown";
	default:
		return "Unknown";
	}
}

/**
 * @brief Countdown getter.
 */
short LevelState::getCountdown() const
{
	if (_state != LevelState::PREGAME && _state != LevelState::WARMUP_COUNTDOWN &&
	    _state != LevelState::WARMUP_FORCED_COUNTDOWN)
		return 0;

	return ceil((_countdown_done_time - level.time) / (float)TICRATE);
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
 * @brief Check if the round should be allowed to end.
 */
bool LevelState::canEndGame() const
{
	return _state == LevelState::INGAME;
}

/**
 * @brief Check if the player should be able to fire their weapon.
 */
bool LevelState::canFireWeapon() const
{
	return _state == LevelState::INGAME || _state == LevelState::WARMUP;
}

/**
 * @brief Check if a player should be allowed to join the game.
 */
bool LevelState::canJoinGame() const
{
	if (g_survival && LevelState::INGAME)
	{
		// Joining in the middle of a survival round needs special
		// permission from the jointimer.
		if (getJoinTimeLeft() <= 0)
			return false;
		else
			return true;
	}

	return _state != LevelState::ENDGAME;
}

/**
 * @brief Check if a player's lives should be allowed to change.
 */
bool LevelState::canLivesChange() const
{
	return _state == LevelState::INGAME;
}

/**
 * @brief Check to see if we should allow players to toggle their ready state.
 */
bool LevelState::canReadyToggle() const
{
	return _state == LevelState::INGAME || _state == LevelState::PREGAME ||
	       _state == LevelState::WARMUP || _state == LevelState::WARMUP_COUNTDOWN;
}

/**
 * @brief Check if a score should be allowed to change.
 */
bool LevelState::canScoreChange() const
{
	return _state == LevelState::INGAME;
}

/**
 * @brief Check if obituaries are allowed to be shown.
 */
bool LevelState::canShowObituary() const
{
	return _state == LevelState::INGAME;
}

/**
 * @brief Check if we're allowed to "tick" gameplay systems.
 */
bool LevelState::canTickGameplay() const
{
	return _state == LevelState::WARMUP || _state == LevelState::INGAME;
}

/**
 * @brief Check if timeleft should advance.
 */
bool LevelState::canTimeLeftAdvance() const
{
	return _state == LevelState::INGAME;
}

/**
 * @brief Set callback to call when changing state.
 */
void LevelState::setStateCB(LevelState::SetStateCB cb)
{
	_set_state_cb = cb;
}

/**
 * @brief Reset warmup to "factory defaults" for the level.
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
 * @brief Switch to the endgame state if we're in the proper position to do so.
 */
void LevelState::endGame()
{
	if (sv_gametype == GM_COOP)
	{
		// A normal coop exit bypasses LevelState completely, so if we're
		// here, the mission was a failure and needs to be restarted.
		setState(LevelState::WARMUP_FORCED_COUNTDOWN);
	}
	else
	{
		setState(LevelState::ENDGAME);
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
	if (sv_warmup && _state != LevelState::WARMUP && P_NumPlayersInGame() == 0)
	{
		setState(LevelState::WARMUP);
		return;
	}

	// Depending on our state, do something.
	switch (_state)
	{
	case LevelState::UNKNOWN:
		I_FatalError("Tried to tic unknown LevelState.\n");
		break;
	case LevelState::INGAME:
		// Nothing special happens.
		break;
	case LevelState::PREGAME:
		// Once the timer has run out, start the round.
		if (level.time >= _countdown_done_time)
		{
			setState(LevelState::INGAME);
			SV_BroadcastPrintf(PRINT_HIGH, "The round has started.\n");
			return;
		}
		break;
	case LevelState::ENDGAME:
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
	case LevelState::WARMUP_FORCED_COUNTDOWN: {
		// Once the timer has run out, start the game.
		if (level.time >= _countdown_done_time)
		{
			setState(LevelState::INGAME);
			G_DeferedFullReset();
			SV_BroadcastPrintf(PRINT_HIGH, "The match has started.\n");
			return;
		}
		break;
	}
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

	if (_state == LevelState::PREGAME || _state == LevelState::WARMUP_COUNTDOWN ||
	    _state == LevelState::WARMUP_FORCED_COUNTDOWN)
	{
		// Most countdowns use the countdown cvar.
		_countdown_done_time = level.time + (sv_countdown.asInt() * TICRATE);
	}
	else if (_state == LevelState::ENDGAME)
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
 * @brief Check to see if se whould end the game on frags.
 */
void G_FragsCheckEndGame()
{
	if (!::serverside || !::levelstate.canEndGame())
		return;

	if (sv_fraglimit <= 0.0)
		return;

	PlayerResults pr;
	P_PlayerQuery(&pr, 0);
	for (PlayerResults::const_iterator it = pr.begin(); it != pr.end(); ++it)
	{
		if ((*it)->fragcount >= sv_fraglimit)
		{
			// [ML] 04/4/06: Added !sv_fragexitswitch
			SV_BroadcastPrintf(PRINT_HIGH, "Frag limit hit. Game won by %s!\n",
			                   (*it)->userinfo.netname.c_str());
			SV_SetWinPlayer((*it)->id);
			::levelstate.endGame();
		}
	}
}

void G_TeamFragsCheckEndGame()
{
	if (!::serverside || !::levelstate.canEndGame())
		return;

	if (sv_fraglimit <= 0.0)
		return;

	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		if (GetTeamInfo((team_t)i)->Points >= sv_fraglimit)
		{
			SV_BroadcastPrintf(PRINT_HIGH, "Frag limit hit. %s team wins!\n",
			                   GetTeamInfo((team_t)i)->ColorString.c_str());
			::levelstate.endGame();
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

	if (!g_survival || !::levelstate.canEndGame())
		return;

	if (sv_gametype == GM_COOP)
	{
		// Everybody losing their lives in coop is a failure.
		if (P_PlayerQuery(NULL, PQ_HASLIVES).result == 0)
		{
			SV_BroadcastPrintf(PRINT_HIGH,
			                   "Game over: All players have run out of lives.\n");
			::levelstate.endGame();
		}
	}
	else if (sv_gametype == GM_DM)
	{
		pr.clear();

		// One person being alive is success, nobody alive is a draw.
		PlayerCounts pc = P_PlayerQuery(&pr, PQ_HASLIVES);
		if (pc.result == 0 || pr.empty())
		{
			SV_BroadcastPrintf(PRINT_HIGH,
			                   "Game over: All players have run out of lives.\n");
			::levelstate.endGame();
		}
		else if (pc.result == 1)
		{
			SV_BroadcastPrintf(PRINT_HIGH,
			                   "Game over: %s was the last player standing.\n",
			                   pr.front()->userinfo.netname.c_str());
			::levelstate.endGame();
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
			SV_BroadcastPrintf(PRINT_HIGH,
			                   "Game over: All teams have run out of lives.\n");
			::levelstate.endGame();
		}
		else if (aliveteams == 1)
		{
			TeamInfo* teamInfo = GetTeamInfo(pr.front()->userinfo.team);
			SV_BroadcastPrintf(PRINT_HIGH,
			                   "Game over: %s team was the last team standing.\n",
			                   teamInfo->ColorStringUpper.c_str());
			::levelstate.endGame();
		}

		// Nobody won the game yet - keep going.
	}
}

VERSION_CONTROL(g_levelstate, "$Id$")
