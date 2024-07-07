// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	CL_MAIN
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "gstrings.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"
#include "gi.h"
#include "i_net.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "st_stuff.h"
#include "m_argv.h"
#include "cl_main.h"
#include "c_effect.h"
#include "c_console.h"
#include "d_main.h"
#include "p_ctf.h"
#include "m_random.h"
#include "m_memio.h"
#include "w_ident.h"
#include "md5.h"
#include "m_fileio.h"
#include "r_sky.h"
#include "cl_demo.h"
#include "cl_download.h"
#include "cl_maplist.h"
#include "cl_vote.h"
#include "p_mobj.h"
#include "p_snapshot.h"
#include "p_lnspec.h"
#include "cl_netgraph.h"
#include "p_pspr.h"
#include "d_netcmd.h"
#include "g_levelstate.h"
#include "v_text.h"
#include "hu_stuff.h"
#include "p_acs.h"
#include "i_input.h"

#include "g_gametype.h"
#include "cl_parse.h"
#include "cl_replay.h"
#include "cl_state.h"
#include "clc_message.h"
#include "cl_connect.h"

#include <bitset>
#include <map>
#include <set>
#include <sstream>

#include "server.pb.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#if _MSC_VER == 1310
#pragma optimize("",off)
#endif

// denis - fancy gfx, but no game manipulation
bool clientside = true, serverside = false;
baseapp_t baseapp = client;

gameplatform_t platform;

extern bool step_mode;

// denis - client version (VERSION or other supported)
short version = 0;
int gameversion = 0;				// GhostlyDeath -- Bigger Game Version
int gameversiontosend = 0;		// If the server is 0.4, let's fake our client info

buf_t net_buffer(MAX_UDP_PACKET);
buf_t write_buffer(MAX_UDP_PACKET);

bool      noservermsgs;
int       last_received;

// [SL] 2012-03-17 - world_index is the gametic on the server that the client
// is currently simulating.  world_index_accum is a continuous accumulator that
// is used to advance world_index if appropriate.
int       world_index = 0;
float     world_index_accum = 0.0f;

int       last_svgametic = 0;
int       last_player_update = 0;

// denis - unique session key provided by the server
std::string digest;

// denis - clientside compressor, used for decompression
//huffman_client compressor;

// [SL] 2011-06-27 - Class to record and playback network recordings
NetDemo netdemo;
// [SL] 2011-07-06 - not really connected (playing back a netdemo)
bool simulated_connection = false;

NetCommand localcmds[MAXSAVETICS];

extern NetGraph netgraph;

// [SL] 2012-03-07 - Players that were teleported during the current gametic
std::set<byte> teleported_players;

// [SL] 2012-04-06 - moving sector snapshots received from the server
std::map<unsigned short, SectorSnapshotManager> sector_snaps;

EXTERN_CVAR (sv_teamsinplay)

EXTERN_CVAR (cl_predictsectors)

EXTERN_CVAR (cl_interp)

// [SL] Force enemies to have the specified color
EXTERN_CVAR (r_forceenemycolor)
EXTERN_CVAR (r_forceteamcolor)

static argb_t enemycolor, teamcolor;

void P_PlayerLeavesGame(player_s* player);

//
// CL_ShadePlayerColor
//
// Shades base_color darker using the intensity of shade_color.
//
argb_t CL_ShadePlayerColor(argb_t base_color, argb_t shade_color)
{
	if (base_color == shade_color)
		return base_color;

	fahsv_t color = V_RGBtoHSV(base_color);
	color.setv(0.7f * color.getv() + 0.3f * V_RGBtoHSV(shade_color).getv());
	return V_HSVtoRGB(color);
}

//
// CL_GetPlayerColor
//
// Returns the color for the player after applying game logic (teammate, enemy)
// and applying CVARs like r_forceteamcolor and r_forceenemycolor.
//
argb_t CL_GetPlayerColor(player_t *player)
{
	if (!player)
		return 0;

	argb_t base_color(255, player->userinfo.color[1], player->userinfo.color[2], player->userinfo.color[3]);
	argb_t shade_color = base_color;
	
	bool teammate = false;
	if (G_IsCoopGame())
		teammate = true;
	if (G_IsFFAGame())
		teammate = false;
	if (G_IsTeamGame())
	{
		teammate = P_AreTeammates(consoleplayer(), *player);
		base_color = GetTeamInfo(player->userinfo.team)->Color;
	}
	if (player->id != consoleplayer_id && !consoleplayer().spectator)
	{
		if (r_forceteamcolor && teammate)
			base_color = teamcolor;
		else if (r_forceenemycolor && !teammate)
			base_color = enemycolor;
	}

	return CL_ShadePlayerColor(base_color, shade_color);
}



static void CL_RebuildAllPlayerTranslations()
{
	// [SL] vanilla demo colors override
	if (demoplayback)
		return;

	for (auto& player : ::players)
	{
		R_BuildPlayerTranslation(player.id, CL_GetPlayerColor(&player));
	}
}

CVAR_FUNC_IMPL (r_enemycolor)
{
	// cache the color whenever the user changes it
	enemycolor = argb_t(V_GetColorFromString(var));
	CL_RebuildAllPlayerTranslations();
}

CVAR_FUNC_IMPL (r_teamcolor)
{
	// cache the color whenever the user changes it
	teamcolor = argb_t(V_GetColorFromString(var));
	CL_RebuildAllPlayerTranslations();
}

CVAR_FUNC_IMPL (r_forceenemycolor)
{
	CL_RebuildAllPlayerTranslations();
}

CVAR_FUNC_IMPL (r_forceteamcolor)
{
	CL_RebuildAllPlayerTranslations();
}

CVAR_FUNC_IMPL (cl_team)
{
	if (var.asInt() >= sv_teamsinplay)
		var.Set(sv_teamsinplay.asInt() - 1);
	
	CL_RebuildAllPlayerTranslations();
}

EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (sv_maxclients)
EXTERN_CVAR (sv_infiniteammo)
EXTERN_CVAR (sv_nomonsters)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (sv_allowredscreen)
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_itemsrespawn)
EXTERN_CVAR (sv_allowcheats)
EXTERN_CVAR (sv_allowtargetnames)
EXTERN_CVAR (sv_keepkeys)
EXTERN_CVAR (cl_mouselook)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (cl_disconnectalert)
EXTERN_CVAR (waddirs)

void CL_PlayerTimes (void);
void CL_Decompress();

bool M_FindFreeName(std::string &filename, const std::string &extension);

void CL_SimulateWorld();

// some doom functions (not csDoom)
void D_Display(void);
void D_DoAdvanceDemo(void);
void M_Ticker(void);

void R_InterpolationTicker();

size_t P_NumPlayersInGame();
void G_PlayerReborn (player_t &player);
void P_KillMobj (AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void P_SetPsprite (player_t *player, int position, statenum_t stnum);
void P_ExplodeMissile (AActor* mo);
void P_CalcHeight (player_t *player);
bool P_CheckMissileSpawn (AActor* th);

void P_PlayerLookUpDown (player_t *p);
team_t D_TeamByName (const char *team);
gender_t D_GenderByName (const char *gender);
void AM_Stop();

//
// CL_CalculateWorldIndexSync
//
// Calculates world_index based on the most recently received gametic from the
// server and the number of tics the client wants to withold for interpolation.
//
static int CL_CalculateWorldIndexSync()
{
	return last_svgametic ? last_svgametic - cl_interp : 0;
}

//
// CL_CalculateWorldIndexDriftCorrection
//
// [SL] 2012-03-17 - Try to maintain sync with the server by gradually
// slowing down or speeding up world_index
//
static int CL_CalculateWorldIndexDriftCorrection()
{
	static const float CORRECTION_PERIOD = 1.0f / 16.0f;

	int delta = CL_CalculateWorldIndexSync() - world_index;
	if (delta == 0)
		world_index_accum = 0.0f;
	else
		world_index_accum += CORRECTION_PERIOD * delta;

	// truncate the decimal portion of world_index_accum
	int correction = int(world_index_accum);

	// reset world_index_accum if our correction will affect world_index
	if (correction != 0)
		world_index_accum = 0.0f;

	return correction;
}

//
// CL_ResyncWorldIndex
//
// Recalculate world_index based and resets world_index_accum, which keeps
// track of how much the sync has drifted.
//
void CL_ResyncWorldIndex()
{
	world_index = CL_CalculateWorldIndexSync();
	world_index_accum = 0.0f;
}

void Host_EndGame(const char *msg)
{
    Printf("%s", msg);
	CL_QuitNetGame(NQ_SILENT);
}

std::string spyplayername;
void CL_CheckDisplayPlayer(void);

//
// CL_CheckDisplayPlayer
//
// Perfoms validation on the value of displayplayer_id based on the current
// game state and status of the consoleplayer.
//
void CL_CheckDisplayPlayer(void)
{
	static byte previd = consoleplayer_id;
	byte newid = 0;

	// [jsd]: try to spy on player by name when connected if spyplayername is set:
	if (spyplayername.length() > 0) {
		player_t &spyplayer = nameplayer(spyplayername);
		if (validplayer(spyplayer)) {
			displayplayer_id = spyplayer.id;
		}
	}

	if (displayplayer_id != previd)
		newid = displayplayer_id;

	if (!validplayer(displayplayer()) || !displayplayer().mo)
		newid = consoleplayer_id;

	if (!P_CanSpy(consoleplayer(), displayplayer(), demoplayback || netdemo.isPlaying() || netdemo.isPaused()))
		newid = consoleplayer_id;

	if (displayplayer().spectator)
		newid = consoleplayer_id;

	if (newid)
	{
		// Request information about this player from the server
		// (weapons, ammo, health, etc)
		MSG_WriteCLC(&write_buffer, CLC_Spy(newid));
		displayplayer_id = newid;

		// Changing display player can sometimes affect status bar visibility
		// since the status bar isn't visible when display player is a spectator.
		// The status bar needs to be refreshed as well because the status bar face
		// widget background color changes.
		if (idplayer(newid).spectator != idplayer(previd).spectator)
			R_ForceViewWindowResize();
		ST_ForceRefresh();

		previd = newid;
	}

}

//
// CL_SpyCycle
//
// Cycles through the point-of-view of players in the game.  Checks
// are made to ensure only spectators can view enemy players.
//
template<class Iterator>
void CL_SpyCycle(Iterator begin, Iterator end)
{
	// Make sure we have players to iterate over
	if (players.empty())
		return;

	if (gamestate == GS_INTERMISSION)
	{
		displayplayer_id = consoleplayer_id;
		return;
	}

	if (!validplayer(displayplayer()))
	{
		CL_CheckDisplayPlayer();
		return;
	}

	// set the sentinal iterator to point to displayplayer
	Iterator sentinal = begin;
	while (sentinal != end && sentinal->id != displayplayer_id)
		++sentinal;

	// We can't find the displayplayer.  This is bad.
	if (sentinal == end)
		return;

	// iterate through all of the players until we reach sentinal again
	Iterator it = sentinal;

	do
	{
		// Increment iterator and wrap around if we hit end.
		// The sentinal will stop the lop.
		if (++it == end)
			it = begin;

		player_t& self = consoleplayer();
		player_t& player = *it;

		// spectators only cycle between active players
		if (P_CanSpy(self, player, demoplayback || netdemo.isPlaying() || netdemo.isPaused()))
		{
			displayplayer_id = player.id;
			CL_CheckDisplayPlayer();

			if (demoplayback)
			{
				consoleplayer_id = player.id;
				ST_ForceRefresh();
			}

			return;
		}
	} while (it != sentinal);
}

extern BOOL advancedemo;
QWORD nextstep = 0;
int canceltics = 0;

void CL_StepTics(unsigned int count)
{
	DObject::BeginFrame ();

	// run the realtics tics
	while (count--)
	{
		if (canceltics && canceltics--)
			continue;

		NetUpdate();

		if (advancedemo)
			D_DoAdvanceDemo();

		C_Ticker();
		M_Ticker();
		HU_Ticker();

		if (P_AtInterval(TICRATE))
			CL_PlayerTimes();

		if (sv_gametype == GM_CTF)
			CTF_RunTics ();

		::levelstate.tic();

		Maplist_Runtic();

		R_InterpolationTicker();

		G_Ticker ();
		gametic++;
		if (netdemo.isPlaying() && !netdemo.isPaused())
			netdemo.ticker();
	}

	DObject::EndFrame ();
}

//
// CL_DisplayTics
//
void CL_DisplayTics()
{
	I_GetEvents(true);
	D_Display();
}

//
// CL_RunTics
//
void CL_RunTics()
{
	std::string cmd = I_ConsoleInput();
	if (cmd.length())
		AddCommandString(cmd);

	if (CON.is_open())
	{
		CON.clear();
		if (!CON.eof())
		{
			std::getline(CON, cmd);
			AddCommandString(cmd);
		}
	}

	if (step_mode)
	{
		NetUpdate();

		if (nextstep)
		{
			canceltics = 0;
			CL_StepTics(nextstep);
			nextstep = 0;

			// debugging output
			extern unsigned char prndindex;
			if (!(players.empty()) && players.begin()->mo)
				Printf("level.time %d, prndindex %d, %d %d %d\n",
				       level.time, prndindex, players.begin()->mo->x, players.begin()->mo->y, players.begin()->mo->z);
			else
 				Printf("level.time %d, prndindex %d\n", level.time, prndindex);
		}
	}
	else
	{
		CL_StepTics(1);
	}

	if (!ClientState::get().isConnected())
		CL_RequestConnectInfo();

	// [RH] Use the consoleplayer's camera to update sounds
	S_UpdateSounds(listenplayer().camera);	// move positional sounds
	S_UpdateMusic();	// play another chunk of music

	D_DisplayTicker();
}

/////// CONSOLE COMMANDS ///////

BEGIN_COMMAND (stepmode)
{
	step_mode = !step_mode;
}
END_COMMAND (stepmode)

BEGIN_COMMAND (step)
{
	nextstep = argc > 1 ? atoi(argv[1]) : 1;
}
END_COMMAND (step)

BEGIN_COMMAND (players)
{
	// Gather all ingame players
	std::map<int, std::string> mplayers;
	for (Players::const_iterator it = players.begin();it != players.end();++it) {
		if (it->ingame()) {
			mplayers[it->id] = it->userinfo.netname;
		}
	}

	// Print them, ordered by player id.
	Printf("PLAYERS IN GAME:\n");
	for (std::map<int, std::string>::iterator it = mplayers.begin();it != mplayers.end();++it) {
		Printf("%3d. %s\n", (*it).first, (*it).second.c_str());
	}
	Printf("%d %s\n", mplayers.size(), mplayers.size() == 1 ? "PLAYER" : "PLAYERS");
}
END_COMMAND (players)


BEGIN_COMMAND (playerinfo)
{
	player_t *player = &consoleplayer();

	if(argc > 1)
	{
		player_t &p = idplayer(atoi(argv[1]));

		if (!validplayer(p))
		{
			Printf ("Bad player number\n");
			return;
		}
		else
			player = &p;
	}

	if (!validplayer(*player))
	{
		Printf ("Not a valid player\n");
		return;
	}

	char color[8];
	sprintf(color, "#%02X%02X%02X",
			player->userinfo.color[1], player->userinfo.color[2], player->userinfo.color[3]);

	Printf (PRINT_HIGH, "---------------[player info]----------- \n");
	Printf(PRINT_HIGH, " userinfo.netname - %s \n",		player->userinfo.netname.c_str());

	if (sv_gametype == GM_CTF || sv_gametype == GM_TEAMDM) {
		Printf(PRINT_HIGH, " userinfo.team    - %s \n",
		       GetTeamInfo(player->userinfo.team)->ColorizedTeamName().c_str());
	}
	Printf(PRINT_HIGH, " userinfo.aimdist - %d \n",		player->userinfo.aimdist >> FRACBITS);
	Printf(PRINT_HIGH, " userinfo.color   - %s \n",		color);
	Printf(PRINT_HIGH, " userinfo.gender  - %d \n",		player->userinfo.gender);
	Printf(PRINT_HIGH, " time             - %d \n",		player->GameTime);
	Printf(PRINT_HIGH, " spectator        - %d \n",		player->spectator);
	Printf(PRINT_HIGH, " downloader       - %d \n",		player->playerstate == PST_DOWNLOAD);
	Printf (PRINT_HIGH, "--------------------------------------- \n");
}
END_COMMAND (playerinfo)


BEGIN_COMMAND(kill)
{
	if (sv_allowcheats || G_IsCoopGame())
	{
		MSG_WriteCLC(&write_buffer, odaproto::clc::Kill{});
	}
	else
	{
		Printf("You must run the server with '+set sv_allowcheats 1' or disable "
		       "sv_keepkeys to enable this command.\n");
	}
}
END_COMMAND(kill)


BEGIN_COMMAND (serverinfo)
{
	std::vector<std::string> server_cvars;

    cvar_t *Cvar = GetFirstCvar();
    size_t MaxFieldLength = 0;

    // [Russell] - Find the largest cvar name, used for formatting
    while (Cvar)
	{
        if (Cvar->flags() & CVAR_SERVERINFO)
        {
            size_t FieldLength = strlen(Cvar->name());

            if (FieldLength > MaxFieldLength)
                MaxFieldLength = FieldLength;

			// store this cvar name in our vector to be sorted later
			server_cvars.push_back(Cvar->name());
        }

        Cvar = Cvar->GetNext();
    }

	// sort the list of cvars
	std::sort(server_cvars.begin(), server_cvars.end());

    // Heading
    Printf ("\n%*s - Value\n", MaxFieldLength, "Name");

    // Data
	for (size_t i = 0; i < server_cvars.size(); i++)
	{
		cvar_t *dummy;
		Cvar = cvar_t::FindCVar(server_cvars[i].c_str(), &dummy);

		Printf( "%*s - %s\n",
				MaxFieldLength,
				Cvar->name(),
				Cvar->cstring());
	}

    Printf ("\n");
}
END_COMMAND (serverinfo)


BEGIN_COMMAND(rcon)
{
	if (ClientState::get().isConnected() && argc > 1)
	{
		char command[256];

		strncpy(command, args, ARRAY_LENGTH(command) - 1);
		command[255] = '\0';

		MSG_WriteCLC(&write_buffer, CLC_RCon(command));
	}
}
END_COMMAND(rcon)


BEGIN_COMMAND(rcon_password)
{
	if (ClientState::get().isConnected() && argc > 1)
	{
		MSG_WriteCLC(&write_buffer, CLC_RConPasswordLogin(argv[1], ::digest));
	}
}
END_COMMAND(rcon_password)

BEGIN_COMMAND(rcon_logout)
{
	if (ClientState::get().isConnected())
	{
		MSG_WriteCLC(&write_buffer, CLC_RConPasswordLogout());
	}
}
END_COMMAND(rcon_logout)


BEGIN_COMMAND (playerteam)
{
	if (G_IsTeamGame())
		Printf("Your are in the %s team.\n", V_GetTeamColor(consoleplayer().userinfo.team).c_str());
	else
		Printf("You need to play a team-based gamemode in order to use this command.\n");
}
END_COMMAND (playerteam)

BEGIN_COMMAND (changeteams)
{
	int iTeam = (int)consoleplayer().userinfo.team;
	iTeam = ++iTeam % sv_teamsinplay.asInt();
	cl_team.Set(GetTeamInfo((team_t)iTeam)->ColorStringUpper.c_str());
}
END_COMMAND (changeteams)

BEGIN_COMMAND (spectate)
{
	bool spectator = consoleplayer().spectator;

	if (spectator)
	{
		// reset camera to self
		displayplayer_id = consoleplayer_id;
		CL_CheckDisplayPlayer();
	}

	// Only send message if currently not a spectator, or to remove from play queue
	if (!spectator || consoleplayer().QueuePosition > 0)
	{
		MSG_WriteCLC(&write_buffer, CLC_Spectate(true));
	}
}
END_COMMAND (spectate)

BEGIN_COMMAND(ready)
{
	constexpr std::array<const char*, 1> arr = {"ready"};
	MSG_WriteCLC(&write_buffer, CLC_NetCmd(arr.data(), 1));
}
END_COMMAND(ready)

static void NetCmdHelp()
{
	Printf(PRINT_HIGH,
	       "netcmd - Send an arbitrary string command to a server\n\n"
	       "Common commands:\n"
	       "  ] netcmd help\n"
	       "  Check to see if the server has any server-specific netcmd's.\n\n"
	       "  ] netcmd motd\n"
	       "  Ask the server for the MOTD.\n\n"
	       "  ] netcmd ready\n"
	       "  Set yourself as ready or unready.\n\n"
	       "  ] netcmd vote <\"yes\"|\"no\">\n"
	       "  Vote \"yes\" or \"no\" in an ongoing vote.\n");
}

BEGIN_COMMAND(netcmd)
{
	if (argc < 2)
	{
		NetCmdHelp();
		return;
	}

	MSG_WriteCLC(&write_buffer, CLC_NetCmd(&argv[1], argc - 1));
}
END_COMMAND(netcmd)

BEGIN_COMMAND (join)
{
	//if (P_NumPlayersInGame() >= sv_maxplayers)
	//{
	//	C_MidPrint("The game is currently full", NULL);
	//	return;
	//}

	MSG_WriteCLC(&write_buffer, CLC_Spectate(false));
}
END_COMMAND (join)

BEGIN_COMMAND (flagnext)
{
	if (sv_gametype == GM_CTF && (consoleplayer().spectator || netdemo.isPlaying()))
	{
		for (int i = 0; i < NUMTEAMS; i++)
		{
			byte id = GetTeamInfo((team_t)i)->FlagData.flagger;
			if (id != 0 && displayplayer_id != id)
			{
				displayplayer_id = id;
				CL_CheckDisplayPlayer();
				return;
			}
		}
	}
}
END_COMMAND (flagnext)

BEGIN_COMMAND (spynext)
{
	CL_SpyCycle(players.begin(), players.end());
}
END_COMMAND (spynext)

BEGIN_COMMAND (spyprev)
{
	CL_SpyCycle(players.rbegin(), players.rend());
}
END_COMMAND (spyprev)

BEGIN_COMMAND (spy)
{
	if (argc <= 1) {
		if (spyplayername.length() > 0) {
			Printf(PRINT_HIGH, "Unfollowing player '%s'.\n", spyplayername.c_str());

			// revert to not spying:
			displayplayer_id = consoleplayer_id;
		} else {
			Printf(PRINT_HIGH, "Expecting player name.  Try 'players' to list all player names.\n");
		}

		// clear last player name:
		spyplayername = "";
	} else {
		// remember player name in case of disconnect/reconnect e.g. level change:
		spyplayername = argv[1];

		Printf(PRINT_HIGH, "Following player '%s'. Use 'spy' with no player name to unfollow.\n",
			   spyplayername.c_str());
	}

	CL_CheckDisplayPlayer();
}
END_COMMAND (spy)

void STACK_ARGS call_terms (void);

void CL_QuitCommand()
{
	call_terms();
	exit(EXIT_SUCCESS);
}

BEGIN_COMMAND (quit)
{
	CL_QuitCommand();
}
END_COMMAND (quit)

// An alias for 'quit'
BEGIN_COMMAND (exit)
{
	CL_QuitCommand();
}
END_COMMAND (exit)

//
// NetDemo related functions
//

CVAR_FUNC_IMPL (cl_netdemoname)
{
	// No empty format strings allowed.
	if (strlen(var.cstring()) == 0)
		var.RestoreDefault();
}

//
// CL_GenerateNetDemoFileName
//
//
std::string CL_GenerateNetDemoFileName(const std::string &filename = cl_netdemoname.cstring())
{
	const std::string expanded_filename(M_ExpandTokens(filename));
	std::string newfilename(expanded_filename);
	newfilename = M_GetUserFileName(newfilename.c_str());

	// keep trying to find a filename that doesn't yet exist
	if (!M_FindFreeName(newfilename, "odd"))
	{
		I_Warning("Unable to generate netdemo file name.");
		return std::string();
	}

	return newfilename;
}

void CL_NetDemoPlay(const std::string& filename)
{
	std::string found = M_FindUserFileName(filename, ".odd");
	if (found.empty())
	{
		Printf(PRINT_WARNING, "Could not find demo %s.\n", filename.c_str());
		return;
	}

	netdemo.startPlaying(found);
}

BEGIN_COMMAND(stopnetdemo)
{
	if (netdemo.isRecording())
	{
		netdemo.stopRecording();
	}
	else if (netdemo.isPlaying())
	{
		netdemo.stopPlaying();
	}
}
END_COMMAND(stopnetdemo)

BEGIN_COMMAND(netrecord)
{
	if (netdemo.isRecording())
	{
		Printf(PRINT_HIGH, "Already recording a netdemo.  Please stop recording before "\
				"beginning a new netdemo recording.\n");
		return;
	}

	if (!ClientState::get().isConnected() || simulated_connection)
	{
		Printf(PRINT_HIGH, "You must be connected to a server to record a netdemo.\n");
		return;
	}

	std::string filename;
	if (argc > 1 && strlen(argv[1]) > 0)
		filename = CL_GenerateNetDemoFileName(argv[1]);
	else
		filename = CL_GenerateNetDemoFileName();

	if (netdemo.startRecording(filename))
		netdemo.writeMapChange();
}
END_COMMAND(netrecord)

BEGIN_COMMAND(netpause)
{
	if (netdemo.isPaused())
	{
		netdemo.resume();
		paused = false;
		Printf(PRINT_HIGH, "Demo resumed.\n");
	}
	else if (netdemo.isPlaying())
	{
		netdemo.pause();
		paused = true;
		Printf(PRINT_HIGH, "Demo paused.\n");
	}
}
END_COMMAND(netpause)

BEGIN_COMMAND(netplay)
{
	if(argc <= 1)
	{
		Printf(PRINT_HIGH, "Usage: netplay <demoname>\n");
		return;
	}

	if (!ClientState::get().isConnected())
	{
 		G_CheckDemoStatus();	// cleans up vanilla demo or single player game
	}

	CL_QuitNetGame(NQ_SILENT);

	std::string filename = argv[1];
	CL_NetDemoPlay(filename);
}
END_COMMAND(netplay)

BEGIN_COMMAND(netdemostats)
{
	if (!netdemo.isPlaying() && !netdemo.isPaused())
		return;

	std::vector<int> maptimes = netdemo.getMapChangeTimes();
	int curtime = netdemo.calculateTimeElapsed();
	int totaltime = netdemo.calculateTotalTime();

	Printf(PRINT_HIGH, "\n%s\n", netdemo.getFileName().c_str());
	Printf(PRINT_HIGH, "============================================\n");
	Printf(PRINT_HIGH, "Total time: %i seconds\n", totaltime);
	Printf(PRINT_HIGH, "Current position: %i seconds (%i%%)\n",
		curtime, curtime * 100 / totaltime);
	Printf(PRINT_HIGH, "Number of maps: %i\n", maptimes.size());
	for (size_t i = 0; i < maptimes.size(); i++)
	{
		Printf(PRINT_HIGH, "> %02i Starting time: %i seconds\n",
			i + 1, maptimes[i]);
	}
}
END_COMMAND(netdemostats)

BEGIN_COMMAND(netff)
{
	if (netdemo.isPlaying())
		netdemo.nextSnapshot();
	else if (netdemo.isPaused());
		netdemo.nextTic();
}
END_COMMAND(netff)

BEGIN_COMMAND(netrew)
{
	if (netdemo.isPlaying())
		netdemo.prevSnapshot();
}
END_COMMAND(netrew)

BEGIN_COMMAND(netnextmap)
{
	if (netdemo.isPlaying())
		netdemo.nextMap();
}
END_COMMAND(netnextmap)

BEGIN_COMMAND(netprevmap)
{
	if (netdemo.isPlaying())
		netdemo.prevMap();
}
END_COMMAND(netprevmap)

//
// CL_MoveThing
//
void CL_MoveThing(AActor *mobj, fixed_t x, fixed_t y, fixed_t z)
{
	if (!mobj)
		return;

	// [SL] 2011-11-06 - Return before setting the thing's floorz value if
	// the thing hasn't moved.  This ensures the floorz value is correct for
	// things that have spawned (too close to a ledge) but have not yet moved.
	if (mobj->x == x && mobj->y == y && mobj->z == z)
		return;

	P_CheckPosition(mobj, x, y);
	mobj->UnlinkFromWorld ();

	mobj->x = x;
	mobj->y = y;
	mobj->z = z;
	mobj->floorz = tmfloorz;
	mobj->ceilingz = tmceilingz;
	mobj->dropoffz = tmdropoffz;
	mobj->floorsector = tmfloorsector;
	mobj->LinkToWorld ();
}

//
// CL_SendUserInfo
//
void CL_SendUserInfo(void)
{
	D_SetupUserInfo();

	MSG_WriteCLC(&write_buffer, CLC_UserInfo(consoleplayer().userinfo));

	CL_RebuildAllPlayerTranslations();	// Refresh Player Translations AFTER sending the new status to the server.
}

//
// CL_FindPlayer
//
player_t &CL_FindPlayer(size_t id)
{
	player_t *p = &idplayer(id);

	// Totally new player?
	if(!validplayer(*p))
	{
		if (players.size() >= MAXPLAYERS)
			return *p;

		players.emplace_back(player_t{});

		p = &players.back();
		p->id = id;
	}

	return *p;
}

/**
 * @brief Update a player's spectate setting and do any necessary busywork for it.
 * 
 * @param player Plyaer to update.
 * @param spectate New spectate setting.
*/
void CL_SpectatePlayer(player_t& player, bool spectate)
{
	bool wasalive = !player.spectator && player.mo && player.mo->health > 0;
	bool wasspectator = player.spectator;
	player.spectator = spectate;

	if (player.spectator && wasalive)
		P_DisconnectEffect(player.mo);
	if (player.spectator && player.mo && !wasspectator)
		P_PlayerLeavesGame(&player);

	// [tm512 2014/04/11] Do as the server does when unspectating a player.
	// If the player has a "valid" mo upon going to PST_LIVE, any enemies
	// that are still targeting the spectating player will cause a stack
	// overflow in P_SetMobjState.

	if (!player.spectator && !wasalive)
	{
		if (player.mo)
			P_KillMobj(NULL, player.mo, NULL, true);

		player.playerstate = PST_REBORN;
	}

	if (&player == &consoleplayer())
	{
		R_ForceViewWindowResize();		// toggline spectator mode affects status bar visibility

		if (player.spectator)
		{
			player.playerstate = PST_LIVE;				// Resurrect dead spectators
			player.cheats |= CF_FLY;					// Make players fly by default
			player.deltaviewheight = 1000 << FRACBITS;	// GhostlyDeath -- Sometimes if the player spectates while he is falling down he squats

			movingsectors.clear(); // Clear all moving sectors, otherwise client side prediction will not move active sectors
		}
		else
		{
			displayplayer_id = consoleplayer_id; // get out of spynext
			player.cheats &= ~CF_FLY;	// remove flying ability
		}

		ClientReplay::getInstance().reset();

		CL_RebuildAllPlayerTranslations();
	}
	else
	{
		R_BuildPlayerTranslation(player.id, CL_GetPlayerColor(&player));
	}

	P_ClearPlayerPowerups(player);	// Remove all current powerups

	// GhostlyDeath -- If the player matches our display player...
	CL_CheckDisplayPlayer();
}

//
// CL_InitNetwork
//
void CL_InitNetwork (void)
{
    const char *v = Args.CheckValue ("-port");
    if (v)
    {
		localport = atoi (v);
		Printf (PRINT_HIGH, "using alternate port %i\n", localport);
    }
    else
		localport = CLIENTPORT;

    // set up a socket and net_message buffer
    InitNetCommon();

    SZ_Clear(&write_buffer);

	size_t ParamIndex = Args.CheckParm("-connect");
	if (ParamIndex)
	{
		const char* address = Args.GetArg(ParamIndex + 1);

		if (address && address[0] != '-' && address[0] != '+')
		{
			bool ok = false;
			const char* password = Args.GetArg(ParamIndex + 2);
			if (password && password[0] != '-' && password[0] != '+')
			{
				ok = ClientState::get().startConnect(address, password);
			}
			else
			{
				ok = ClientState::get().startConnect(address);
			}

			if (!ok)
			{
				PrintFmt("Could not resolve host \"{}\"\n", address);
				return;
			}

			::gamestate = GS_CONNECTING;
		}
	}
}

//
// CL_PlayerJustTeleported
//
// Returns true if we have received a svc_activateline message from the server
// involving this player and teleportation
//
bool CL_PlayerJustTeleported(player_t *player)
{
	if (player && teleported_players.find(player->id) != teleported_players.end())
		return true;

	return false;
}

//
// CL_ClearPlayerJustTeleported
//
void CL_ClearPlayerJustTeleported(player_t *player)
{
	if (player)
		teleported_players.erase(player->id);
}

ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, BOOL dropped);

//
// CL_ClearSectorSnapshots
//
// Removes all sector snapshots at the start of a map, etc
//
void CL_ClearSectorSnapshots()
{
	sector_snaps.clear();
}

// Decompress the packet sequence
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
void CL_Decompress()
{
	if(!MSG_BytesLeft())
		return;

	MSG_DecompressMinilzo();
}

/**
 * @brief Read the header of the packet and prepare the rest of it for reading.
 * 
 * @return False if the packet was scuttled, otherwise true.
 */
bool CL_ReadPacketHeader()
{
	// Packet sequence number.
	const uint32_t packetID = MSG_ReadLong();

	// Acknowledge packet - for next clc_ack.
	ClientState::get().ack(packetID);

	// Flag bits.
	byte flags = MSG_ReadByte();
	if (flags & SVF_UNUSED_MASK)
	{
		Printf(PRINT_WARNING, "Protocol flag bits (%u) were not understood.", flags);
		CL_QuitNetGame(NQ_PROTO);
		return false;
	}
	else if (flags & SVF_COMPRESSED)
	{
		CL_Decompress();
	}

	netgraph.addPacketIn();
	return true;
}

void CL_Clear()
{
	size_t left = MSG_BytesLeft();
	MSG_ReadChunk(left);
}

//
// CL_ReadAndParseMessages
//
bool CL_ReadAndParseMessages()
{
	if (!CL_ReadMessages())
		return false;

	if (!CL_ParseMessages())
		return false;

	return true;
}

void CL_SaveCmd(void)
{
	NetCommand *netcmd = &localcmds[gametic % MAXSAVETICS];
	netcmd->fromPlayer(&consoleplayer());
	netcmd->setTic(gametic);
	netcmd->setWorldIndex(world_index);
}

extern int outrate;

//
// CL_SendCmd
//
void CL_SendCmd(void)
{
	player_t *p = &consoleplayer();

	if (netdemo.isPlaying())	// we're not really connected to a server
		return;

	if (!p->mo || gametic < 1 )
		return;

	// Resolve acks first.
	// [LM] Add this to the low-level protocol, not as a message.
	MSG_WriteCLC(&write_buffer, CLC_Ack(ClientState::get().getRecentAck(),
	                                    ClientState::get().getAckBits()));

	// GhostlyDeath -- If we are spectating, tell the server of our new position
	if (p->spectator)
	{
		MSG_WriteCLC(&write_buffer, CLC_SpectateUpdate(p->mo));
	}

	MSG_WriteCLC(&::write_buffer, CLC_Move(::gametic, ::localcmds, MAXSAVETICS));

	int bytesWritten = NET_SendPacket(write_buffer, ClientState::get().getAddress());
	netgraph.addTrafficOut(bytesWritten);

	outrate += write_buffer.size();
	SZ_Clear(&write_buffer);
}

void CL_KeepAlive()
{
	// Update the server on acked packets.
	MSG_WriteCLC(&write_buffer, CLC_Ack(ClientState::get().getRecentAck(),
	                                    ClientState::get().getAckBits()));

	// Send just the ack.
	int bytesWritten = NET_SendPacket(write_buffer, ClientState::get().getAddress());
	netgraph.addTrafficOut(bytesWritten);

	outrate += write_buffer.size();
	SZ_Clear(&write_buffer);
}

//
// CL_PlayerTimes
//
void CL_PlayerTimes()
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame())
			it->GameTime++;
	}
}

//
// CL_SendCheat
//
void CL_SendCheat(int cheats)
{
	MSG_WriteCLC(&write_buffer, CLC_CheatNumber(cheats));
}

//
// CL_SendCheat
//
void CL_SendGiveCheat(const char* item)
{
	MSG_WriteCLC(&write_buffer, CLC_CheatGiveTo(item));
}


void PickupMessage (AActor *toucher, const char *message)
{
	// Some maps have multiple items stacked on top of each other.
	// It looks odd to display pickup messages for all of them.
	static int lastmessagetic;
	static const char *lastmessage = NULL;

	if (toucher == consoleplayer().camera
		&& (lastmessagetic != gametic || lastmessage != message))
	{
		lastmessagetic = gametic;
		lastmessage = message;
		Printf (PRINT_PICKUP, "%s\n", message);
	}
}

//
// void WeaponPickupMessage (weapontype_t &Weapon)
//
// This is used for displaying weaponstay messages, it is inevitably a hack
// because weaponstay is a hack
void WeaponPickupMessage (AActor *toucher, weapontype_t &Weapon)
{
    switch (Weapon)
    {
        case wp_shotgun:
        {
            PickupMessage(toucher, GStrings(GOTSHOTGUN));
        }
        break;

        case wp_chaingun:
        {
            PickupMessage(toucher, GStrings(GOTCHAINGUN));
        }
        break;

        case wp_missile:
        {
            PickupMessage(toucher, GStrings(GOTLAUNCHER));
        }
        break;

        case wp_plasma:
        {
            PickupMessage(toucher, GStrings(GOTPLASMA));
        }
        break;

        case wp_bfg:
        {
            PickupMessage(toucher, GStrings(GOTBFG9000));
        }
        break;

        case wp_chainsaw:
        {
            PickupMessage(toucher, GStrings(GOTCHAINSAW));
        }
        break;

        case wp_supershotgun:
        {
            PickupMessage(toucher, GStrings(GOTSHOTGUN2));
        }
        break;

        default:
        break;
    }
}

void CL_RemoveCompletedMovingSectors()
{
	std::map<unsigned short, SectorSnapshotManager>::iterator itr;
	itr = sector_snaps.begin();

	while (itr != sector_snaps.end())
	{
		SectorSnapshotManager *mgr = &(itr->second);
		int time = mgr->getMostRecentTime();

		// are all the snapshots in the container invalid or too old?
		if (world_index - time > NUM_SNAPSHOTS || mgr->empty())
			sector_snaps.erase(itr++);
		else
			++itr;
	}
}

CVAR_FUNC_IMPL (cl_interp)
{
	// Resync the world index since the sync offset has changed
	CL_ResyncWorldIndex();
	netgraph.setInterpolation(var);
}

//
// CL_SimulateSectors
//
// Iterates through the list of moving sector snapshot containers
// and loads the world_index snapshot for each sector that is not
// currently being predicted.  Predicted sectors are handled elsewhere.
//
void CL_SimulateSectors()
{
	// Get rid of snapshots for sectors that are done moving
	CL_RemoveCompletedMovingSectors();

	// Move sectors
	std::map<unsigned short, SectorSnapshotManager>::iterator itr;
	for (itr = sector_snaps.begin(); itr != sector_snaps.end(); ++itr)
	{
		unsigned short sectornum = itr->first;
		if (sectornum >= numsectors)
			continue;

		sector_t *sector = &sectors[sectornum];

		// will this sector be handled when predicting sectors?
		if (cl_predictsectors && CL_SectorIsPredicting(sector))
			continue;

		// Fetch the snapshot for this world_index and run the sector's
		// thinkers to play any sector sounds
		SectorSnapshot snap = itr->second.getSnapshot(world_index);
		if (snap.isValid())
		{
			snap.toSector(sector);

			if (sector->ceilingdata)
				sector->ceilingdata->RunThink();
			if (sector->floordata && sector->ceilingdata != sector->floordata)
				sector->floordata->RunThink();

			snap.toSector(sector);
		}
	}
}

//
// CL_SimulatePlayers()
//
// Iterates through the players vector and loads the world_index snapshot
// for all players except consoleplayer, as this is handled by the prediction
// functions.
//
void CL_SimulatePlayers()
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		player_t *player = &*it;
		if (!player || !player->mo || player->spectator)
			continue;

		// Consoleplayer is handled in CL_PredictWorld
		if (player->id == consoleplayer_id)
			continue;

		PlayerSnapshot snap = player->snapshots.getSnapshot(world_index);
		if (snap.isValid())
		{
			// Examine the old position.  If it doesn't match the snapshot for the
			// previous world_index, then old position was probably extrapolated
			// and should be smoothly moved towards the corrected position instead
			// of snapping to it.

			if (snap.isContinuous())
			{
				// [SL] Save the position prior to the new update so it can be
				// used for rendering interpolation
				player->mo->prevx = player->mo->x;
				player->mo->prevy = player->mo->y;
				player->mo->prevz = player->mo->z;
				player->mo->prevangle = player->mo->angle;
				player->mo->prevpitch = player->mo->pitch;

				PlayerSnapshot prevsnap = player->snapshots.getSnapshot(world_index - 1);

				v3fixed_t offset;
				M_SetVec3Fixed(&offset, prevsnap.getX() - player->mo->x,
										prevsnap.getY() - player->mo->y,
										prevsnap.getZ() - player->mo->z);

				fixed_t dist = M_LengthVec3Fixed(&offset);
				if (dist > 2 * FRACUNIT)
				{
					#ifdef _SNAPSHOT_DEBUG_
					Printf(PRINT_HIGH, "Snapshot %i, Correcting extrapolation error of %i\n",
							world_index, dist >> FRACBITS);
					#endif	// _SNAPSHOT_DEBUG_

					static const fixed_t correction_amount = FRACUNIT * 0.80f;
					M_ScaleVec3Fixed(&offset, &offset, correction_amount);

					// Apply a smoothing offset to the current snapshot
					snap.setX(snap.getX() - offset.x);
					snap.setY(snap.getY() - offset.y);
					snap.setZ(snap.getZ() - offset.z);
				}
			}

			int oldframe = player->mo->frame;
			snap.toPlayer(player);

			if (player->playerstate != PST_LIVE)
				player->mo->frame = oldframe;

			if (!snap.isContinuous())
			{
				// [SL] Save the position after to the new update so this position
				// won't be interpolated.
				player->mo->prevx = player->mo->x;
				player->mo->prevy = player->mo->y;
				player->mo->prevz = player->mo->z;
				player->mo->prevangle = player->mo->angle;
				player->mo->prevpitch = player->mo->pitch;
			}
		}
	}
}


//
// CL_SimulateWorld
//
// Maintains synchronization with the server by manipulating world_index.
// Loads snapshots for all moving sectors and players for the server gametic
// denoted by world_index.
//
void CL_SimulateWorld()
{
	if (gamestate != GS_LEVEL || netdemo.isPaused())
		return;

	// if the world_index falls outside this range, resync it
	static const int MAX_BEHIND = 16;
	static const int MAX_AHEAD = 16;

	int lower_sync_limit = CL_CalculateWorldIndexSync() - MAX_BEHIND;
	int upper_sync_limit = CL_CalculateWorldIndexSync() + MAX_AHEAD;

	// Was the displayplayer just teleported?
	bool continuous = displayplayer().snapshots.getSnapshot(world_index).isContinuous();

	// Reset the synchronization with the server if needed
	if (world_index <= 0 || !continuous ||
		world_index > upper_sync_limit || world_index < lower_sync_limit)
	{
		#ifdef _WORLD_INDEX_DEBUG_
		std::string reason;
		if (!continuous)
			reason = "discontinuous";
		else if (world_index > upper_sync_limit)
			reason = "too far ahead of server";
		else if (world_index < lower_sync_limit)
			reason = "too far behind server";
		else
			reason = "invalid world_index";

		Printf(PRINT_HIGH, "Gametic %i, world_index %i, Resynching world index (%s).\n",
			gametic, world_index, reason.c_str());
		#endif // _WORLD_INDEX_DEBUG_

		CL_ResyncWorldIndex();
	}

	// Not using interpolation?  Use the last update always
	if (!cl_interp)
		world_index = last_svgametic;

	#ifdef _WORLD_INDEX_DEBUG_
	Printf(PRINT_HIGH, "Gametic %i, simulating world_index %i\n",
		gametic, world_index);
	#endif // _WORLD_INDEX_DEBUG_

	// [SL] 2012-03-29 - Add sync information to the netgraph
	netgraph.setWorldIndexSync(world_index - (last_svgametic - cl_interp));

	CL_SimulateSectors();
	CL_SimulatePlayers();

	// [SL] 2012-03-17 - Try to maintain sync with the server by gradually
	// slowing down or speeding up world_index
	int drift_correction = CL_CalculateWorldIndexDriftCorrection();

	#ifdef _WORLD_INDEX_DEBUG_
	if (drift_correction != 0)
		Printf(PRINT_HIGH, "Gametic %i, increasing world index by %i.\n",
				gametic, drift_correction);
	#endif // _WORLD_INDEX_DEBUG_

	world_index = world_index + 1 + drift_correction;
}

void OnChangedSwitchTexture (line_t *line, int useAgain) {}
void SV_OnActivatedLine(line_t* line, AActor* mo, const int side,
                        const LineActivationType activationType, const bool bossaction)
{
}

VERSION_CONTROL (cl_main_cpp, "$Id$")
