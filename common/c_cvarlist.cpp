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
//	Common console variables, used by both client and server.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"

// Server settings
// ---------------

// Game mode
CVAR (sv_gametype,			"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// (Teamplay/CTF): Players can injure others on the same team
CVAR (sv_friendlyfire,		"1", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (Teamplay/CTF): Game ends when team score is reached
CVAR (sv_scorelimit,		"5", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// (Teamplay/CTF) When disabled, treat team spawns like normal deathmatch spawns.
CVAR (sv_teamspawns, "1", CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)
// Cheat code usage is allowed
CVAR (sv_allowcheats,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Exit switch/teleports are usable
CVAR (sv_allowexit,		"1", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Players can jump
CVAR (sv_allowjump,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Give double ammo regardless of difficulty
CVAR (sv_doubleammo,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Makes water movement more realistic (?)
CVAR_FUNC_DECL (sv_forcewater, "0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Look up/down is allowed
CVAR (sv_freelook,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Names of players appear in the FOV
CVAR (sv_allowtargetnames, "0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Game ends on frag limit being reached
CVAR (sv_fraglimit,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters are at nightmare speed
CVAR (sv_fastmonsters,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Allow exit switch at maxfrags, must click to exit
CVAR (sv_fragexitswitch,   "0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Players will have infinite ammunition
CVAR (sv_infiniteammo,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Items will respawn after time
CVAR (sv_itemsrespawn,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// If itemrespawn is on, items will respawn after this time. (in seconds)
CVAR (sv_itemrespawntime,	"30", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters will respawn after time
CVAR (sv_monstersrespawn,	"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Monsters are not present
CVAR (sv_nomonsters,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Skill level
CVAR (sv_skill,            "3", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Game ends on time limit being reached
CVAR (sv_timelimit,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Speedhack code (server related)
CVAR (sv_speedhackfix,		"0", CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Weapons stay
CVAR (sv_weaponstay,		"1", CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// Compatibility options for vanilla
// ---------------------------------

// Enable/disable inifnitely tall actors
CVAR (co_realactorheight, "0", CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// [Spleen] When enabled, monsters can get pushed or thrusted off of ledges, like in boom
CVAR (co_allowdropoff, "0", CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// [ML] When enabled, additional checks are made on two-sided lines, allows additional 
// silent bfg tricks, and the player will "oof" on two-sided lines
CVAR (co_boomlinecheck, "0", CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Enable/disable the "level 8 full sound at far distances" feature
CVAR (co_level8soundfeature, "0", CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// Misc stuff
// ----------

// debugging mode
CVAR (developer, "0", CVAR_NULL)
// Port (???)
CVAR (port, "0", CVAR_NOSET | CVAR_NOENABLEDISABLE)
// File compression (???)
CVAR (filecompression,	"1", CVAR_ARCHIVE)
// Chase camera settings
CVAR (chase_height,		"-8", CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chase_dist,		"90", CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
// Generate centerview when -mlook encountered?
CVAR (lookspring,		"1", CVAR_CLIENTARCHIVE)
// Allows players to walk through other players
CVAR (sv_unblockplayers, "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)
// [Spleen] Allow custom WAD directories to be specified in a cvar
CVAR (waddirs, "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

VERSION_CONTROL (c_cvarlist_cpp, "$Id$")
