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

#include "c_dispatch.h"
#include "g_gametype.h"
#include "i_system.h"

EXTERN_CVAR(g_lives)
EXTERN_CVAR(g_lives_jointimer)
EXTERN_CVAR(sv_countdown)
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_warmup_autostart)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(g_rounds)

LevelState levelstate;

const int PREROUND_SECONDS = 5;
const int ENDGAME_SECONDS = 3;

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
		return "Endround Countdown";
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

	return ceil((_countdown_done_time - ::level.time) / (float)TICRATE);
}

/**
 * @brief Get the current round number.
 */
int LevelState::getRound() const
{
	return _round_number;
}

/**
 * @brief Amount of time left for a player to join the game.
 */
int LevelState::getJoinTimeLeft() const
{
	if (_state != LevelState::INGAME)
		return 0;

	int end_time = _ingame_start_time + g_lives_jointimer * TICRATE;
	int left = ceil((end_time - ::level.time) / (float)TICRATE);
	return MAX(left, 0);
}

/**
 * @brief Get the most recent win information.
 */
WinInfo LevelState::getWinInfo() const
{
	return _last_wininfo;
}

/**
 * @brief Set callback to call when changing state.
 */
void LevelState::setStateCB(LevelState::SetStateCB cb)
{
	_set_state_cb = cb;
}

/**
 * @brief Set who the winner of the round or match should be.  Note that this
 *        data isn't persisted until after the state changes.
 *
 * @param type Winner of the round.
 * @param id ID of the winning player or team.  If N/A, put 0.
 */
void LevelState::setWinner(WinInfo::WinType type, int id)
{
	// Set our round/match winner.
	_last_wininfo.type = type;
	_last_wininfo.id = id;
}

/**
 * @brief Reset levelstate to "factory defaults" for the level.
 */
void LevelState::reset()
{
	if (::level.flags & LEVEL_LOBBYSPECIAL)
	{
		// Lobbies are all warmup, all the time.
		setState(LevelState::WARMUP);
	}
	else if (g_lives && sv_gametype != GM_COOP)
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
		// Defer to the default.
		_round_number = 1;
		setState(LevelState::getStartOfRoundState());

		// Don't print "match started" for every game mode.
		if (g_rounds)
			SV_BroadcastPrintf(PRINT_HIGH, "Round %d has started.\n", _round_number);
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
		// Check for round-ending conditions.
		if (G_RoundsShouldEndGame())
			setState(LevelState::ENDGAME_COUNTDOWN);
		else
			setState(LevelState::ENDROUND_COUNTDOWN);
	}
	else if (sv_gametype == GM_COOP)
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
 * @brief End the game immediately.
 */
void LevelState::endGame()
{
	setState(LevelState::ENDGAME_COUNTDOWN);
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
		if (sv_warmup || g_lives)
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
		if (::level.time >= _countdown_done_time)
		{
			setState(LevelState::INGAME);
			SV_BroadcastPrintf(PRINT_HIGH, "FIGHT!\n");
			return;
		}
		break;
	case LevelState::INGAME:
		// Nothing special happens.
		// [AM] Maybe tic the gametype-specific logic here?
		break;
	case LevelState::ENDROUND_COUNTDOWN:
		// Once the timer has run out, reset and go to the next round.
		if (::level.time >= _countdown_done_time)
		{
			// Must be run first, so setState(INGAME) knows the correct start time.
			G_DeferedReset();

			_round_number += 1;
			setState(LevelState::getStartOfRoundState());

			SV_BroadcastPrintf(PRINT_HIGH, "Round %d has started.\n", _round_number);
			return;
		}
		break;
	case LevelState::ENDGAME_COUNTDOWN:
		// Once the timer has run out, go to intermission.
		if (::level.time >= _countdown_done_time)
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
		if (::level.time >= _countdown_done_time)
		{
			// Must be run first, so setState(INGAME) knows the correct start time.
			G_DeferedFullReset();

			_round_number += 1;
			setState(LevelState::getStartOfRoundState());

			if (g_rounds)
				SV_BroadcastPrintf(PRINT_HIGH, "Round %d has started.\n", _round_number);
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
	serialized.round_number = _round_number;
	serialized.last_wininfo_type = _last_wininfo.type;
	serialized.last_wininfo_id = _last_wininfo.id;
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
	_round_number = serialized.round_number;
	_last_wininfo.type = serialized.last_wininfo_type;
	_last_wininfo.id = serialized.last_wininfo_id;
}

/**
 * @brief Set the appropriate start-of-round state.
 */
LevelState::States LevelState::getStartOfRoundState()
{
	// Deathmatch game modes in survival start with a start-of-round timer.
	// This is inappropriate for coop or objective-based modes.
	if (g_lives && (sv_gametype == GM_DM || sv_gametype == GM_TEAMDM))
		return LevelState::PREROUND_COUNTDOWN;

	return LevelState::INGAME;
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
	//Printf("setState: %s\n", getStateString());

	if (_state == LevelState::WARMUP_COUNTDOWN ||
	    _state == LevelState::WARMUP_FORCED_COUNTDOWN)
	{
		// Most countdowns use the countdown cvar.
		_countdown_done_time = ::level.time + (sv_countdown.asInt() * TICRATE);
	}
	else if (_state == LevelState::PREROUND_COUNTDOWN)
	{
		// This actually has tactical significance, so it needs to have its
		// own hardcoded time or a cvar dedicated to it.  Also, we don't add
		// level.time to it becuase level.time always starts at zero preround.
		_countdown_done_time = PREROUND_SECONDS * TICRATE;
	}
	else if (_state == LevelState::ENDROUND_COUNTDOWN ||
	         _state == LevelState::ENDGAME_COUNTDOWN)
	{
		// Endgame has a little pause as well, like the old "shotclock" variable.
		_countdown_done_time = ::level.time + ENDGAME_SECONDS * TICRATE;
	}
	else
	{
		_countdown_done_time = 0;
	}

	// Set the start time as soon as we go ingame.
	if (_state == LevelState::INGAME)
	{
		if (::gameaction == ga_resetlevel || ::gameaction == ga_fullresetlevel)
		{
			_ingame_start_time = 1;
		}
		else
		{
			_ingame_start_time = ::level.time + 1;
		}
	}

	// If we're in a warmup state, alwasy reset the round count to zero.
	if (_state == LevelState::WARMUP || _state == LevelState::WARMUP_COUNTDOWN ||
	    _state == LevelState::WARMUP_FORCED_COUNTDOWN)
	{
		_round_number = 0;
	}

	// If we're setting the state to something that's not an ending countdown,
	// reset our winners to nothing.
	if (_state != LevelState::ENDROUND_COUNTDOWN &&
	    _state != LevelState::ENDGAME_COUNTDOWN)
	{
		_last_wininfo.reset();
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

VERSION_CONTROL(g_levelstate, "$Id$")
