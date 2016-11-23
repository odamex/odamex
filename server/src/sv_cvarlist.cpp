// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Server console variables
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"


// Log file settings
// -----------------

CVAR(			log_fulltimestamps, "0", "Extended timestamp info (dd/mm/yyyy hh:mm:ss)",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			log_packetdebug, "0", "Print debugging messages for each packet sent",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

// Server administrative settings
// ------------------------------

CVAR(			sv_motd, "Welcome to Odamex", "Message Of The Day to display to clients upon connecting",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(			sv_email, "email@domain.com", "Administrator email address",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(			sv_website, "http://odamex.net/", "Server or Admin website",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(			sv_waddownload,	"0", "Allow downloading of WAD files from this server",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(			sv_emptyreset, "0", "Reloads the current map when all players leave",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_emptyfreeze,  "0", "Freezes the game state when there are no players",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_globalspectatorchat, "1", "Players can see spectator chat",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR_RANGE(		sv_maxcorpses, "200", "Maximum corpses to appear on map",
				CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 65536.0f)

CVAR(			sv_clientcount,	"0", "Set to the number of connected players (for scripting)",
				CVARTYPE_BYTE, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(sv_maxclients, "4", "Maximum clients that can connect to a server",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR_RANGE_FUNC_DECL(sv_maxplayers, "4", "Maximum players that can join the game, the rest are spectators",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR_RANGE_FUNC_DECL(sv_maxplayersperteam, "3", "Maximum number of players that can be on a team",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR_FUNC_DECL(	join_password, "", "Clients can connect if they have this password",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	rcon_password, "", "Remote console password",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	sv_usemasters, "1", "Advertise on master servers",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_endmapscript, "",  "Script to run at end of each map (e.g. to choose next map)",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)	

CVAR(			sv_startmapscript, "", "Script to run at start of each map (e.g. to override cvars)",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)	

CVAR(			sv_curmap, "", "Set to the last played map",
				CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR(			sv_nextmap, "", "Set to the next map to be played",
				CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)	

CVAR(			sv_loopepisode, "0", "Determines whether Doom 1 episodes carry over",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)	

CVAR_FUNC_DECL(	sv_shufflemaplist, "0", "Randomly shuffle the maplist",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

// Network settings
// ----------------

CVAR_RANGE(		sv_natport,	"0", "NAT firewall workaround, this is a port number",
				CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 65536.0f)

CVAR_RANGE(		sv_flooddelay, "1.5", "Chat flood protection time (in seconds)",
				CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 10.0f)

CVAR_RANGE_FUNC_DECL(sv_maxrate, "200", "Forces clients to be on or below this rate",
				CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 7.0f, 100000.0f)

CVAR_RANGE_FUNC_DECL(sv_waddownloadcap, "200", "Cap wad file downloading to a specific rate",
				CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 7.0f, 100000.0f)

#ifdef ODA_HAVE_MINIUPNP
CVAR(			sv_upnp, "1", "Enable UPnP support",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR_RANGE(		sv_upnp_discovertimeout, "2000", "UPnP Router discovery timeout",
				CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 500.0f, 10000.0f)

CVAR(			sv_upnp_description, "",  "Router-side description of port mapping",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(			sv_upnp_internalip, "", "Set to the local machine IP address",
				CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR(			sv_upnp_externalip, "", "Set to the router IP address",
				CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)
#endif

// Gameplay settings
// =================

CVAR_RANGE(		sv_teamsinplay, "2", "Teams that are enabled",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR(			ctf_manualreturn, "0", "Flags dropped must be returned manually", 
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(			ctf_flagathometoscore, "1",  "Team flag must be at home pedestal for any captures " \
				"of an enemy flag returned to said pedestal to count as a point",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(		ctf_flagtimeout, "10",  "Time for a dropped flag to be returned automatically to its home base",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR(			sv_ticbuffer, "1", "Buffer controller input from players experiencing sudden " \
				"latency spikes for smoother movement",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)


// Ban settings
// ============

CVAR(			sv_banfile, "banlist.json", "Default file to save and load the banlist.",
				CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(		sv_banfile_reload, "0", "Number of seconds to wait between automatically loading the banlist.",
				CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 86400.0f)

// Vote settings
// =============

CVAR(			sv_vote_countabs, "1", "Count absent voters as 'no' if the vote timer runs out.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR_RANGE(		sv_vote_majority, "0.5", "Ratio of yes votes needed for vote to pass.",
				CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(			sv_vote_speccall, "1", "Spectators are allowed to callvote.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_vote_specvote, "1", "Spectators are allowed to vote.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR_RANGE(		sv_vote_timelimit, "30", "Amount of time a vote takes in seconds.",
				CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 120.0f)

CVAR_RANGE(		sv_vote_timeout, "60", "Timeout between votes in seconds.",
				CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 600.0f)

CVAR(			sv_callvote_coinflip, "0", "Clients can flip a coin.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_kick, "0", "Clients can votekick other players.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_forcespec, "0", "Clients can vote to force a player to spectate.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_forcestart, "0", "Clients can vote to force the match to start.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_map, "0", "Clients can vote to switch to a specific map from the server's maplist.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_nextmap, "0", "Clients can vote on progressing to the next map.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_randmap, "0", "Clients can vote to switch to a random map from the server's maplist.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_randcaps, "0", "Clients can vote to force the server to pick two players " \
				"from the pool of ingame players and force-spectate everyone else.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_randpickup, "0", "Clients can vote to force the server to pick a certian " \
				"number of players from the pool of ingame players and force-spectate everyone else.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_restart, "0", "Clients can vote to reload the current map.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_fraglimit, "0", "Clients can vote a new fraglimit.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_scorelimit, "0", "Clients can vote a new scorelimit.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

CVAR(			sv_callvote_timelimit, "0", "Clients can vote a new timelimit.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE)

// Warmup mode
CVAR(			sv_warmup, "0", "Enable a 'warmup mode' before the match starts.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR_RANGE(		sv_warmup_autostart, "1.0", "Ratio of players needed for warmup to automatically start the game.",
				CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE(		sv_countdown, "5", "Number of seconds to wait before starting the game from " \
				"warmup or restarting the game.",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 60.0f)

CVAR(			sv_warmup_pugs, "0", "Enables PUGs optimisations for teamgames.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR(			sv_warmup_overtime_enable, "0", "Enables overtime whenever a draw occurs.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR_RANGE(		sv_warmup_overtime, "2", "Number of minutes added during an overtime.",
				CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 10.0f)

// Experimental settings (all categories)
// =======================================

CVAR(			sv_dmfarspawn, "0", "EXPERIMENTAL: When enabled in DM, players will spawn at the farthest point " \
                "from each other.",
				CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

// Hacky abominations that should be purged with fire and brimstone
// =================================================================

// None currently

VERSION_CONTROL (sv_cvarlist_cpp, "$Id$")

