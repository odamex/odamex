// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: sv_cvarlist.cpp 984 2008-07-11 08:54:56Z russellrice $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2008 by The Odamex Team.
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


// Server administrative settings
// ------------------------------

// Server name that appears to masters, clients and launchers
CVAR (hostname,		"Untitled Odamex Server",	CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Administrator email address
CVAR (email,		"email@domain.com",			CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Website of this server/other
CVAR (website,      "http:\\odamex.net/",         CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Enables WAD file downloading
CVAR (waddownload,	"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)
// Reset the current map when the last player leaves
CVAR (emptyreset,   "0",        CVAR_ARCHIVE | CVAR_SERVERINFO)
// Allow spectators talk to show to ingame players
CVAR (globalspectatorchat, "1", CVAR_ARCHIVE | CVAR_SERVERINFO)
// Maximum corpses that can appear on a map
CVAR (maxcorpses, "200", CVAR_ARCHIVE | CVAR_SERVERINFO)
// Unused (tracks number of connected players for scripting)
CVAR (clientcount,	"0", CVAR_NOSET | CVAR_NOENABLEDISABLE)
// Deprecated
CVAR (cleanmaps, "", CVAR_NULL)
// Antiwallhack code
CVAR (antiwallhack,	"0", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Maximum number of clients that can connect to the server
CVAR_FUNC_DECL (maxclients, "4", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum number of players that can join the game, the rest are spectators
CVAR_FUNC_DECL (maxplayers,	"4", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Clients can only join if they specify a password
CVAR_FUNC_DECL (password, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Todo: document
CVAR_FUNC_DECL (spectate_password, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Remote console password
CVAR_FUNC_DECL (rcon_password, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Advertise on the master servers
CVAR_FUNC_DECL (usemasters, "1", CVAR_ARCHIVE)
// script to run at end of each map (e.g. to choose next map)
CVAR (endmapscript, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	
// script to run at start of each map (e.g. to override cvars)
CVAR (startmapscript, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	
// tracks last played map
CVAR (curmap, "", CVAR_NOSET | CVAR_NOENABLEDISABLE)
// tracks next map to be played			
CVAR (nextmap, "", CVAR_NULL | CVAR_NOENABLEDISABLE)	
// Determines whether Doom 1 episodes should carry over.		
CVAR (loopepisode, "0", CVAR_ARCHIVE)	
// Network compression (experimental)
CVAR (networkcompression, "0", CVAR_ARCHIVE)
// NAT firewall workaround port number
CVAR (natport,	"0", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// The time difference in which a player message to all players can be repeated
// in seconds
CVAR (flooddelay, "1.5", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

VERSION_CONTROL (sv_cvarlist_cpp, "$Id: sv_cvarlist.cpp 984 2008-07-11 08:54:56Z russellrice $")
