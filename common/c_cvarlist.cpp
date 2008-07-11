// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: c_cvarlist.cpp 971 2008-07-03 00:56:27Z russellrice $
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
//	Common console variables, used by both client and server.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"

// Misc stuff
// ----------

// debugging mode
CVAR (developer, "0", CVAR_NULL)

// Game modes and settings
// -----------------------

// Deathmatch game mode
CVAR (deathmatch,		"0", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Teamplay game mode
CVAR (teamplay,			"0", CVAR_SERVERINFO | CVAR_LATCH)
// Capture The Flag game mode
CVAR (usectf,			"0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH)
// (Teamplay/CTF): Players can injure others on the same team
CVAR (friendlyfire,		"1", CVAR_SERVERINFO)
// (Teamplay/CTF): Game ends when team score is reached
CVAR (scorelimit,		"10", CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// (Teamplay/CTF): Teams that are enabled
CVAR (blueteam,			"1",  CVAR_SERVERINFO | CVAR_ARCHIVE)
CVAR (redteam,			"1",  CVAR_SERVERINFO | CVAR_ARCHIVE)
CVAR (goldteam,			"0",  CVAR_SERVERINFO | CVAR_ARCHIVE)

// Cheat code usage is allowed
CVAR (allowcheats,		"0", CVAR_SERVERINFO | CVAR_LATCH)
// Exit switch/teleports are usable
CVAR (allowexit,		"0", CVAR_SERVERINFO)
// Players can jump
CVAR (allowjump,		"0", CVAR_SERVERINFO)
// Look up/down is allowed
CVAR (allowfreelook,	"0", CVAR_SERVERINFO)
// Names of players appear in the FOV
CVAR (allowtargetnames, "0", CVAR_SERVERINFO)
// Game ends on frag limit being reached
CVAR (fraglimit,		"0", CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters are at nightmare speed
CVAR (fastmonsters,		"0", CVAR_SERVERINFO)
// Allow exit switch at maxfrags, must click to exit
CVAR (fragexitswitch,   "0", CVAR_SERVERINFO)
// Players will have infinite ammunition
CVAR (infiniteammo,		"0", CVAR_SERVERINFO)
// Items will respawn after time
CVAR (itemsrespawn,		"0", CVAR_SERVERINFO | CVAR_LATCH)
// Monsters will respawn after time
CVAR (monstersrespawn,	"0", CVAR_SERVERINFO)
// Monsters are not present
CVAR (nomonsters,		"0", CVAR_SERVERINFO | CVAR_LATCH)
// Skill level
CVAR (skill,            "3", CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Game ends on time limit being reached
CVAR (timelimit,		"0", CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Speedhack code (server related)
CVAR (speedhackfix, "0", CVAR_SERVERINFO)
// File compression (???)
CVAR (filecompression, "1", CVAR_ARCHIVE)
// Chase camera settings
CVAR (chase_height, "-8", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chase_dist, "90", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

VERSION_CONTROL (c_cvarlist_cpp, "$Id: c_cvarlist.cpp 971 2008-07-03 00:56:27Z russellrice $")
