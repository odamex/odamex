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
	typedef void (*SetStateCB)(SerializedLevelState);
	enum States
	{
		UNKNOWN,                 // Unknown state.
		INGAME,                  // In the middle of a game.
		PREGAME,                 // Ingame round countdown.
		ENDGAME,                 // Game complete, a slight pause before intermission.
		WARMUP,                  // Warmup state.
		WARMUP_COUNTDOWN,        // Warmup countdown.
		WARMUP_FORCED_COUNTDOWN, // Forced countdown, can't be cancelled by unreadying.
	};
	LevelState()
	    : _state(LevelState::UNKNOWN), _countdown_done_time(0), _ingame_start_time(0),
	      _set_state_cb(NULL)
	{
	}
	LevelState::States getState() const;
	const char* getStateString() const;
	short getCountdown() const;
	int getJoinTimeLeft() const;
	bool canEndGame() const;
	bool canFireWeapon() const;
	bool canJoinGame() const;
	bool canLivesChange() const;
	bool canReadyToggle() const;
	bool canScoreChange() const;
	bool canShowObituary() const;
	bool canTickGameplay() const;
	bool canTimeLeftAdvance() const;
	void setStateCB(LevelState::SetStateCB cb);
	void reset(level_locals_t& level);
	void restart();
	void forceStart();
	void readyToggle();
	void endGame();
	void tic();
	SerializedLevelState serialize() const;
	void unserialize(SerializedLevelState serialized);

  private:
	LevelState::States _state;
	int _countdown_done_time;
	int _ingame_start_time;
	LevelState::SetStateCB _set_state_cb;
	void setState(LevelState::States new_state);
};

struct SerializedLevelState
{
	LevelState::States state;
	int countdown_done_time;
	int ingame_start_time;
};

extern LevelState levelstate;

void G_FragsCheckEndGame();
void G_TeamFragsCheckEndGame();
void G_LivesCheckEndGame();

#endif
