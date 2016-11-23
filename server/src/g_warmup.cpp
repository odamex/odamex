// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//   Implements warmup mode.
//
//-----------------------------------------------------------------------------

#include "g_warmup.h"

#include <cmath>

#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "g_level.h"
#include "i_net.h"
#include "sv_main.h"

EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(sv_warmup_autostart)
EXTERN_CVAR(sv_countdown)
EXTERN_CVAR(sv_timelimit)

extern int mapchange;

// Store Warmup state.
Warmup warmup;

// Broadcast warmup state to client.
void SV_SendWarmupState(player_t &player, Warmup::status_t status, short count = 0)
{
	client_t* cl = &player.client;
	MSG_WriteMarker(&cl->reliablebuf, svc_warmupstate);
	MSG_WriteByte(&cl->reliablebuf, static_cast<byte>(status));
	if (status == Warmup::COUNTDOWN || status == Warmup::FORCE_COUNTDOWN)
		MSG_WriteShort(&cl->reliablebuf, count);
}

// Broadcast warmup state to all clients.
void SV_BroadcastWarmupState(Warmup::status_t status, short count = 0)
{
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (!it->ingame())
			continue;
		SV_SendWarmupState(*it, status, count);
	}
}

// Set warmup status and send clients warmup information.
void Warmup::set_status(Warmup::status_t new_status)
{
	this->status = new_status;

	// [AM] If we switch to countdown, set the correct time.
	if (this->status == Warmup::COUNTDOWN || this->status == Warmup::FORCE_COUNTDOWN)
	{
		this->time_begin = level.time + (sv_countdown.asInt() * TICRATE);
		SV_BroadcastWarmupState(new_status, (short)sv_countdown.asInt());
	}
	else
		SV_BroadcastWarmupState(new_status);
}

// Status getter
Warmup::status_t Warmup::get_status()
{
	return this->status;
}

// Get Overtime number
short Warmup::get_overtime()
{
	return this->overtime_count;
}

// Add another Overtime
void Warmup::add_overtime()
{
	this->overtime_count++;
}

// Warmup countdown getter
short Warmup::get_countdown()
{
	return ceil((this->time_begin - level.time) / (float)TICRATE);
}

// Reset warmup to "factory defaults".
void Warmup::reset()
{
	if (sv_warmup && sv_gametype != GM_COOP)
		this->set_status(Warmup::WARMUP);
	else
		this->set_status(Warmup::DISABLED);
	this->overtime_count = 0;
	this->time_begin = 0;
}

// Don't allow a players score to change if the server is in the middle of warmup.
bool Warmup::checkscorechange()
{
	if (this->status == Warmup::WARMUP || this->status == Warmup::COUNTDOWN ||
	    this->status == Warmup::FORCE_COUNTDOWN)
		return false;
	return true;
}

// Don't allow the timeleft to advance if the server is in the middle of warmup.
bool Warmup::checktimeleftadvance()
{
	if (this->status == Warmup::WARMUP || this->status == Warmup::COUNTDOWN ||
	    this->status == Warmup::FORCE_COUNTDOWN)
		return false;
	return true;
}

// Don't allow players to fire their weapon if the server is in the middle
// of a countdown.
bool Warmup::checkfireweapon()
{
	if (this->status == Warmup::COUNTDOWN || this->status == Warmup::FORCE_COUNTDOWN)
		return false;
	return true;
}

// Check to see if we should allow players to toggle their ready state.
bool Warmup::checkreadytoggle()
{
	if (this->status == Warmup::INGAME || this->status == Warmup::FORCE_COUNTDOWN)
		return false;
	return true;
}

extern size_t P_NumPlayersInGame();
extern size_t P_NumReadyPlayersInGame();

// Start or stop the countdown based on if players are ready or not.
void Warmup::readytoggle()
{
	// If warmup mode is disabled, don't bother.
	if (this->status == Warmup::DISABLED)
		return;

	// If we're not actually in a countdown we can control, don't bother.
	if (this->status == Warmup::FORCE_COUNTDOWN)
		return;

	// Rerun our checks to make sure we didn't skip them earlier.
	if (!this->checkreadytoggle())
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
	if (f_calc > i_calc - MPEPSILON && f_calc < i_calc + MPEPSILON) {
		needed = i_calc + 1;
	}
	needed = (int)ceil(f_calc);

	if (ready >= needed)
	{
		if (this->status == Warmup::WARMUP)
			this->set_status(Warmup::COUNTDOWN);
	}
	else
	{
		if (this->status == Warmup::COUNTDOWN)
		{
			this->set_status(Warmup::WARMUP);
			SV_BroadcastPrintf(PRINT_HIGH, "Countdown aborted: Player unreadied.\n");
		}
	}
	return;
}

// We want to restart the map, so initialize a countdown that we
// can't bail out of.
void Warmup::restart()
{
	this->set_status(Warmup::FORCE_COUNTDOWN);
}

// Force the start of the game.
void Warmup::forcestart()
{
	// Useless outside of warmup.
	if (this->status != Warmup::WARMUP)
		return;

	this->set_status(Warmup::COUNTDOWN);
}

// Handle tic-by-tic maintenance of the warmup.
void Warmup::tic()
{
	// If warmup isn't enabled, return
	if (this->status == Warmup::DISABLED)
		return;

	// If autostart is zeroed out, start immediately.
	if (this->status == Warmup::WARMUP && sv_warmup_autostart == 0.0f)
		this->set_status(Warmup::COUNTDOWN);

	// If there aren't any more active players, go back to warm up mode [tm512 2014/04/08]
	if (this->status != Warmup::WARMUP && P_NumPlayersInGame () == 0)
		this->set_status (Warmup::WARMUP);

	// If we're not advancing the countdown, we don't care.
	if (!(this->status == Warmup::COUNTDOWN || this->status == Warmup::FORCE_COUNTDOWN))
		return;

	// If we haven't reached the level tic that we begin the map on,
	// we don't care.
	if (this->time_begin > level.time)
	{
		// Broadcast a countdown (this should be handled clientside)
		if ((this->time_begin - level.time) % TICRATE == 0)
		{
			SV_BroadcastWarmupState(this->status, this->get_countdown());
		}
		return;
	}

	if (sv_warmup)
		this->set_status(Warmup::INGAME);
	else
		this->set_status(Warmup::DISABLED);

	// [SL] always reset the time (for now at least)
	level.time = 0;
	level.timeleft = sv_timelimit * TICRATE * 60;
	level.inttimeleft = mapchange / TICRATE;

	G_DeferedFullReset();
	SV_BroadcastPrintf(PRINT_HIGH, "The match has started.\n");
}

BEGIN_COMMAND (forcestart)
{
	warmup.forcestart();
}
END_COMMAND (forcestart)
