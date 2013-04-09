// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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
// Common console variables, used by both client and server.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"

// Server settings
// ---------------

// Game mode
CVAR (sv_gametype, "0", "Sets the game mode, values are:\n" \
                        "// 0 = Cooperative\n" \
                        "// 1 = Deathmatch\n" \
                        "// 2 = Team Deathmatch\n" \
                        "// 3 = Capture The Flag",
      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)

// (Coop/Teamplay/CTF): Players can injure others on the same team
CVAR (sv_friendlyfire, "1", "When set, players can injure others on the same team, it is ignored in deathmatch",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (Teamplay/CTF): Game ends when team score is reached
CVAR (sv_scorelimit, "5", "Game ends when team score is reached in Teamplay/CTF",
      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// (Teamplay/CTF) When disabled, treat team spawns like normal deathmatch spawns.
CVAR (sv_teamspawns, "1", "When disabled, treat team spawns like normal deathmatch spawns in Teamplay/CTF",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)
// Cheat code usage is allowed
CVAR (sv_allowcheats, "0", "Allow usage of cheats in all game modes",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Exit switch/teleports are usable
CVAR (sv_allowexit, "1", "Allow use of Exit switch/teleports in all game modes",
     CVARTYPE_BOOL,  CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Players can jump
CVAR (sv_allowjump, "0", "Allows players to jump when set in all game modes",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Give double ammo regardless of difficulty
CVAR (sv_doubleammo, "0", "Give double ammo regardless of difficulty",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Multiplier for player weapon damage
CVAR (sv_weapondamage, "1.0", "Amount to multiply player weapon damage by",
      CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Makes water movement more realistic
CVAR_FUNC_DECL (sv_forcewater, "0", "Makes water more realistic (boom maps at the moment)",
                CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Look up/down is allowed
CVAR (sv_freelook,      "0", "Allow Looking up and down",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Names of players appear in the FOV
CVAR (sv_allowtargetnames, "0", "When set, names of players appear in the FOV",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Game ends on frag limit being reached
CVAR (sv_fraglimit,     "0", "Sets the amount of frags a player can accumulate before the game ends",
      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters are at nightmare speed
CVAR (sv_fastmonsters,     "0", "Monsters are at nightmare speed",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Multiplier for monster damage
CVAR (sv_monsterdamage, "1.0", "Amount to multiply monster weapon damage by",
      CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// OLD: Allow exit switch at maxfrags, must click to exit
// NEW: When enabled, exit switch will kill the player who flips it
// [ML] NOTE: Behavior was changed October 2012, see bug
CVAR (sv_fragexitswitch,   "0", "When enabled, exit switch will kill the player who flips it",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Players will have infinite ammunition
CVAR (sv_infiniteammo,     "0", "Infinite ammo for all players",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Items will respawn after time
CVAR (sv_itemsrespawn,     "0", "Items will respawn after a fixed period, see sv_itemrespawntime",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// If itemrespawn is on, items will respawn after this time. (in seconds)
CVAR (sv_itemrespawntime,  "30", "If sv_itemsrespawn is set, items will respawn after this time (in seconds)",
      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters will respawn after time
CVAR (sv_monstersrespawn,  "0", "Monsters will respawn after a period of time",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Monsters are not present
CVAR (sv_nomonsters,    "0", "No monsters will be present",
     CVARTYPE_BOOL,  CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Monsters have a specific percentage of their normal health
CVAR (sv_monstershealth, "1.0", "Amount to multiply monster health by",
      CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Skill level
CVAR (sv_skill,             "3", "Sets the skill level, values are:\n" \
                                 "// 0 - No things mode\n" \
                                 "// 1 - I'm Too Young To Die\n" \
                                 "// 2 - Hey, Not Too Rough\n" \
                                 "// 3 - Hurt Me Plenty\n" \
                                 "// 4 - Ultra-Violence\n" \
                                 "// 5 - Nightmare",
      CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Game ends on time limit being reached
CVAR_FUNC_DECL (sv_timelimit,    "0", "Sets the time limit for the game to end, must be greater than 1",
      CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

// Intermission ends on intermission time limit being reached
CVAR_FUNC_DECL (sv_intermissionlimit, "10", "Sets the time limit for the intermission to end, 0 disables (defaults to 10 seconds)",
      CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

// Weapons stay
CVAR (sv_weaponstay,    "1", "Weapons stay after pickup",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Keep keys on death
CVAR(sv_keepkeys, "0", "Keep keys on death",
     CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// [SL] 2011-05-11 - Allow reconciliation for players on lagged connections
CVAR (sv_unlag,            "1", "Allow reconciliation for players on lagged connections",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
CVAR (sv_maxunlagtime,	"1.0", "Cap the maxiumum time allowed for player reconciliation",
      CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// [ML] allow weapon & view bob changing
CVAR (sv_allowmovebob, "0", "Allow weapon & view bob changing",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// [ML] Allow clients to adjust amount of red pain screen intensity
CVAR (sv_allowredscreen, "0","Allow clients to adjust amount of red pain screen intensity",
     CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

// [SL] Allow PWO
CVAR (sv_allowpwo, "0", "Allow clients to set their preferences for automatic weapon swithching",
   CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

// [AM] Allow true widescreen usage
CVAR (sv_allowwidescreen, "1", "Allow clients to use true widescreen",
      CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)

// [AM] Allow players to see the spawns on the map
CVAR (sv_allowshowspawns, "1", "Allow clients to see spawn points as particle fountains",
      CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)

// Compatibility options for vanilla
// ---------------------------------

// Enable/disable infinitely tall actors
CVAR (co_realactorheight, "0", "Enable/Disable infinitely tall actors",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// [Spleen] When enabled, monsters can get pushed or thrusted off of ledges, like in boom
CVAR (co_allowdropoff, "0", "When enabled, monsters can get pushed or thrusted off of ledges, like in boom",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// [ML] When enabled, activate correct impact of projectiles and bullets on surfaces (spawning puffs,explosions)
CVAR (co_fixweaponimpacts, "0", "When enabled, activate correct impact of projectiles and bullets on surfaces (spawning puffs,explosions)",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// [ML] When enabled, additional checks are made on two-sided lines, allows additional
// silent bfg tricks, and the player will "oof" on two-sided lines
CVAR (co_boomlinecheck, "0", "additional checks are made on two-sided lines, allows additional silent bfg tricks, and the player will \"oof\" on two-sided lines",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Use Boom's algorithm for determining the actors in, or at least touching, a sector
CVAR (co_boomsectortouch, "0", "Use a finer-grained, faster, and more accurate test for actors that are touching a sector (i.e. those affected if it moves)",
	  CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Enable/disable the "level 8 full sound at far distances" feature
CVAR (co_level8soundfeature, "0", "Enable/disable the \"level 8 full sound at far distances\" feature",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// Fix the blockmap collision bug
CVAR (co_blockmapfix, "0", "Fix the blockmap collision bug",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// Enable/disable zdoom-based gravity and physics interactions
CVAR (co_zdoomphys, "0", "Enable/disable zdoom-based gravity and physics interactions",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// 0 - Play the switch resetting sound at map location (0,0) like the vanilla bug
// 1 - switch sounds attenuate with distance like plats and doors.
CVAR (co_zdoomswitches, "0", "Play switch sounds attenuate with distance",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Use ZDoom's sound curve instead of vanilla Doom's
CVAR (co_zdoomsoundcurve, "0", "Use ZDoom's sound curve instead of vanilla Doom's",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Turns off the west-facing silent spawns vanilla bug
CVAR (co_nosilentspawns, "0", "Turns off the west-facing silent spawns vanilla bug",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Increase precision of autoatim
CVAR (co_fineautoaim, "0", "Increase precision of autoatim",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Dead player's view follows the actor who killed them
CVAR (cl_deathcam, "1", "Dead player's view follows the actor who killed them",
      CVARTYPE_BOOL, CVAR_ARCHIVE)

CVAR (cl_predictsectors, "1", "Move floors and ceilings immediately instead of waiting for confirmation",
     CVARTYPE_BOOL, CVAR_ARCHIVE)

CVAR (cl_predictpickup, "1", "Predict weapon pickups", CVARTYPE_BOOL, CVAR_ARCHIVE)

// Movebob
CVAR_FUNC_DECL (cl_movebob, "1.0", "Adjust weapon and movement bobbing",
      CVARTYPE_BOOL, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR (cl_rockettrails, "0", "Rocket trails on/off (currently unused)", CVARTYPE_BOOL,  CVAR_ARCHIVE)

// [AM] Force a player to respawn.
CVAR (sv_forcerespawn, "0", "Force a player to respawn.",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// [AM] Force a player to respawn after a set amount of time
CVAR (sv_forcerespawntime, "30", "Force a player to respawn after a set amount of time",
      CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
CVAR (co_zdoomspawndelay, "0", "Force a player to wait a second before respawning",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_FUNC_DECL (sv_gravity, "800", "Gravity of the environment",
      CVARTYPE_INT, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (sv_aircontrol, "0.00390625", "How much control the player has in the air",
      CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL (sv_splashfactor, "1.0", "Rocket explosion thrust effect?",
      CVARTYPE_FLOAT,  CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)


// Misc stuff
// ----------

// debugging mode
CVAR (developer, "0", "debugging mode",
      CVARTYPE_BOOL, CVAR_NULL)
// Port (???)
CVAR (port, "0", "Display currently used port number",
      CVARTYPE_INT, CVAR_NOSET | CVAR_NOENABLEDISABLE)
// Chase camera settings
CVAR (chase_height,     "-8", "Height of chase camera",
      CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chase_dist,    "90", "Chase camera distance",
      CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
// Generate centerview when -mlook encountered?
CVAR (lookspring,    "1", "Generate centerview when mlook encountered",
      CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
// Allows players to walk through other players
CVAR (sv_unblockplayers, "0", "Allows players to walk through other players",
      CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)
// [Spleen] Allow custom WAD directories to be specified in a cvar
CVAR (waddirs, "", "Allow custom WAD directories to be specified",
      CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// [Xyltol 02/27/2012] Hostname retrieval for Scoreboard
CVAR (sv_hostname,      "Untitled Odamex Server", "Server name to appear on masters, clients and launchers",
   CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE | CVAR_SERVERINFO)

// Experimental settings (all categories)
// =======================================

// Speedhack code (server related)
CVAR (sv_speedhackfix,     "0", "Experimental anti-speedhack code",
      CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

VERSION_CONTROL (c_cvarlist_cpp, "$Id$")
