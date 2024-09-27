// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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


#include "odamex.h"

#include "g_levelstate.h"

#include <cmath>

#include "c_dispatch.h"
#include "g_gametype.h"
#include "i_system.h"

EXTERN_CVAR(g_lives)
EXTERN_CVAR(g_lives_jointimer)
EXTERN_CVAR(sv_countdown)
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(sv_warmup_autostart)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(g_rounds)
EXTERN_CVAR(g_roundlimit)
EXTERN_CVAR(g_preroundtime)
EXTERN_CVAR(g_preroundreset)
EXTERN_CVAR(g_postroundtime)

LevelState levelstate;

/**
 * @brief Countdown getter.
 */
int LevelState::getCountdown() const
{
	if (m_state == LevelState::WARMUP || m_state == LevelState::INGAME)
		return 0;

	int period = m_countdownDoneTime - ::level.time;
	if (period < 0)
	{
		// Time desync at the start of a round, force to maximum.
		return g_preroundtime.asInt();
	}

	return ceil(period / (float)TICRATE);
}

/**
 * @brief Return which team is on defense this round.
 */
team_t LevelState::getDefendingTeam() const
{
	if (!G_IsSidesGame())
	{
		return TEAM_NONE;
	}

	// Blue always goes first, then red, then so on...
	int teams = clamp(sv_teamsinplay.asInt(), 2, 3);
	int round0 = MAX(::levelstate.getRound() - 1, 0);
	return static_cast<team_t>(round0 % teams);
}

/**
 * @brief Get ingame start time.
 */
int LevelState::getIngameStartTime() const
{
	return m_ingameStartTime;
}

/**
 * @brief Amount of time left for a player to join the game.
 */
int LevelState::getJoinTimeLeft() const
{
	if (m_state != LevelState::INGAME)
		return 0;

	int end_time = m_ingameStartTime + g_lives_jointimer * TICRATE;
	int left = ceil((end_time - ::level.time) / (float)TICRATE);
	return MAX(left, 0);
}

/**
 * @brief Get the current round number.
 */
int LevelState::getRound() const
{
	return m_roundNumber;
}

/**
 * @brief State getter.
 */
LevelState::States LevelState::getState() const
{
	return m_state;
}

/**
 * @brief Get state as string.
 */
const char* LevelState::getStateString() const
{
	switch (m_state)
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
 * @brief Get the most recent win information.
 */
WinInfo LevelState::getWinInfo() const
{
	return m_lastWininfo;
}

/**
 * @brief Set callback to call when changing state.
 */
void LevelState::setStateCB(LevelState::SetStateCB cb)
{
	m_setStateCB = cb;
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
	m_lastWininfo.type = type;
	m_lastWininfo.id = id;
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
	else if (g_lives && P_NumPlayersInGame() == 0)
	{
		// Empty servers that use lives should always get sent to warmup.
		// If this wasn't here, LevelState::tic() would do the transition,
		// but with the potential for a spurious "round start" message.
		setState(LevelState::WARMUP);
	}
	else if (g_lives && !G_IsCoopGame())
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
		m_roundNumber = 1;
		setState(LevelState::getStartOfRoundState());

		// Don't print "match started" for every game mode.
		if (::g_rounds)
			printRoundStart();
	}

	m_countdownDoneTime = 0;
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
	if (m_state != LevelState::WARMUP)
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

	// Lobbies can't be readied up in.
	if (::level.flags & LEVEL_LOBBYSPECIAL)
		return;

	// No useful work can be done unless we're either in warmup or taking
	// part in the normal warmup countdown.
	if (!(m_state == LevelState::WARMUP || m_state == LevelState::WARMUP_COUNTDOWN))
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
		if (m_state == LevelState::WARMUP)
			setState(LevelState::WARMUP_COUNTDOWN);
	}
	else
	{
		if (m_state == LevelState::WARMUP_COUNTDOWN)
		{
			setState(LevelState::WARMUP);
			SV_BroadcastPrintf("Countdown aborted: Player unreadied.\n");
		}
	}
}

/**
 * @brief Depending on if we're using rounds or not, either kick us to an
 *        "end of round" state or just end the game right here.
 */
void LevelState::endRound()
{
	if (G_IsRoundsGame())
	{
		// Check for round-ending conditions.
		if (G_RoundsShouldEndGame())
			setState(LevelState::ENDGAME_COUNTDOWN);
		else
			setState(LevelState::ENDROUND_COUNTDOWN);
	}
	else if (G_IsCoopGame())
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
	if (m_state != LevelState::ENDGAME_COUNTDOWN)
		setState(LevelState::ENDGAME_COUNTDOWN);
}

/**
 * @brief Handle tic-by-tic maintenance of the level state.
 */
void LevelState::tic()
{
	// Can't have levelstate without a level.
	if (::gamestate != GS_LEVEL)
		return;

	// Clients are not authoritative on levelstate.
	if (!::serverside)
		return;

	// Handle singleplayer pauses.
	if (::paused || ::menuactive)
		return;

	// If there aren't any more active players, go back to warm up mode [tm512 2014/04/08]
	if (m_state != LevelState::WARMUP && P_NumPlayersInGame() == 0)
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
	switch (m_state)
	{
	case LevelState::UNKNOWN:
		I_FatalError("Tried to tic unknown LevelState.\n");
		break;
	case LevelState::PREROUND_COUNTDOWN:
		// Once the timer has run out, start the round.
		if (::level.time >= m_countdownDoneTime)
		{
			// Possibly reset the map again before the fight begins.
			if (g_preroundreset)
				G_DeferedReset();

			setState(LevelState::INGAME);
			SV_BroadcastPrintf("FIGHT!\n");
			return;
		}
		break;
	case LevelState::INGAME:
		// Nothing special happens.
		// [AM] Maybe tic the gametype-specific logic here?
		break;
	case LevelState::ENDROUND_COUNTDOWN:
		// Once the timer has run out, reset and go to the next round.
		if (::level.time >= m_countdownDoneTime)
		{
			// Must be run first, so setState(INGAME) knows the correct start time.
			G_DeferedReset();

			m_roundNumber += 1;
			setState(LevelState::getStartOfRoundState());
			printRoundStart();
			return;
		}
		break;
	case LevelState::ENDGAME_COUNTDOWN:
		// Once the timer has run out, go to intermission.
		if (::level.time >= m_countdownDoneTime)
		{
			G_ExitLevel(0, 1);
			return;
		}
		break;
	case LevelState::WARMUP: {
		if (::level.flags & LEVEL_LOBBYSPECIAL)
		{
			// Lobby maps stay in warmup mode forever.
			return;
		}

		if (!sv_warmup)
		{
			// We are in here for gametype reasons.  Auto-start once we
			// have enough players.
			PlayerResults pr = PlayerQuery().execute();
			if (G_IsCoopGame() && pr.count >= 1)
			{
				// Coop needs one player.
				setState(LevelState::WARMUP_FORCED_COUNTDOWN);
				return;
			}
			else if (sv_gametype == GM_DM && pr.count >= 2)
			{
				// DM needs two players.
				setState(LevelState::WARMUP_FORCED_COUNTDOWN);
				return;
			}
			else if (G_IsTeamGame())
			{
				// We need at least one person on at least two different teams.
				int ready = 0;
				for (size_t i = 0; i < NUMTEAMS; i++)
				{
					if (pr.teamCount[i] > 0)
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
		if (::level.time >= m_countdownDoneTime)
		{
			// Must be run first, so setState(INGAME) knows the correct start time.
			G_DeferedFullReset();

			m_roundNumber += 1;
			setState(LevelState::getStartOfRoundState());

			if (g_rounds)
				printRoundStart();
			else
			{
				SV_BroadcastPrintf("The %s has started.\n",
				                   G_IsCoopGame() ? "game" : "match");
			}
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
	serialized.state = m_state;
	serialized.countdown_done_time = m_countdownDoneTime;
	serialized.ingame_start_time = m_ingameStartTime;
	serialized.round_number = m_roundNumber;
	serialized.last_wininfo_type = m_lastWininfo.type;
	serialized.last_wininfo_id = m_lastWininfo.id;
	return serialized;
}

/**
 * @brief Unserialize variables into levelstate.  Usually comes from a server.
 *
 * @param serialized New level state to set.
 */
void LevelState::unserialize(SerializedLevelState serialized)
{
	m_state = serialized.state;
	m_countdownDoneTime = serialized.countdown_done_time;
	m_ingameStartTime = serialized.ingame_start_time;
	m_roundNumber = serialized.round_number;
	m_lastWininfo.type = serialized.last_wininfo_type;
	m_lastWininfo.id = serialized.last_wininfo_id;
}

/**
 * @brief Set the appropriate start-of-round state.
 */
LevelState::States LevelState::getStartOfRoundState()
{
	// Preround only makes sense for deathmatch modes.
	const bool validMode = sv_gametype == GM_DM || sv_gametype == GM_TEAMDM;
	if (G_IsRoundsGame() && g_preroundtime > 0.0 && validMode)
	{
		return LevelState::PREROUND_COUNTDOWN;
	}

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
	m_state = new_state;
	// Printf("setState: %s\n", getStateString());

	if (m_state == LevelState::WARMUP_COUNTDOWN ||
	    m_state == LevelState::WARMUP_FORCED_COUNTDOWN)
	{
		// Most countdowns use the countdown cvar.
		m_countdownDoneTime = ::level.time + (sv_countdown.asInt() * TICRATE);
	}
	else if (m_state == LevelState::PREROUND_COUNTDOWN)
	{
		// This actually has tactical significance, so it needs to have its
		// own hardcoded time or a cvar dedicated to it.  Also, we don't add
		// level.time to it becuase level.time always starts at zero preround.
		m_countdownDoneTime = g_preroundtime.asInt() * TICRATE;
	}
	else if (m_state == LevelState::ENDROUND_COUNTDOWN ||
	         m_state == LevelState::ENDGAME_COUNTDOWN)
	{
		// Endgame has a little pause as well, like the old "shotclock" variable.
		m_countdownDoneTime = ::level.time + (g_postroundtime.asInt() * TICRATE);
	}
	else
	{
		m_countdownDoneTime = 0;
	}

	// Set the start time as soon as we go ingame.
	if (m_state == LevelState::INGAME)
	{
		if (::gameaction == ga_resetlevel || ::gameaction == ga_fullresetlevel)
		{
			m_ingameStartTime = 1;
		}
		else
		{
			m_ingameStartTime = ::level.time + 1;
		}
	}

	// If we're in a warmup state, alwasy reset the round count to zero.
	if (m_state == LevelState::WARMUP || m_state == LevelState::WARMUP_COUNTDOWN ||
	    m_state == LevelState::WARMUP_FORCED_COUNTDOWN)
	{
		m_roundNumber = 0;
	}

	// If we're setting the state to something that's not an ending countdown,
	// reset our winners to nothing.
	if (m_state != LevelState::ENDROUND_COUNTDOWN &&
	    m_state != LevelState::ENDGAME_COUNTDOWN)
	{
		m_lastWininfo.reset();
	}

	if (m_setStateCB)
	{
		m_setStateCB(serialize());
	}
}

/**
 * @brief Print a message at the start of each round.
 */
void LevelState::printRoundStart() const
{
	std::string left, right;
	if (g_roundlimit > 0)
	{
		StrFormat(left, "Round %d of %d has started", m_roundNumber,
		          g_roundlimit.asInt());
	}
	else
	{
		StrFormat(left, "Round %d has started", m_roundNumber);
	}

	team_t def = getDefendingTeam();
	if (G_IsCoopGame() && g_roundlimit)
	{
		StrFormat(right, "%d attempts left", g_roundlimit.asInt() - m_roundNumber + 1);
	}
	else if (def != TEAM_NONE)
	{
		TeamInfo& teaminfo = *GetTeamInfo(def);
		StrFormat(right, "%s is on defense", teaminfo.ColorizedTeamName().c_str());
	}

	if (!right.empty())
	{
		SV_BroadcastPrintf("%s - %s.\n", left.c_str(), right.c_str());
	}
	else
	{
		SV_BroadcastPrintf("%s.\n", left.c_str());
	}
}

BEGIN_COMMAND(forcestart)
{
	::levelstate.forceStart();
}
END_COMMAND(forcestart)

static const char* StateToString(LevelState::States state)
{
	switch (state)
	{
	case LevelState::WARMUP:
		return "WARMUP";
	case LevelState::WARMUP_COUNTDOWN:
		return "WARMUP_COUNTDOWN";
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		return "WARMUP_FORCED_COUNTDOWN";
	case LevelState::PREROUND_COUNTDOWN:
		return "PREROUND_COUNTDOWN";
	case LevelState::INGAME:
		return "INGAME";
	case LevelState::ENDROUND_COUNTDOWN:
		return "ENDROUND_COUNTDOWN";
	case LevelState::ENDGAME_COUNTDOWN:
		return "ENDGAME_COUNTDOWN";
	default:
		return "UNKNOWN";
	}
}

static const char* WinTypeToString(WinInfo::WinType type)
{
	switch (type)
	{
	case WinInfo::WIN_NOBODY:
		return "WIN_NOBODY";
	case WinInfo::WIN_EVERYBODY:
		return "WIN_EVERYBODY";
	case WinInfo::WIN_DRAW:
		return "WIN_DRAW";
	case WinInfo::WIN_PLAYER:
		return "WIN_PLAYER";
	case WinInfo::WIN_TEAM:
		return "WIN_TEAM";
	default:
		return "WIN_UNKNOWN";
	}
};

BEGIN_COMMAND(levelstateinfo)
{
	SerializedLevelState sls = ::levelstate.serialize();
	Printf("Current level time: %d\n", ::level.time);
	Printf("State: %s\n", StateToString(sls.state));
	Printf("Countdown done time: %d\n", sls.countdown_done_time);
	Printf("Ingame start time: %d\n", sls.ingame_start_time);
	Printf("Round number: %d\n", sls.round_number);
	Printf("Last WinInfo type: %s\n", WinTypeToString(sls.last_wininfo_type));
	Printf("Last WinInfo ID: %d\n", sls.last_wininfo_id);
}
END_COMMAND(levelstateinfo)

VERSION_CONTROL(g_levelstate, "$Id$")
