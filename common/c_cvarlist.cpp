// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
// Common console variables, used by both client and server.
//
//-----------------------------------------------------------------------------


#include "odamex.h"


// Server settings
// ---------------

// Game mode
CVAR_RANGE(sv_gametype, "0",
           "Sets the game mode, values are:\n"
           "// 0 = Cooperative\n"
           "// 1 = Deathmatch\n"
           "// 2 = Team Deathmatch\n"
           "// 3 = Capture The Flag\n"
           "// 4 = Horde\n",
           CVARTYPE_BYTE,
           CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f,
           4.0f)

CVAR(				sv_friendlyfire, "1", "When set, players can injure others on the same team, " \
					"it is ignored in deathmatch",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_scorelimit, "5", "Game ends when team score is reached in Teamplay/CTF",
					CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR(				sv_teamspawns, "1", "When disabled, treat team spawns like normal deathmatch " \
					"spawns in Teamplay/CTF",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

CVAR(				sv_playerbeacons, "0",	"When enabled, print player beacons in WDL events." \
					"This will take effect on map change.",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

CVAR(				sv_allowcheats, "0", "Allow usage of cheats in all game modes",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowexit, "1", "Allow use of Exit switch/teleports in all game modes",
					CVARTYPE_BOOL,  CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowjump, "0", "Allows players to jump when set in all game modes",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_doubleammo, "0", "Give double ammo regardless of difficulty",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_weapondamage, "1.0", "Amount to multiply player weapon damage by",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 100.0f)

CVAR_FUNC_DECL(		sv_forcewater, "0", "Makes water more realistic (boom maps at the moment)",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(				sv_freelook, "0", "Allow Looking up and down",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowtargetnames, "0", "When set, names of players appear in the FOV",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_fraglimit,     "0", "Sets the amount of frags a player can accumulate before the game ends",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 65536.0f)

CVAR(				sv_fastmonsters, "0", "Monsters are at nightmare speed",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_monsterdamage, "1.0", "Amount to multiply monster weapon damage by",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 100.0f)

// OLD: Allow exit switch at maxfrags, must click to exit
// NEW: When enabled, exit switch will kill the player who flips it
// [ML] NOTE: Behavior was changed October 2012, see bug
CVAR(				sv_fragexitswitch, "0", "When enabled, exit switch will kill the player who flips it",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_infiniteammo, "0", "Infinite ammo for all players",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_itemsrespawn, "0", "Items will respawn after a fixed period, see sv_itemrespawntime",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(				sv_respawnsuper, "0", "Allows Invisibility/Invulnerability spheres from respawning (need sv_itemsrespawn set to 1)",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR_RANGE(			sv_itemrespawntime, "30", "If sv_itemsrespawn is set, items will respawn after this " \
					"time (in seconds)",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 500.0f)

CVAR(				sv_monstersrespawn,  "0", "Monsters will respawn after a period of time",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_nomonsters, "0", "No monsters will be present",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)


CVAR_RANGE(			sv_monstershealth, "1.0", "Amount to multiply monster health by",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 100.0f)

CVAR_RANGE(			sv_skill,"3", "Sets the skill level, values are:\n" \
					"// 0 - No things mode\n" \
					"// 1 - I'm Too Young To Die\n" \
					"// 2 - Hey, Not Too Rough\n" \
					"// 3 - Hurt Me Plenty\n" \
					"// 4 - Ultra-Violence\n" \
					"// 5 - Nightmare",
					CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 7.0f)

CVAR_RANGE(sv_timelimit, "0", "Sets the time limit for the game to end (in minutes)",
           CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
           0.0f, 1440.0f)

CVAR_RANGE_FUNC_DECL(sv_intermissionlimit, "10", "Sets the time limit for the intermission to end (in seconds).",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 60.0f)

CVAR(				sv_weaponstay,    "1", "Weapons stay after pickup",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(				sv_keepkeys, "0", "Keep keys on death",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_FUNC_DECL(		sv_sharekeys, "0", "Share keys found to every player.",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_maxunlagtime, "1.0", "Cap the maxiumum time allowed for player reconciliation (in seconds)",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(				sv_allowmovebob, "1", "Allow weapon & view bob changing",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowredscreen, "1","Allow clients to adjust amount of red pain screen intensity",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

CVAR(				sv_allowpwo, "0", "Allow clients to set their preferences for automatic weapon switching",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

CVAR_FUNC_DECL(		sv_allowwidescreen, "1", "Allow clients to use true widescreen",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR(				sv_allowshowspawns, "1", "Allow clients to see spawn points as particle fountains",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR(				sv_forcerespawn, "0", "Force a player to respawn",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_forcerespawntime, "30", "Force a player to respawn after a set amount of time (in seconds)",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 32768.0f)

CVAR_RANGE(			sv_spawndelaytime, "0.0", "Force a player to wait a period (in seconds) before respawning",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 32768.0f)

CVAR(				sv_unblockplayers, "0", "Allows players to walk through other players, and player projectiles to pass through teammates.",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

CVAR(				sv_hostname, "Untitled Odamex Server", "Server name to appear on masters, clients and launchers",
					CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE | CVAR_SERVERINFO)

CVAR(sv_downloadsites, "",
     "A list of websites to download WAD files from, separated by spaces",
     CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)


// Game mode options
// -----------------

CVAR(g_gametypename, "",
     "A custom name for the gametype.  If blank, a default name is used based on "
     "currently set cvars.",
     CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE | CVAR_SERVERINFO)

CVAR_FUNC_DECL(g_spawninv, "default",
               "The default inventory a player should spawn with.  See the \"spawninv\" "
               "console command.",
               CVARTYPE_STRING,
               CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE | CVAR_SERVERINFO)

CVAR(g_ctf_notouchreturn, "0",
     "Prevents touch-return of the flag, forcing the player to wait for it to timeout",
     CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR(g_rounds, "0",
     "Turns on rounds - if enabled, reaching the gametype's win condition only wins a "
     "single round, and a player or team must win a certain number of rounds to win the "
     "game",
     CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH);

CVAR(g_sides, "0",
     "Turns on offensive vs defensive sides for team-based game modes - "
     "teams on offense don't have to defend any team objective, teams on defense win "
     "if time expires, and if rounds are enabled the team on defense changes every round",
     CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(g_roundlimit, "0",
     "Number of total rounds the game can go on for before the game ends", CVARTYPE_INT,
     CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_lives, "0",
     "Number of lives a player has before they can no longer respawn - this can result "
     "in a game loss depending on the gametype",
     CVARTYPE_INT,
     CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)

CVAR(g_lives_jointimer, "0",
     "When players have limited lives, this is the number of seconds after the round or "
     "game started that new players can join - if set to zero, players need to queue and "
     "wait until the next round to join",
     CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_winlimit, "0",
     "Number of times a round must be won before a player or team wins the game",
     CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_winnerstays, "0", "After a match winners stay in the game, losers get spectated.",
     CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(g_preroundtime, "5", "Amount of time before a round where you can't shoot",
     CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(g_preroundreset, "0", "After preround is over, reset the map one last time.",
     CVARTYPE_INT, CVAR_SERVERARCHIVE)

CVAR(g_postroundtime, "3", "Amount of time after a round before the next round/endgame",
     CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(g_thingfilter, "0", "Removes some things from the map. Values are:\n" \
           "// -1 - Multiplayer things are added in singleplayer.\n" \
           "// 0 - All things are retained (default).\n" \
           "// 1 - Only Coop weapons are removed.\n" \
           "// 2 - All Coop things are removed.\n" \
           "// 3 - All pickupable things are removed.\n",
           CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE | CVAR_LATCH,
           -1.0f, 3.0f)

CVAR(g_resetinvonexit, "0",
     "Always reset players to their starting inventory on level exit", CVARTYPE_BOOL,
     CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_horde_waves, "5", "Number of horde waves per map", CVARTYPE_INT,
     CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_horde_mintotalhp, "4.0", "Multiplier for minimum spawned health at a time",
     CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_horde_maxtotalhp, "10.0", "Multiplier for maximum spawned health at a time",
     CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(g_horde_goalhp, "8.0", "Goal health multiplier for a given round", CVARTYPE_FLOAT,
     CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR_RANGE(g_horde_spawnempty_min, "1",
           "Minimum number of seconds it takes to spawn a monster in an empty horde map",
           CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 1,
           60)

CVAR_RANGE(g_horde_spawnempty_max, "3",
           "Maximum number of seconds it takes to spawn a monster in an empty horde map",
           CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 1,
           60)

CVAR_RANGE(g_horde_spawnfull_min, "2",
           "Minimum number of seconds it takes to spawn a monster in a full horde map",
           CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 1,
           60)

CVAR_RANGE(g_horde_spawnfull_max, "6",
           "Maximum number of seconds it takes to spawn a monster in a full horde map",
           CVARTYPE_INT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 1,
           60)

// Game mode options commonized from the server
//     At some point, replace "sv_" with "g_"
// -------------------------------------------------------------------------

CVAR(sv_warmup, "0", "Enable a 'warmup mode' before the match starts.", CVARTYPE_BOOL,
     CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)
CVAR_RANGE(sv_warmup_autostart, "1.0",
           "Ratio of players needed for warmup to automatically start the game.",
           CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f,
           1.0f)
CVAR_RANGE(sv_countdown, "5",
           "Number of seconds to wait before starting the game from "
           "warmup or restarting the game.",
           CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f,
           60.0f)

// Compatibility options
// ---------------------------------

	// Fixes to Vanilla
	//------------------------------
	CVAR(			co_realactorheight, "0", "Enable/Disable infinitely tall actors",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

	CVAR(			co_nosilentspawns, "0", "Turns off the west-facing silent spawns vanilla bug",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_fixweaponimpacts, "0", "Corrected behavior for impact of projectiles and bullets on surfaces",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_blockmapfix, "0", "Fix the blockmap collision bug",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)


	// Boom-compatibility changes
	//------------------------------
	CVAR(			co_boomphys, "0", "Use a finer-grained, faster, and more accurate test for actors, " \
					"sectors and lines",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_allowdropoff, "0", "Allow monsters can get pushed or thrusted off of ledges",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

	CVAR(			co_novileghosts, "0", "Disables vanilla's ghost monster quirk that lets Arch-viles resurrect crushed monsters as unshootable ghosts",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

	CVAR(			co_removesoullimit, "0", "Allows pain elementals to still spawn lost souls if more than 20 are present",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)



	// ZDoom-compatibility changes
	//------------------------------
	CVAR(			co_zdoomphys, "0", "Enable/disable ZDoom-based gravity and physics interactions",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_zdoomsound, "0", "Enable sound attenuation curve + attenuation of switch sounds with distance",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_fineautoaim, "0", "Increase precision of vertical auto-aim",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_globalsound, "0", "Make pickup sounds global", CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)


// Client options
// ---------------------

CVAR(				cl_deathcam, "1", "Dead player's view follows the actor who killed them",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(			cl_predictsectors, "1", "Move floors and ceilings immediately instead of waiting for confirmation, values are:\n"
					"// 0 - Don't predict any sectors\n" \
					"// 1 - Predict all sectors based only on actor movements\n" \
					"// 2 - Predict only sectors activated by you\n",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR(				cl_predictpickup, "1", "Predict weapon pickups",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(			cl_movebob, "1.0", "Adjust weapon and movement bobbing",
					CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(				cl_centerbobonfire, "0",
					"Centers the weapon bobbing when firing a weapon",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE_FUNC_DECL(sv_gravity, "800", "Gravity of the environment",
					CVARTYPE_WORD, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
					0.0f, 32768.0f)

CVAR_RANGE_FUNC_DECL(sv_aircontrol, "0.00390625", "How much control the player has in the air",
					CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
					0.0f, 32768.0f)

CVAR_RANGE_FUNC_DECL(sv_splashfactor, "1.0", "Rocket explosion thrust effect?",
					CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
					0.01f, 100.0f)

CVAR(               cl_waddownloaddir, "", "Set custom WAD download directory",
					CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR				(r_softinvulneffect, "1",
					"Change invuln to enable light googles and invert the pallete on the weapon sprite only.",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

// Misc stuff
// ----------

CVAR(				developer, "0", "Debugging mode",
					CVARTYPE_BOOL, CVAR_NULL)

CVAR(debug_disconnect, "0", "Show source file:line where a disconnect happens",
     CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(		language, "auto", "Language to use for ingame strings",
					CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(				port, "0", "Display currently used network port number",
					CVARTYPE_INT, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR_RANGE(			chase_height, "-8", "Height of chase camera",
					CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, -32768.0f, 32768.0f)

CVAR_RANGE(			chase_dist, "90", "Chase camera distance",
					CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, -32768.0f, 32768.0f)

CVAR(				lookspring, "1", "Generate centerview when mlook encountered",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				waddirs, "", "Allow custom WAD directories to be specified",
					CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(net_rcvbuf, "131072", "Net receive buffer size in bytes",
					CVARTYPE_INT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE,
					1500.0f, 256.0f * 1024.0f * 1024.0f)

CVAR_RANGE_FUNC_DECL(net_sndbuf, "131072", "Net send buffer size in bytes",
					CVARTYPE_INT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE,
					1500.0f, 256.0f * 1024.0f * 1024.0f)

// Experimental settings (all categories)
// =======================================

CVAR(				sv_weapondrop, "0", "Enable/disable weapon drop.",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

VERSION_CONTROL (c_cvarlist_cpp, "$Id$")
