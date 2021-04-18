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
#include "g_levelstate.h"
#include "p_local.h"

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

static struct WDLState {
	// Directory to log stats to.
	std::string logdir;

	// True if we're recording stats for this map or restart, otherwise false.
	bool recording;

	// The starting gametic of the most recent log.
	int begintic;
} wdlstate;

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
	::wdlstate.logdir = argv[1];

	// Ensure our path ends with a slash.
	if (*(::wdlstate.logdir.end() - 1) != PATHSEPCHAR)
		::wdlstate.logdir += PATHSEPCHAR;

	Printf(
		PRINT_HIGH, "wdlstats: Enabled, will log to directory \"%s\" on next map change.\n",
		wdlstate.logdir.c_str()
	);
}
END_COMMAND(wdlstats)

void M_StartWDLLog()
{
	if (::wdlstate.logdir.empty())
	{
		::wdlstate.recording = false;
		return;
	}

	// Ensure we're CTF.
	if (sv_gametype != 3)
	{
		::wdlstate.recording = false;
		Printf(
			PRINT_HIGH,
			"wdlstats: Not logging, incorrect gametype.\n"
		);
		return;
	}

	// Ensure that we're not in an invalid warmup state.
	if (::levelstate.getState() != LevelState::INGAME)
	{
		// [AM] This is a little too much inside baseball to print about.
		::wdlstate.recording = false;
		return;
	}

	// Clear our ingame players.
	::wdlplayers.clear();

	// Start with a fresh slate of events.
	::wdlevents.clear();

	// Turn on recording.
	::wdlstate.recording = true;

	// Set our starting tic.
	::wdlstate.begintic = ::gametic;

	Printf(
		PRINT_HIGH, "wdlstats: Started, will log to directory \"%s\".\n",
		wdlstate.logdir.c_str()
	);
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
		return true;
	}

	// We ran through all our events, must be a new event.
	return false;
}

/**
 * Log accuracy event.
 * 
 * Returns true if the function successfully appended to an existing event,
 * otherwise false if we need to generate a new event.
 */
static bool LogAccuracyEvent(
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

		// Update our existing event - by doing nothing.
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
	if (!::wdlstate.recording)
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

	if (event == WDL_EVENT_ACCURACY)
	{
		if (LogAccuracyEvent(event, activator, target, arg0, arg1, arg2))
			return;
	}

	// Add the event to the log.
	WDLEvent evt = {
		event, aname, tname, ::gametic,
		{ ax, ay, az }, { tx, ty, tz },
		arg0, arg1, arg2
	};
	::wdlevents.push_back(evt);
}

/**
 * Log a WDL event when you have actor pointers.
 */
void M_LogActorWDLEvent(
	WDLEvents event, AActor* activator, AActor* target,
	int arg0, int arg1, int arg2
)
{
	if (!::wdlstate.recording)
		return;

	player_t* ap = NULL;
	if (activator != NULL && activator->type == MT_PLAYER)
		ap = activator->player;

	player_t* tp = NULL;
	if (target != NULL && target->type == MT_PLAYER)
		tp = target->player;

	M_LogWDLEvent(event, ap, tp, arg0, arg1, arg2);
}

/**
 * Possibly log a WDL accuracy miss.
 * 
 * Looks for an accuracy log somewhere in the backlog, if there is none, it's
 * a miss.
 */
void M_MaybeLogWDLAccuracyMiss(player_t* activator, int arg0, int arg1)
{
	if (!::wdlstate.recording)
		return;

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

	// See if we have an existing accuracy event for this tic.
	WDLEventLog::reverse_iterator it = ::wdlevents.rbegin();
	for (;it != ::wdlevents.rend(); ++it)
	{
		if ((*it).gametic != ::gametic)
		{
			// Whoops, we went a whole gametic without seeing an accuracy
			// to our name.
			break;
		}

		// Event type is the same?
		if ((*it).ev != WDL_EVENT_ACCURACY)
			continue;

		// Activator is the same?
		if ((*it).activator != activator->userinfo.netname)
			continue;

		// We found an existing accuracy event for this tic - bail out.
		return;
	}

	// Add the event to the log.
	WDLEvent evt = {
		WDL_EVENT_ACCURACY, aname, "", ::gametic,
		{ ax, ay, az }, { 0, 0, 0 },
		arg0, arg1, 0
	};
	::wdlevents.push_back(evt);
}

weapontype_t M_MODToWeapon(int mod)
{
	switch (mod)
	{
	case MOD_FIST:
		return wp_fist;
	case MOD_PISTOL:
		return wp_pistol;
	case MOD_SHOTGUN:
		return wp_shotgun;
	case MOD_CHAINGUN:
		return wp_chaingun;
	case MOD_ROCKET:
	case MOD_R_SPLASH:
		return wp_missile;
	case MOD_PLASMARIFLE:
		return wp_plasma;
	case MOD_BFG_BOOM:
	case MOD_BFG_SPLASH:
		return wp_bfg;
	case MOD_CHAINSAW:
		return wp_chainsaw;
	case MOD_SSHOTGUN:
		return wp_supershotgun;
	}

	return NUMWEAPONS;
}

void M_CommitWDLLog()
{
	if (!::wdlstate.recording)
		return;

	// See if we can write a file.
	std::string timestamp = GenerateTimestamp();
	std::string filename = ::wdlstate.logdir + "wdl_" + timestamp + ".log";
	FILE* fh = fopen(filename.c_str(), "w+");
	if (fh == NULL)
	{
		::wdlstate.recording = false;
		Printf(PRINT_HIGH, "wdlstats: Could not save\"%s\" for writing.\n", filename.c_str());
		return;
	}

	// Header
	fprintf(fh, "version=%d\n", 5);
	fprintf(fh, "time=%s\n", timestamp.c_str());
	fprintf(fh, "levelnum=%d\n", ::level.levelnum);
	fprintf(fh, "levelname=%s\n", ::level.level_name);
	fprintf(fh, "duration=%d\n", ::gametic - ::wdlstate.begintic);
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
			eit->ev, eit->activator.c_str(), eit->target.c_str(), eit->gametic,
			eit->apos[0], eit->apos[1], eit->apos[2],
			eit->tpos[0], eit->tpos[1], eit->tpos[2],
			eit->arg0, eit->arg1, eit->arg2);
	}

	fclose(fh);

	// Turn off stat recording global - it must be turned on again by the
	// log starter next go-around.
	::wdlstate.recording = false;

	Printf(PRINT_HIGH, "wdlstats: Log saved as \"%s\".\n", filename.c_str());
}

static void PrintWDLEvent(const WDLEvent& evt)
{
	// FIXME: Once we have access to StrFormat, dedupe this format string.
	//                 "ev,ac,tg,gt,ax,ay,az,tx,ty,tz,a0,a1,a2"
	Printf(PRINT_HIGH, "%d,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		evt.ev, evt.activator.c_str(), evt.target.c_str(), evt.gametic,
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
		"  ] wdlinfo state\n"
		"  Return relevant WDL stats state.\n\n"
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
		Printf(PRINT_HIGH, "%" PRIuSIZE " events found\n", ::wdlevents.size());
		return;
	}
	else if (stricmp(argv[1], "state") == 0)
	{
		// Count total events.
		Printf(PRINT_HIGH, "Currently recording?: %s\n", ::wdlstate.recording ? "Yes" : "No");
		Printf(PRINT_HIGH, "Directory to write logs to: \"%s\"\n", ::wdlstate.logdir.c_str());
		Printf(PRINT_HIGH, "Log starting gametic: %d\n", ::wdlstate.begintic);
		return;
	}
	else if (stricmp(argv[1], "tail") == 0)
	{
		// Show last 10 events.
		WDLEventLog::const_iterator it = ::wdlevents.end() - 10;
		if (it < ::wdlevents.begin())
			it = wdlevents.begin();

		Printf(PRINT_HIGH, "Showing last %" PRIdSIZE " events:\n", ::wdlevents.end() - it);
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
