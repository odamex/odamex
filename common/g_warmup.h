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

#ifndef __G_WARMUP_H__
#define __G_WARMUP_H__

#include <cstddef>

#include "d_player.h"
#include "doomstat.h"

class Warmup
{
public:
	typedef enum
	{
	    DISABLED,
	    WARMUP,
	    INGAME,
	    COUNTDOWN,      // Can be cancelled by unreadying
	    FORCE_COUNTDOWN // Can't be cancelled by unreadying
	} status_t;
	Warmup() : status(Warmup::DISABLED), time_begin(0) { }
	Warmup::status_t get_status();
	short get_countdown();
	void reset();
	bool checkscorechange();
	bool checktimeleftadvance();
	bool checkfireweapon();
	bool checkreadytoggle();
	void restart();
	void forcestart();
	void readytoggle();
	void tic();
	short get_overtime();
	void add_overtime();
	void set_client_status(status_t new_status); // Clientside only.
private:
	status_t status;
	short overtime_count;
	int time_begin;
	void set_status(status_t new_status);
};

void SV_SendWarmupState(player_t &player, Warmup::status_t status, short count); // Serverside only.

extern Warmup warmup;

#endif
