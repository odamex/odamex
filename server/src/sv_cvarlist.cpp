// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2010 by The Odamex Team.
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
CVAR (log_fulltimestamps, "0", "Extended timestamp info (dd/mm/yyyy hh:mm:ss)",
      CVARTYPE_BOOL, CVAR_ARCHIVE)


// Server administrative settings
// ------------------------------

// Server "message of the day" that gets to displayed to clients upon connecting
CVAR (sv_motd,		"Welcome to Odamex", "Message Of The Day", CVARTYPE_STRING,
      CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Server name that appears to masters, clients and launchers
CVAR (sv_hostname,		"Untitled Odamex Server", "Server name to appear on masters, clients and launchers",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Administrator email address
CVAR (sv_email,		"email@domain.com", "Administrator email address",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Website of this server/other
CVAR (sv_website,      "http://odamex.net/", "Server or Admin website",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Enables WAD file downloading
CVAR (sv_waddownload,	"0", "Allow downloading of WAD files from this server",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)
// Enables WAD file download cap
CVAR (sv_waddownloadcap, "200", "Cap wad file downloading to a specific rate",
      CVARTYPE_INT, CVAR_ARCHIVE | CVAR_SERVERINFO)
// Reset the current map when the last player leaves
CVAR (sv_emptyreset,   "0", "Reloads the current map when all players leave",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)
// Do not run ticker functions when there are no players
CVAR (sv_emptyfreeze,  "0", "Does not progress the game when there are no players",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)
// Allow spectators talk to show to ingame players
CVAR (sv_globalspectatorchat, "1", "Players can see spectator chat",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)
// Maximum corpses that can appear on a map
CVAR (sv_maxcorpses, "200", "Maximum corpses to appear on map",
      CVARTYPE_WORD, CVAR_ARCHIVE | CVAR_SERVERINFO)
// Unused (tracks number of connected players for scripting)
CVAR (sv_clientcount,	"0", "Don't use",
      CVARTYPE_BYTE, CVAR_NOSET | CVAR_NOENABLEDISABLE)
// Deprecated
CVAR (sv_cleanmaps, "", "Deprecated",
      CVARTYPE_NONE, CVAR_NULL)
// Anti-wall hack code
CVAR (sv_antiwallhack,	"0", "Experimental anti-wallkhack code",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Maximum number of clients that can connect to the server
CVAR_FUNC_DECL (sv_maxclients, "4", "Maximum clients that can connect to a server",
      CVARTYPE_BYTE, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum number of players that can join the game, the rest are spectators
CVAR_FUNC_DECL (sv_maxplayers,	"4", "Maximum players that can join the game, the rest are spectators",
      CVARTYPE_BYTE, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Clients can only join if they specify a password
CVAR_FUNC_DECL (join_password, "", "Clients can connect if they have this password",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// TODO: document
CVAR_FUNC_DECL (spectate_password, "", "",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Remote console password
CVAR_FUNC_DECL (rcon_password, "", "Remote console password",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Advertise on the master servers
CVAR_FUNC_DECL (sv_usemasters, "1", "Advertise on master servers",
      CVARTYPE_BOOL, CVAR_ARCHIVE)
// script to run at end of each map (e.g. to choose next map)
CVAR (sv_endmapscript, "",  "Script to run at end of each map (e.g. to choose next map)",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	
// script to run at start of each map (e.g. to override cvars)
CVAR (sv_startmapscript, "", "Script to run at start of each map (e.g. to override cvars)",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	
// tracks last played map
CVAR (sv_curmap, "", "Tracks last played map",
      CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)
// tracks next map to be played			
CVAR (sv_nextmap, "", "Tracks next map to be played",
      CVARTYPE_STRING, CVAR_NULL | CVAR_NOENABLEDISABLE)	
// Determines whether Doom 1 episodes should carry over.		
CVAR (sv_loopepisode, "0", "Determines whether Doom 1 episodes carry over",
      CVARTYPE_BOOL, CVAR_ARCHIVE)	

// Network settings
// ----------------

// Network compression (experimental)
CVAR (sv_networkcompression, "0", "Experimental network compression",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)
// NAT firewall workaround port number
CVAR (sv_natport,	"0", "NAT firewall workaround, this is a port number",
      CVARTYPE_INT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// The time difference in which a player message to all players can be repeated
// in seconds
CVAR (sv_flooddelay, "1.5", "Flood protection time",
      CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// GhostlyDeath <August 14, 2008> -- Randomize the map list
CVAR_FUNC_DECL (sv_shufflemaplist,	"0", "Randomly shuffle the maplist",
      CVARTYPE_BOOL, CVAR_ARCHIVE)
// [Spleen] limits the rate of clients to avoid bandwidth issues
CVAR_FUNC_DECL (sv_maxrate, "200", "Forces clients to be on or below this rate",
      CVARTYPE_INT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#ifdef ODA_HAVE_MINIUPNP
// Enable Universal Plug and Play to auto-configure a compliant router
CVAR (sv_upnp, "1", "UPnP support",
      CVARTYPE_BOOL, CVAR_ARCHIVE)
// The timeout looking for upnp routers
CVAR (sv_upnp_discovertimeout, "2000", "UPnP Router discovery timeout",
      CVARTYPE_INT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Description for the port mapping
CVAR (sv_upnp_description, "",  "Router-side description of port mapping",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Used to get the internal IP address of the local computer, unsettable
CVAR (sv_upnp_internalip, "", "Local machine IP address, unsettable",
      CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)
// Used to get the external IP address of the router, unsettable
CVAR (sv_upnp_externalip, "", "Router IP address, unsettable",
      CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)
#endif

// Gameplay settings
// =================

// (CTF/Team play): Teams that are enabled
CVAR (sv_teamsinplay, "2", "Teams that are enabled", CVARTYPE_BYTE,
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// (CTF) Flags dropped by the player must be returned manually
CVAR (ctf_manualreturn,	"0", "Flags dropped must be returned manually", 
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (CTF) Team flag must be at home pedestal for any captures of an enemy flag
// returned to said pedestal to count as a point
CVAR (ctf_flagathometoscore, "1",  "Team flag must be at home pedestal for any captures of an enemy flag returned to said pedestal to count as a point",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (CTF) Time for a dropped flag to be returned to its home base, in seconds
CVAR (ctf_flagtimeout, "10",  "Time for a dropped flag to be returned automatically to its home base",
      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// (ALL) Display a countdown to all players during intermission screen
//CVAR (sv_inttimecountdown, "0",  "Display time left for an intermission screen to next map",
//      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR (sv_ticbuffer, "0", "Buffer controller input from players experiencing sudden latency spikes for smoother movement",
	  CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

VERSION_CONTROL (sv_cvarlist_cpp, "$Id$")

