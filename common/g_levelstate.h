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

#ifndef __G_LEVELSTATE_H__
#define __G_LEVELSTATE_H__

#include <cstddef>

#include "d_player.h"
#include "doomstat.h"

struct SerializedLevelState;

class LevelState
{
  public:
	enum States
	{
		UNKNOWN,                // Unknown state.
		INGAME,                 // In the middle of a game.
		COUNTDOWN,              // Ingame round countdown.
		END,                    // Game complete, a slight pause before intermission.
		WARMUP,                 // Warmup state.
		WARMUP_COUNTDOWN,       // Warmup countdown.
		WARMUP_FORCE_COUNTDOWN, // Forced countdown, can't be cancelled by unreadying.
	};
	LevelState() : _state(LevelState::UNKNOWN), _time_begin(0)
	{
	}
	LevelState::States getState() const;
	short getCountdown() const;
	void reset(level_locals_t& level);
	bool checkScoreChange() const;
	bool checkTimeLeftAdvance() const;
	bool checkFireWeapon() const;
	bool checkReadyToggle() const;
	void restart();
	void forceStart();
	void readyToggle();
	void tic();
	SerializedLevelState serialize() const;
	void unserialize(SerializedLevelState serialized);

  private:
	LevelState::States _state;
	int _time_begin;
	void setState(LevelState::States new_state);
};

struct SerializedLevelState
{
	LevelState::States state;
	int time_begin;
};

extern LevelState levelstate;

#endif
