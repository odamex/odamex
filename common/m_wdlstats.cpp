// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//     Defines needed by the WDL stats logger.
//
//-----------------------------------------------------------------------------

#include "m_wdlstats.h"

#include <string>
#include <vector>

#include "c_dispatch.h"
#include "g_warmup.h"

#define WDLSTATS_VERSION 5

extern Players players;

EXTERN_CVAR(sv_gametype)

// Strings for WDL events.
static const char* wdlevstrings[] = {
	"DAMAGE",
	"CARRIERDAMAGE",
	"KILL",
	"CARRIERKILL",
	"ENVIRODAMAGE",
	"ENVIROCARRIERDAMAGE",
	"ENVIROKILL",
	"ENVIROCARRIERKILL",
	"TOUCH",
	"PICKUPTOUCH",
	"CAPTURE",
	"PICKUPCAPTURE",
	"ASSIST",
	"RETURNFLAG",
	"POWERPICKUP",
	"ACCURACY",
};

// WDL Stats dir - if not empty, we are logging.
static std::string wdlstatdir;

// A single tracked player
struct WDLPlayer
{
	std::string netname;
	team_t team;
};

// WDL Players that we're keeping track of.
typedef std::vector<WDLPlayer> WDLPlayers;
static WDLPlayers wdlplayers;

// A single event.
struct WDLEvent
{
	WDLEvents ev;
	std::string activator;
	std::string target;
	int gametic;
	fixed_t apos[3];
	fixed_t tpos[3];
	int arg0;
	int arg1;
	int arg2;
};

// Events that we're keeping track of.
typedef std::vector<WDLEvent> WDLEventLog;
static WDLEventLog wdlevents;

// The starting gametic of the most recent log.
static int wdlbegintic;

// Turn an event enum into a string.
static const char* WDLEventString(WDLEvents i)
{
	if (i >= ARRAY_LENGTH(::wdlevstrings) || i < 0)
		return "UNKNOWN";
	return ::wdlevstrings[i];
}

static void AddWDLPlayer(const player_t* player)
{
	// Don't add player if their name is already in the vector.
	WDLPlayers::const_iterator it = ::wdlplayers.begin();
	for (; it != ::wdlplayers.end(); ++it)
	{
		if ((*it).netname == player->userinfo.netname)
			return;
	}

	WDLPlayer wdlplayer = {
		player->userinfo.netname,
		player->userinfo.team,
	};
	::wdlplayers.push_back(wdlplayer);
}

// Returns true if a player is ingame.
// FIXME: Put this someplace global.
static bool PlayerInGame(const player_t* player)
{
	return (
		player->ingame() &&
		player->spectator == false
	);
}

// Returns true if a player is ingame and on a specific team
// FIXME: Put this someplace global.
static bool PlayerInTeam(const player_t* player, byte team)
{
	return (
		player->ingame() &&
		player->userinfo.team == team &&
		player->spectator == false
	);
}

// Returns the number of players on a team
// FIXME: Put this someplace global.
static int CountTeamPlayers(byte team)
{
	int count = 0;

	Players::const_iterator it = players.begin();
	for (; it != players.end(); ++it)
	{
		const player_t* player = &*it;
		if (PlayerInTeam(player, team))
			count += 1;
	}

	return count;
}

// Generate a log filename based on the current time.
static std::string GenerateTimestamp()
{
	time_t ti = time(NULL);
	struct tm *lt = localtime(&ti);

	char buf[128];
	if (!strftime(&buf[0], ARRAY_LENGTH(buf), "%Y.%m.%d.%H.%M.%S", lt))
		return "";

	return std::string(buf, strlen(&buf[0]));
}

static void WDLStatsHelp()
{
	Printf(PRINT_HIGH,
		"wdlstats - Starts logging WDL statistics to the given directory.  Unless "
		"you are running a WDL server, you probably are not interested in this.\n\n"
		"Usage:\n"
		"  ] wdlstats <DIRNAME>\n"
		"  Starts logging WDL statistics in the directory DIRNAME.\n");
}

BEGIN_COMMAND(wdlstats)
{
	if (argc < 2)
	{
		WDLStatsHelp();
		return;
	}

	// Setting the stats dir tells us that we intend to log.
	::wdlstatdir = argv[1];

	// Ensure our path ends with a slash.
	if (*(::wdlstatdir.end() - 1) != PATHSEPCHAR)
		::wdlstatdir += PATHSEPCHAR;

	Printf(
		PRINT_HIGH, "wdlstats: Enabled, will log to directory \"%s\".\n", wdlstatdir.c_str()
	);
}
END_COMMAND(wdlstats)

void M_StartWDLLog()
{
	if (::wdlstatdir.empty())
		return;

	// Ensure we're CTF.
	if (sv_gametype != 3)
	{
		Printf(
			PRINT_HIGH,
			"wdlstats: Not logging, incorrect gametype.\n"
		);
		return;
	}

	// Ensure that we're not in an invalid warmup state.
	Warmup::status_t wstatus = ::warmup.get_status();
	if (wstatus != Warmup::DISABLED && wstatus != Warmup::INGAME)
	{
		// [AM] This message can probably be deleted once we're sure the
		//      condition is appropriate.
		Printf(
			PRINT_HIGH,
			"wdlstats: Not logging, not ingame (yet).\n"
		);
		return;
	}

	// Ensure we're 3v3 or more.
	int blueplayers = CountTeamPlayers(TEAM_BLUE);
	int redplayers = CountTeamPlayers(TEAM_RED);
	if (blueplayers < 3 && redplayers < 3 && false)
	{
		Printf(
			PRINT_HIGH,
			"wdlstats: Not logging, too few players on a team (%d vs %d).\n",
			blueplayers, redplayers
		);
		return;
	}

	/// Clear our ingame players.
	::wdlplayers.clear();

	// Start with a fresh slate of events.
	::wdlevents.clear();

	// Set our starting tic.
	::wdlbegintic = ::gametic;

	Printf(PRINT_HIGH, "wdlstats: Started, will log to directory \"%s\".\n", wdlstatdir.c_str());
}

/**
 * Log a damage event.
 * 
 * Because damage can come in multiple pieces, this checks for an existing
 * event this tic and adds to it if it finds one.
 * 
 * Returns true if the function successfully appended to an existing event,
 * otherwise false if we need to generate a new event.
 */
static bool LogDamageEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2
)
{
	WDLEventLog::reverse_iterator it = ::wdlevents.rbegin();
	for (;it != ::wdlevents.rend(); ++it)
	{
		if ((*it).gametic != ::gametic)
		{
			// We're too late for events from last tic, so we must have a
			// new event.
			return false;
		}

		// Event type is the same?
		if ((*it).ev != event)
			continue;

		// Activator is the same?
		if ((*it).activator != activator->userinfo.netname)
			continue;

		// Target is the same?
		if ((*it).target != target->userinfo.netname)
			continue;

		// Update our existing event.
		(*it).arg0 += arg0;
		(*it).arg1 += arg1;
		Printf(PRINT_HIGH, "wdlstats: Updated targeted event %s.\n", WDLEventString(event));
		return true;
	}

	// We ran through all our events, must be a new event.
	return false;
}

/**
 * Log a WDL event.
 * 
 * The particulars of what you pass to this needs to be checked against the document.
 */
void M_LogWDLEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2
)
{
	if (::wdlstatdir.empty())
		return;

	// Activator
	std::string aname = "";
	int ax = 0;
	int ay = 0;
	int az = 0;
	if (activator != NULL)
	{
		// Add the activator.
		AddWDLPlayer(activator);
		aname = activator->userinfo.netname;

		// Add the activator's body information.
		if (activator->mo)
		{
			ax = activator->mo->x >> FRACBITS;
			ay = activator->mo->y >> FRACBITS;
			az = activator->mo->z >> FRACBITS;
		}
	}

	// Target
	std::string tname = "";
	int tx = 0;
	int ty = 0;
	int tz = 0;
	if (target != NULL)
	{
		// Add the target.
		AddWDLPlayer(target);
		tname = target->userinfo.netname;

		// Add the target's body information.
		if (target->mo)
		{
			tx = target->mo->x >> FRACBITS;
			ty = target->mo->y >> FRACBITS;
			tz = target->mo->z >> FRACBITS;
		}
	}

	// Damage events are handled specially.
	if (
		activator && target &&
		(event == WDL_EVENT_DAMAGE || event == WDL_EVENT_CARRIERDAMAGE)
	) {
		if (LogDamageEvent(event, activator, target, arg0, arg1, arg2))
			return;
	}

	// Add the event to the log.
	WDLEvent evt = {
		event, aname, tname, ::gametic,
		{ ax, ay, az }, { tx, ty, tz },
		arg0, arg1, arg2
	};
	::wdlevents.push_back(evt);
	Printf(PRINT_HIGH, "wdlstats: Logged targeted event %s.\n", WDLEventString(event));
}

/**
 * Log a WDL event when you have actor pointers.
 */
void M_LogActorWDLEvent(
	WDLEvents event, AActor* activator, AActor* target,
	int arg0, int arg1, int arg2
)
{
	player_t* ap = NULL;
	if (activator != NULL && activator->type == MT_PLAYER)
		ap = activator->player;

	player_t* tp = NULL;
	if (target != NULL && target->type == MT_PLAYER)
		tp = target->player;

	M_LogWDLEvent(event, ap, tp, arg0, arg1, arg2);
}

void M_CommitWDLLog()
{
	if (::wdlstatdir.empty())
		return;

	std::string timestamp = GenerateTimestamp();
	std::string filename = ::wdlstatdir + "wdl_" + timestamp + ".log";
	FILE* fh = fopen(filename.c_str(), "w+");
	if (fh == NULL)
	{
		Printf(PRINT_HIGH, "wdlstats: Could not save\"%s\" for writing.\n", filename.c_str());
		return;
	}

	// Header
	fprintf(fh, "version=%d\n", 5);
	fprintf(fh, "time=%s\n", timestamp.c_str());
	fprintf(fh, "levelnum=%d\n", ::level.levelnum);
	fprintf(fh, "levelname=%s\n", ::level.level_name);
	fprintf(fh, "duration=%d\n", ::gametic - ::wdlbegintic);
	fprintf(fh, "endgametic=%d\n", ::gametic);

	// Players
	fprintf(fh, "players\n");
	WDLPlayers::const_iterator pit = ::wdlplayers.begin();
	for (; pit != ::wdlplayers.end(); ++pit)
		fprintf(fh, "%d,%s\n", pit->team, pit->netname.c_str());

	// Events
	fprintf(fh, "events\n");
	WDLEventLog::const_iterator eit = ::wdlevents.begin();
	for (; eit != ::wdlevents.end(); ++eit)
	{
		//          "ev,ac,tg,gt,ax,ay,az,tx,ty,tz,a0,a1,a2"
		fprintf(fh, "%d,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			eit->ev, eit->activator.c_str(), eit->target.c_str(), ::gametic,
			eit->apos[0], eit->apos[1], eit->apos[2],
			eit->tpos[0], eit->tpos[1], eit->tpos[2],
			eit->arg0, eit->arg1, eit->arg2);
	}

	fclose(fh);
	Printf(PRINT_HIGH, "wdlstats: Log saved as \"%s\".\n", filename.c_str());
}

static void PrintWDLEvent(const WDLEvent& evt)
{
	// FIXME: Once we have access to StrFormat, dedupe this format string.
	//                 "ev,ac,tg,gt,ax,ay,az,tx,ty,tz,a0,a1,a2"
	Printf(PRINT_HIGH, "%d,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		evt.ev, evt.activator.c_str(), evt.target.c_str(), ::gametic,
		evt.apos[0], evt.apos[1], evt.apos[2],
		evt.tpos[0], evt.tpos[1], evt.tpos[2],
		evt.arg0, evt.arg1, evt.arg2);
}

static void WDLInfoHelp()
{
	Printf(PRINT_HIGH,
		"wdlinfo - Looks up internal information about logged WDL events\n\n"
		"Usage:\n"
		"  ] wdlinfo event <ID>\n"
		"  Print the event by ID.\n\n"
		"  ] wdlinfo size\n"
		"  Return the size of the internal event array.\n\n"
		"  ] wdlinfo tail\n"
		"  Print the last 10 events.\n");
}

BEGIN_COMMAND(wdlinfo)
{
	if (argc < 2)
	{
		WDLInfoHelp();
		return;
	}

	if (stricmp(argv[1], "size") == 0)
	{
		// Count total events.
		Printf(PRINT_HIGH, "%u events found\n", ::wdlevents.size());
		return;
	}
	else if (stricmp(argv[1], "tail") == 0)
	{
		// Show last 10 events.
		WDLEventLog::const_iterator it = ::wdlevents.end() - 10;
		if (it < ::wdlevents.begin())
			it = wdlevents.begin();

		Printf(PRINT_HIGH, "Showing last %u events:\n", ::wdlevents.end() - it);
		for (; it != ::wdlevents.end(); ++it)
			PrintWDLEvent(*it);
		return;
	}

	if (argc < 3)
	{
		WDLInfoHelp();
		return;
	}

	if (stricmp(argv[1], "event") == 0)
	{
		int id = atoi(argv[2]);
		if (id >= ::wdlevents.size())
		{
			Printf(PRINT_HIGH, "Event number %d not found\n", id);
			return;
		}
		WDLEvent evt = ::wdlevents.at(id);
		PrintWDLEvent(evt);
		return;
	}

	// Unknown command.
	WDLInfoHelp();
}
END_COMMAND(wdlinfo)
