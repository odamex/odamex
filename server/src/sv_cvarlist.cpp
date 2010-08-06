// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	Server console variables
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"


// Log file settings
// -----------------

// Extended timestamp info (dd/mm/yyyy hh:mm:ss)
CVAR (log_fulltimestamps, "0", CVAR_ARCHIVE)


// Server administrative settings
// ------------------------------

// Server "message of the day" that gets to displayed to clients upon connecting
CVAR (sv_motd,		"Welcome to Odamex",	CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Server name that appears to masters, clients and launchers
CVAR (sv_hostname,		"Untitled Odamex Server",	CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Administrator email address
CVAR (sv_email,		"email@domain.com",			CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Website of this server/other
CVAR (sv_website,      "http://odamex.net/",         CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Enables WAD file downloading
CVAR (sv_waddownload,	"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)
// Reset the current map when the last player leaves
CVAR (sv_emptyreset,   "0",        CVAR_ARCHIVE | CVAR_SERVERINFO)
// Allow spectators talk to show to ingame players
CVAR (sv_globalspectatorchat, "1", CVAR_ARCHIVE | CVAR_SERVERINFO)
// Maximum corpses that can appear on a map
CVAR (sv_maxcorpses, "200", CVAR_ARCHIVE | CVAR_SERVERINFO)
// Unused (tracks number of connected players for scripting)
CVAR (sv_clientcount,	"0", CVAR_NOSET | CVAR_NOENABLEDISABLE)
// Deprecated
CVAR (sv_cleanmaps, "", CVAR_NULL)
// Antiwallhack code
CVAR (sv_antiwallhack,	"0", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Maximum number of clients that can connect to the server
CVAR_FUNC_DECL (sv_maxclients, "4", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum number of players that can join the game, the rest are spectators
CVAR_FUNC_DECL (sv_maxplayers,	"4", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Clients can only join if they specify a password
CVAR_FUNC_DECL (join_password, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// TODO: document
CVAR_FUNC_DECL (spectate_password, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Remote console password
CVAR_FUNC_DECL (rcon_password, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Advertise on the master servers
CVAR_FUNC_DECL (sv_usemasters, "1", CVAR_ARCHIVE)
// script to run at end of each map (e.g. to choose next map)
CVAR (sv_endmapscript, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	
// script to run at start of each map (e.g. to override cvars)
CVAR (sv_startmapscript, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	
// tracks last played map
CVAR (sv_curmap, "", CVAR_NOSET | CVAR_NOENABLEDISABLE)
// tracks next map to be played			
CVAR (sv_nextmap, "", CVAR_NULL | CVAR_NOENABLEDISABLE)	
// Determines whether Doom 1 episodes should carry over.		
CVAR (sv_loopepisode, "0", CVAR_ARCHIVE)	
// Network compression (experimental)
CVAR (sv_networkcompression, "0", CVAR_ARCHIVE)
// NAT firewall workaround port number
CVAR (sv_natport,	"0", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// The time difference in which a player message to all players can be repeated
// in seconds
CVAR (sv_flooddelay, "1.5", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// GhostlyDeath <August 14, 2008> -- Randomize the map list
CVAR_FUNC_DECL (sv_shufflemaplist,	"0", CVAR_ARCHIVE)
// [Spleen] limits the rate of clients to avoid bandwidth issues
CVAR (sv_maxrate, "200000", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// Gameplay settings
// =================

// (Teamplay/CTF): Players can injure others on the same team
CVAR (sv_friendlyfire,		"1", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (Teamplay): Teams that are enabled
CVAR (sv_teamsinplay,		"2", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// (CTF) Flags dropped by the player must be returned manually
CVAR (ctf_manualreturn,	"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// TODO: document
CVAR (ctf_flagathometoscore,	"1", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (CTF) A flag that is dropped will be returned automatically after this timeout
CVAR (ctf_flagtimeout,	"600", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// (Teamplay/CTF) When disabled, treat team spawns like normal deathmatch spawns.
CVAR (sv_teamspawns, "1", CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

VERSION_CONTROL (sv_cvarlist_cpp, "$Id$")

