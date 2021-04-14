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


#include "doomtype.h"
#include "doomstat.h"
#include "gstrings.h"
#include "d_player.h"
#include "g_game.h"
#include "d_net.h"
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

#include <bitset>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

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

buf_t     net_buffer(MAX_UDP_PACKET);

bool      noservermsgs;
int       last_received;

// [SL] 2012-03-17 - world_index is the gametic on the server that the client
// is currently simulating.  world_index_accum is a continuous accumulator that
// is used to advance world_index if appropriate.
int       world_index = 0;
float     world_index_accum = 0.0f;

int       last_svgametic = 0;
int       last_player_update = 0;

bool		recv_full_update = false;

std::string connectpasshash = "";

BOOL      connected;
netadr_t  serveraddr; // address of a server
netadr_t  lastconaddr;

int       packetseq[256];
byte      packetnum;

// denis - unique session key provided by the server
std::string digest;

// denis - clientside compressor, used for decompression
huffman_client compressor;

std::string server_host = "";	// hostname of server

// [SL] 2011-06-27 - Class to record and playback network recordings
NetDemo netdemo;
// [SL] 2011-07-06 - not really connected (playing back a netdemo)
bool simulated_connection = false;
bool forcenetdemosplit = false;		// need to split demo due to svc_reconnect

NetCommand localcmds[MAXSAVETICS];

extern NetGraph netgraph;

// [SL] 2012-03-07 - Players that were teleported during the current gametic
std::set<byte> teleported_players;

// [SL] 2012-04-06 - moving sector snapshots received from the server
std::map<unsigned short, SectorSnapshotManager> sector_snaps;

EXTERN_CVAR (sv_weaponstay)
EXTERN_CVAR (sv_teamsinplay)

EXTERN_CVAR (sv_downloadsites)
EXTERN_CVAR (cl_downloadsites)

EXTERN_CVAR (cl_predictsectors)

EXTERN_CVAR (mute_spectators)
EXTERN_CVAR (mute_enemies)

EXTERN_CVAR (cl_autoaim)

EXTERN_CVAR (cl_interp)
EXTERN_CVAR (cl_serverdownload)
EXTERN_CVAR (cl_forcedownload)

// [SL] Force enemies to have the specified color
EXTERN_CVAR (r_forceenemycolor)
EXTERN_CVAR (r_forceteamcolor)

EXTERN_CVAR (hud_revealsecrets)

static argb_t enemycolor, teamcolor;

void P_PlayerLeavesGame(player_s* player);
void P_DestroyButtonThinkers();

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

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
		R_BuildPlayerTranslation(it->id, CL_GetPlayerColor(&*it));
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
EXTERN_CVAR (cl_connectalert)
EXTERN_CVAR (cl_disconnectalert)
EXTERN_CVAR (waddirs)
EXTERN_CVAR (cl_autorecord)
EXTERN_CVAR (cl_autorecord_coop)
EXTERN_CVAR (cl_autorecord_deathmatch)
EXTERN_CVAR (cl_autorecord_duel)
EXTERN_CVAR (cl_autorecord_teamdm)
EXTERN_CVAR (cl_autorecord_ctf)
EXTERN_CVAR (cl_splitnetdemos)

void CL_PlayerTimes (void);
void CL_GetServerSettings(void);
void CL_RequestDownload(std::string filename, std::string filehash = "");
void CL_TryToConnect(DWORD server_token);
void CL_Decompress(int sequence);

void CL_LocalDemoTic(void);
void CL_NetDemoStop(void);
bool M_FindFreeName(std::string &filename, const std::string &extension);

void CL_SimulateWorld();

//	[Toke - CTF]
void CalcTeamFrags (void);

// some doom functions (not csDoom)
void D_Display(void);
void D_DoAdvanceDemo(void);
void M_Ticker(void);

void R_InterpolationTicker();

size_t P_NumPlayersInGame();
void G_PlayerReborn (player_t &player);
void CL_SpawnPlayer ();
void P_KillMobj (AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void P_SetPsprite (player_t *player, int position, statenum_t stnum);
void P_ExplodeMissile (AActor* mo);
void P_CalcHeight (player_t *player);
bool P_CheckMissileSpawn (AActor* th);
void CL_SetMobjSpeedAndAngle(void);

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
static void CL_ResyncWorldIndex()
{
	world_index = CL_CalculateWorldIndexSync();
	world_index_accum = 0.0f;
}

void Host_EndGame(const char *msg)
{
    Printf("%s", msg);
	CL_QuitNetGame();
}

void CL_QuitNetGame(void)
{
	if(connected)
	{
		SZ_Clear(&net_buffer);
		MSG_WriteMarker(&net_buffer, clc_disconnect);
		NET_SendPacket(net_buffer, serveraddr);
		SZ_Clear(&net_buffer);
		sv_gametype = GM_COOP;
	}

	if (paused)
	{
		paused = false;
		S_ResumeSound ();
	}

	memset (&serveraddr, 0, sizeof(serveraddr));
	connected = false;
	gameaction = ga_fullconsole;
	noservermsgs = false;
	AM_Stop();

	serverside = clientside = true;
	network_game = false;
	simulated_connection = false;	// Ch0wW : don't block people connect to a server after playing a demo

	sv_freelook = 1;
	sv_allowjump = 1;
	sv_allowexit = 1;
	sv_allowredscreen = 1;

	mute_spectators = 0.f;
	mute_enemies = 0.f;

	P_ClearAllNetIds();

	{
		// [jsd] unlink player pointers from AActors; solves crash in R_ProjectSprites after a svc_disconnect message.
		for (Players::iterator it = players.begin(); it != players.end(); it++) {
			player_s &player = *it;
			if (player.mo) {
				player.mo->player = NULL;
			}
		}

		players.clear();
	}

	recv_full_update = false;

	if (netdemo.isRecording())
		netdemo.stopRecording();

	if (netdemo.isPlaying())
		netdemo.stopPlaying();

	if (demorecording)
		G_CleanupDemo();	// Cleanup in case of a vanilla demo

	demoplayback = false;

	// Reset the palette to default
	V_ResetPalette();

	cvar_t::C_RestoreCVars();
}


void CL_Reconnect(void)
{
	recv_full_update = false;

	if (netdemo.isRecording())
		forcenetdemosplit = true;

	if (connected)
	{
		MSG_WriteMarker(&net_buffer, clc_disconnect);
		NET_SendPacket(net_buffer, serveraddr);
		SZ_Clear(&net_buffer);
		connected = false;
		gameaction = ga_fullconsole;

		P_ClearAllNetIds();
	}
	else if (lastconaddr.ip[0])
	{
		serveraddr = lastconaddr;
	}

	simulated_connection = false;	// Ch0wW : don't block people connect to a server after playing a demo
	connecttimeout = 0;
}

std::string spyplayername;
void CL_CheckDisplayPlayer(void);

//
// CL_ConnectClient
//
void CL_ConnectClient(void)
{
	player_t &player = idplayer(MSG_ReadByte());

	CL_CheckDisplayPlayer();

	if (!cl_connectalert)
		return;

	// GhostlyDeath <August 1, 2008> -- Play connect sound
	if (&player == &consoleplayer())
		return;

	S_Sound (CHAN_INTERFACE, "misc/pljoin", 1, ATTN_NONE);
}

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
		MSG_WriteMarker(&net_buffer, clc_spy);
		MSG_WriteByte(&net_buffer, newid);
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

//
// CL_DisconnectClient
//
void CL_DisconnectClient(void)
{
	player_t &player = idplayer(MSG_ReadByte());
	if (players.empty() || !validplayer(player))
		return;

	if (player.mo)
	{
		P_DisconnectEffect(player.mo);

		// [AM] Destroying the player mobj is not our responsibility.  However, we do want to
		//      make sure that the mobj->player doesn't point to an invalid player.
		player.mo->player = NULL;
	}

	// Remove the player from the players list.
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->id == player.id)
		{
			if (cl_disconnectalert && &player != &consoleplayer())
				S_Sound(CHAN_INTERFACE, "misc/plpart", 1, ATTN_NONE);
			if (!it->spectator)
				P_PlayerLeavesGame(&(*it));
			players.erase(it);
			break;
		}
	}

	// if this was our displayplayer, update camera
	CL_CheckDisplayPlayer();
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

	if (!connected)
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

BEGIN_COMMAND (connect)
{
	if (argc == 1)
	{
	    Printf("Usage: connect ip[:port] [password]\n");
	    Printf("\n");
	    Printf("Connect to a server, with optional port number");
	    Printf(" and/or password\n");
	    Printf("eg: connect 127.0.0.1\n");
	    Printf("eg: connect 192.168.0.1:12345 secretpass\n");

	    return;
	}

	simulated_connection = false;	// Ch0wW : don't block people connect to a server after playing a demo

	C_FullConsole();
	gamestate = GS_CONNECTING;

	CL_QuitNetGame();

	if (argc > 1)
	{
		std::string target = argv[1];

        // [Russell] - Passworded servers
        if(argc > 2)
        {
            connectpasshash = MD5SUM(argv[2]);
        }
        else
        {
            connectpasshash = "";
        }

		if(NET_StringToAdr (target.c_str(), &serveraddr))
		{
			if (!serveraddr.port)
				I_SetPort(serveraddr, SERVERPORT);

			lastconaddr = serveraddr;
		}
		else
		{
			Printf("Could not resolve host %s\n", target.c_str());
			memset(&serveraddr, 0, sizeof(serveraddr));
		}
	}

	connecttimeout = 0;
}
END_COMMAND (connect)


BEGIN_COMMAND (disconnect)
{
	CL_QuitNetGame();
}
END_COMMAND (disconnect)


BEGIN_COMMAND (reconnect)
{
	CL_Reconnect();
}
END_COMMAND (reconnect)

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


BEGIN_COMMAND (kill)
{
    if (sv_allowcheats || sv_gametype == GM_COOP)
        MSG_WriteMarker(&net_buffer, clc_kill);
    else
        Printf ("You must run the server with '+set sv_allowcheats 1' or disable sv_keepkeys to enable this command.\n");
}
END_COMMAND (kill)


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


BEGIN_COMMAND (rcon)
{
	if (connected && argc > 1)
	{
		char  command[256];

		strncpy(command, args, ARRAY_LENGTH(command) - 1);
		command[255] = '\0';		

		MSG_WriteMarker(&net_buffer, clc_rcon);
		MSG_WriteString(&net_buffer, command);
	}
}
END_COMMAND (rcon)


BEGIN_COMMAND (rcon_password)
{
	if (connected && argc > 1)
	{
		bool login = true;

		MSG_WriteMarker(&net_buffer, clc_rcon_password);
		MSG_WriteByte(&net_buffer, login);

		std::string password = argv[1];
		MSG_WriteString(&net_buffer, MD5SUM(password + digest).c_str());
	}
}
END_COMMAND (rcon_password)

BEGIN_COMMAND (rcon_logout)
{
	if (connected)
	{
		bool login = false;

		MSG_WriteMarker(&net_buffer, clc_rcon_password);
		MSG_WriteByte(&net_buffer, login);
		MSG_WriteString(&net_buffer, "");
	}
}
END_COMMAND (rcon_logout)


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
		MSG_WriteMarker(&net_buffer, clc_spectate);
		MSG_WriteByte(&net_buffer, true);
	}
}
END_COMMAND (spectate)

BEGIN_COMMAND(ready)
{
	MSG_WriteMarker(&net_buffer, clc_netcmd);
	MSG_WriteString(&net_buffer, "ready");
	MSG_WriteByte(&net_buffer, 0);
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

	MSG_WriteMarker(&net_buffer, clc_netcmd);
	MSG_WriteString(&net_buffer, argv[1]);

	// Pass additional arguments as separate strings.  Avoids argument
	// parsing at the opposite end.
	byte netargc = MIN<size_t>(argc - 2, 0xFF);
	MSG_WriteByte(&net_buffer, netargc);
	for (size_t i = 0; i < netargc; i++)
	{
		MSG_WriteString(&net_buffer, argv[i + 2]);
	}
}
END_COMMAND(netcmd)

BEGIN_COMMAND (join)
{
	//if (P_NumPlayersInGame() >= sv_maxplayers)
	//{
	//	C_MidPrint("The game is currently full", NULL);
	//	return;
	//}

	MSG_WriteMarker(&net_buffer, clc_spectate);
	MSG_WriteByte(&net_buffer, false);
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

void CL_NetDemoStop()
{
	netdemo.stopPlaying();
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

	if (!connected || simulated_connection)
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

	if (!connected)
	{
 		G_CheckDemoStatus();	// cleans up vanilla demo or single player game
	}

	CL_QuitNetGame();
	connected = false;

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

void CL_NetDemoLoadSnap()
{
	AddCommandString("netprevmap");
}

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
	UserInfo* coninfo = &consoleplayer().userinfo;
	D_SetupUserInfo();

	MSG_WriteMarker	(&net_buffer, clc_userinfo);
	MSG_WriteString	(&net_buffer, coninfo->netname.c_str());
	MSG_WriteByte	(&net_buffer, coninfo->team); // [Toke]
	MSG_WriteLong	(&net_buffer, coninfo->gender);

	for (int i = 3; i >= 0; i--)
		MSG_WriteByte(&net_buffer, coninfo->color[i]);

	// [SL] place holder for deprecated skins
	MSG_WriteString	(&net_buffer, "");

	MSG_WriteLong	(&net_buffer, coninfo->aimdist);
	MSG_WriteBool	(&net_buffer, true);	// [SL] deprecated "cl_unlag" CVAR
	MSG_WriteBool	(&net_buffer, coninfo->predict_weapons);
	MSG_WriteByte	(&net_buffer, (char)coninfo->switchweapon);
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		MSG_WriteByte (&net_buffer, coninfo->weapon_prefs[i]);
	}

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

		players.push_back(player_s());

		p = &players.back();
		p->id = id;
	}

	return *p;
}

//
// CL_SetupUserInfo
//
void CL_SetupUserInfo(void)
{
	byte who = MSG_ReadByte();
	player_t *p = &CL_FindPlayer(who);

	p->userinfo.netname = MSG_ReadString();
	p->userinfo.team	= (team_t)MSG_ReadByte();
	p->userinfo.gender	= (gender_t)MSG_ReadLong();

	for (int i = 3; i >= 0; i--)
		p->userinfo.color[i] = MSG_ReadByte();

	// [SL] place holder for deprecated skins
	MSG_ReadString();

	p->GameTime			= MSG_ReadShort();

	if(p->userinfo.gender >= NUMGENDER)
		p->userinfo.gender = GENDER_NEUTER;

	R_BuildPlayerTranslation(p->id, CL_GetPlayerColor(p));

	// [SL] 2012-04-30 - Were we looking through a teammate's POV who changed
	// to the other team?
	// [SL] 2012-05-24 - Were we spectating a teammate before we changed teams?
	CL_CheckDisplayPlayer();
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

/**
 * @brief Updates less-vital members of a player struct.
 */
void CL_PlayerMembers()
{
	player_t& p = CL_FindPlayer(MSG_ReadByte());
	byte flags = MSG_ReadByte();

	if (flags & SVC_PM_SPECTATOR)
		CL_SpectatePlayer(p, MSG_ReadBool());

	if (flags & SVC_PM_READY)
		p.ready = MSG_ReadBool();

	if (flags & SVC_PM_LIVES)
		p.lives = MSG_ReadVarint();

	if (flags & SVC_PM_SCORE)
	{
		p.roundwins = MSG_ReadVarint();
		p.points = MSG_ReadVarint();
		p.fragcount = MSG_ReadVarint();
		p.deathcount = MSG_ReadVarint();
		p.killcount = MSG_ReadVarint();
		p.secretcount = MSG_ReadVarint();
		p.totalpoints = MSG_ReadVarint();
		p.totaldeaths = MSG_ReadVarint();
	}
}

//
// [deathz0r] Receive team frags/captures
//
void CL_TeamMembers()
{
	team_t team = static_cast<team_t>(MSG_ReadVarint());
	int points = MSG_ReadVarint();
	int roundWins = MSG_ReadVarint();

	// Ensure our team is valid.
	TeamInfo* info = GetTeamInfo(team);
	if (info->Team >= NUMTEAMS)
		return;

	info->Points = points;
	info->RoundWins = roundWins;
}

//
// CL_MoveMobj
//
void CL_MoveMobj(void)
{
	AActor  *mo;
	uint32_t netid;
	fixed_t  x, y, z;

	netid = MSG_ReadUnVarint();
	mo = P_FindThingById (netid);

	byte rndindex = MSG_ReadByte();
	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();

	if (!mo)
		return;

	if (mo->player)
	{
		// [SL] 2013-07-21 - Save the position information to a snapshot
		int snaptime = last_svgametic;
		PlayerSnapshot newsnap(snaptime);
		newsnap.setAuthoritative(true);

		newsnap.setX(x);
		newsnap.setY(y);
		newsnap.setZ(z);

		mo->player->snapshots.addSnapshot(newsnap);
	}
	else
	{
		CL_MoveThing (mo, x, y, z);
		mo->rndindex = rndindex;
	}
}

//
// CL_DamageMobj
//
void CL_DamageMobj()
{
	AActor  *mo;
	uint32_t netid;
	int health, pain;

	netid = MSG_ReadUnVarint();
	health = MSG_ReadShort();
	pain = MSG_ReadByte();

	mo = P_FindThingById (netid);

	if (!mo)
		return;

	mo->health = health;

	if(pain < mo->info->painchance)
		P_SetMobjState(mo, mo->info->painstate);
}

int connecttimeout = 0;

//
// [denis] CL_RequestConnectInfo
// Do what a launcher does...
//
void CL_RequestConnectInfo(void)
{
	if (!serveraddr.ip[0])
		return;

	gamestate = GS_CONNECTING;

	if(!connecttimeout)
	{
		connecttimeout = 140;

		Printf(PRINT_HIGH, "connecting to %s\n", NET_AdrToString(serveraddr));

		SZ_Clear(&net_buffer);
		MSG_WriteLong(&net_buffer, LAUNCHER_CHALLENGE);
		NET_SendPacket(net_buffer, serveraddr);
	}

	connecttimeout--;
}

/**
 * @brief Quit the network game while attempting to download a file.
 *
 * @param missing_file Missing file to attempt to download.
 */
static void QuitAndTryDownload(const OWantFile& missing_file)
{
	// Need to set this here, otherwise we render a frame of wild pointers
	// filled with garbage data.
	gamestate = GS_FULLCONSOLE;

	if (missing_file.getBasename().empty())
	{
		Printf(PRINT_WARNING,
		       "Tried to download an empty file.  This is probably a bug "
		       "in the client where an empty file is considered missing.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame();
		return;
	}

	if (!cl_serverdownload)
	{
		// Downloading is disabled client-side
		Printf(PRINT_WARNING,
		       "Unable to find \"%s\". Downloading is disabled on your client.  Go to "
		       "Options > Network Options to enable downloading.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame();
		return;
	}

	if (netdemo.isPlaying())
	{
		// Playing a netdemo and unable to download from the server
		Printf(PRINT_WARNING,
		       "Unable to find \"%s\".  Cannot download while playing a netdemo.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame();
		return;
	}

	if (sv_downloadsites.str().empty() && cl_downloadsites.str().empty())
	{
		// Nobody has any download sites configured.
		Printf("Unable to find \"%s\".  Both your client and the server have no "
		       "download sites configured.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame();
		return;
	}

	// Gather our server and client sites.
	StringTokens serversites = TokenizeString(sv_downloadsites.str(), " ");
	StringTokens clientsites = TokenizeString(cl_downloadsites.str(), " ");

	// Shuffle the sites so we evenly distribute our requests.
	std::random_shuffle(serversites.begin(), serversites.end());
	std::random_shuffle(clientsites.begin(), clientsites.end());

	// Combine them into one big site list.
	Websites downloadsites;
	downloadsites.reserve(serversites.size() + clientsites.size());
	downloadsites.insert(downloadsites.end(), serversites.begin(), serversites.end());
	downloadsites.insert(downloadsites.end(), clientsites.begin(), clientsites.end());

	// Disconnect from the server before we start the download.
	Printf(PRINT_HIGH, "Need to download \"%s\", disconnecting from server...\n",
	       missing_file.getBasename().c_str());
	CL_QuitNetGame();

	// Start the download.
	CL_StartDownload(downloadsites, missing_file, DL_RECONNECT);
}

//
// [denis] CL_PrepareConnect
// Process server info and switch to the right wads...
//
bool CL_PrepareConnect(void)
{
	G_CleanupDemo();	// stop demos from playing before D_DoomWadReboot wipes out Zone memory

	cvar_t::C_BackupCVars(CVAR_SERVERINFO);

	DWORD server_token = MSG_ReadLong();
	server_host = MSG_ReadString();

	bool recv_teamplay_stats = 0;
	gameversiontosend = 0;

	byte playercount = MSG_ReadByte(); // players
	MSG_ReadByte(); // max_players

	std::string server_map = MSG_ReadString();
	byte server_wads = MSG_ReadByte();

	Printf(PRINT_HIGH, "\n");
	Printf(PRINT_HIGH, "> Server: %s\n", server_host.c_str());
	Printf(PRINT_HIGH, "> Map: %s\n", server_map.c_str());

	std::vector<std::string> newwadnames;
	newwadnames.reserve(server_wads);
	for (byte i = 0; i < server_wads; i++)
	{
		newwadnames.push_back(MSG_ReadString());
	}

	MSG_ReadBool();							// deathmatch
	MSG_ReadByte();							// skill
	recv_teamplay_stats |= MSG_ReadBool();	// teamplay
	recv_teamplay_stats |= MSG_ReadBool();	// ctf

	for (byte i = 0; i < playercount; i++)
	{
		MSG_ReadString();
		MSG_ReadShort();
		MSG_ReadLong();
		MSG_ReadByte();
	}

	OWantFiles newwadfiles;
	newwadfiles.resize(server_wads);
	for (byte i = 0; i < server_wads; i++)
	{
		OWantFile& file = newwadfiles.at(i);
		std::string hash = MSG_ReadString();
		if (!OWantFile::makeWithHash(file, newwadnames.at(i), OFILE_WAD, hash))
		{
			Printf(PRINT_WARNING,
			       "Could not construct wanted file \"%s\" that server requested.\n",
			       newwadnames.at(i).c_str());
			CL_QuitNetGame();
			return false;
		}

		Printf("> %s\n   %s\n", file.getBasename().c_str(), file.getWantedHash().c_str());
	}

	// Download website - needed for HTTP downloading to work.
	sv_downloadsites.Set(MSG_ReadString());

	// Receive conditional teamplay information
	if (recv_teamplay_stats)
	{
		MSG_ReadLong();

		for(size_t i = 0; i < NUMTEAMS; i++)
		{
			bool enabled = MSG_ReadBool();

			if (enabled)
				MSG_ReadLong();
		}
	}

	version = MSG_ReadShort();

	Printf(PRINT_HIGH, "> Server protocol version: %i\n", version);

	if(version > VERSION)
		version = VERSION;
	if(version < 62)
		version = 62;

	/* GhostlyDeath -- Need the actual version info */
	if (version == 65)
	{
		size_t l;
		MSG_ReadString();

		for (l = 0; l < 3; l++)
			MSG_ReadShort();
		for (l = 0; l < 14; l++)
			MSG_ReadBool();
		for (l = 0; l < playercount; l++)
		{
			MSG_ReadShort();
			MSG_ReadShort();
			MSG_ReadShort();
		}

		MSG_ReadLong();
		MSG_ReadShort();

		for (l = 0; l < playercount; l++)
			MSG_ReadBool();

		MSG_ReadLong();
		MSG_ReadShort();

		gameversion = MSG_ReadLong();

		// GhostlyDeath -- Assume 40 for compatibility and fake it
		if (((gameversion % 256) % 10) == -1)
		{
			gameversion = 40;
			gameversiontosend = 40;
		}

		Printf(PRINT_HIGH, "> Server Version %i.%i.%i\n", gameversion / 256, (gameversion % 256) / 10, (gameversion % 256) % 10);
	}

    Printf(PRINT_HIGH, "\n");

	// DEH/BEX Patch files
	size_t patch_count = MSG_ReadByte();

	OWantFiles newpatchfiles;
	newpatchfiles.resize(patch_count);
	for (byte i = 0; i < patch_count; ++i)
	{
		OWantFile& file = newpatchfiles.at(i);
		std::string filename = MSG_ReadString();
		if (!OWantFile::make(file, filename, OFILE_DEH))
		{
			Printf(PRINT_WARNING,
			       "Could not construct wanted file \"%s\" that server requested.\n",
			       filename.c_str());
			CL_QuitNetGame();
			return false;
		}

		Printf("> %s\n", file.getBasename().c_str());
	}

    // TODO: Allow deh/bex file downloads
	bool ok = D_DoomWadReboot(newwadfiles, newpatchfiles);
	if (!ok && missingfiles.empty())
	{
		Printf(PRINT_WARNING, "Could not load required set of WAD files.\n");
		CL_QuitNetGame();
		return false;
	}
	else if (!ok && !missingfiles.empty() || cl_forcedownload)
	{
		OWantFile missing_file;
		if (missingfiles.empty())				// cl_forcedownload
		{
			missing_file = newwadfiles.back();
		}
		else									// client is really missing a file
		{
			missing_file = missingfiles.front();
		}

		QuitAndTryDownload(missing_file);
		return false;
	}

	recv_full_update = false;

	connecttimeout = 0;
	CL_TryToConnect(server_token);

	return true;
}

//
//  Connecting to a server...
//
bool CL_Connect(void)
{
	players.clear();

	memset(packetseq, -1, sizeof(packetseq) );
	packetnum = 0;

	MSG_WriteMarker(&net_buffer, clc_ack);
	MSG_WriteLong(&net_buffer, 0);

	compressor.reset();

	connected = true;
    multiplayer = true;
    network_game = true;
	serverside = false;
	simulated_connection = netdemo.isPlaying();

	CL_Decompress(0);
	CL_ParseCommands();

	if (gameaction == ga_fullconsole) // Host_EndGame was called
		return false;

	D_SetupUserInfo();

	//raise the weapon
	if(validplayer(consoleplayer()))
		consoleplayer().psprites[ps_weapon].sy = 32*FRACUNIT+0x6000;

	noservermsgs = false;
	last_received = gametic;

	gamestate = GS_CONNECTED;

	return true;
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

    SZ_Clear(&net_buffer);

    size_t ParamIndex = Args.CheckParm ("-connect");

    if (ParamIndex)
    {
		const char *ipaddress = Args.GetArg(ParamIndex + 1);

		if (ipaddress && ipaddress[0] != '-' && ipaddress[0] != '+')
		{
			NET_StringToAdr (ipaddress, &serveraddr);

			const char *passhash = Args.GetArg(ParamIndex + 2);

			if (passhash && passhash[0] != '-' && passhash[0] != '+')
			{
				connectpasshash = MD5SUM(passhash);
			}

			if (!serveraddr.port)
				I_SetPort(serveraddr, SERVERPORT);

			lastconaddr = serveraddr;
			gamestate = GS_CONNECTING;
		}
    }

    connected = false;
}

void CL_TryToConnect(DWORD server_token)
{
	if (!serveraddr.ip[0])
		return;

	if (!connecttimeout)
	{
		connecttimeout = 140; // 140 tics = 4 seconds

		Printf("challenging %s\n", NET_AdrToString(serveraddr));

		SZ_Clear(&net_buffer);
		MSG_WriteLong(&net_buffer, CHALLENGE); // send challenge
		MSG_WriteLong(&net_buffer, server_token); // confirm server token
		MSG_WriteShort(&net_buffer, version); // send client version
		MSG_WriteByte(&net_buffer, 0); // send type of connection (play/spectate/rcon/download)

		// GhostlyDeath -- Send more version info
		if (gameversiontosend)
			MSG_WriteLong(&net_buffer, gameversiontosend);
		else
			MSG_WriteLong(&net_buffer, GAMEVER);

		CL_SendUserInfo(); // send userinfo

		// [SL] The "rate" CVAR has been deprecated. Now just send a hard-coded
		// maximum rate that the server will ignore.
		const int rate = 0xFFFF;
		MSG_WriteLong(&net_buffer, rate); 

        MSG_WriteString(&net_buffer, (char *)connectpasshash.c_str());

		NET_SendPacket(net_buffer, serveraddr);
		SZ_Clear(&net_buffer);
	}

	connecttimeout--;
}

EXTERN_CVAR (show_messages)

//
// CL_Print
//
void CL_Print (void)
{
	byte level = MSG_ReadByte();
	const char *str = MSG_ReadString();

	// Disallow getting NORCON messages
	if (level == PRINT_NORCON)
		return;

	// TODO : Clientchat moved, remove that but PRINT_SERVERCHAT
	if (level == PRINT_CHAT)
		Printf(level, "%s*%s", TEXTCOLOR_ESCAPE, str);	
	else if (level == PRINT_TEAMCHAT)
		Printf(level, "%s!%s", TEXTCOLOR_ESCAPE, str);
	else if (level == PRINT_SERVERCHAT)
		Printf(level, "%s%s", TEXTCOLOR_YELLOW, str);
	else
		Printf(level, "%s", str);

	if (show_messages)
	{
		if (level == PRINT_CHAT || level == PRINT_SERVERCHAT)
			S_Sound(CHAN_INTERFACE, gameinfo.chatSound, 1, ATTN_NONE);
		else if (level == PRINT_TEAMCHAT)
			S_Sound(CHAN_INTERFACE, "misc/teamchat", 1, ATTN_NONE);
	}
}

// Print a message in the middle of the screen
void CL_MidPrint (void)
{
    const char *str = MSG_ReadString();
    int msgtime = MSG_ReadShort();

    C_MidPrint(str,NULL,msgtime);
}

/**
 * Handle the svc_say server message, which contains a message from another
 * client with a player id attached to it.
 */
void CL_Say()
{
	byte message_visibility = MSG_ReadByte();
	byte player_id = MSG_ReadByte();
	const char* message = MSG_ReadString();

	bool filtermessage = false;

	player_t &player = idplayer(player_id);

	if (!validplayer(player))
		return;

	bool spectator = player.spectator || player.playerstate == PST_DOWNLOAD;

	if (consoleplayer().id != player.id)
	{
		if (spectator && mute_spectators)
			filtermessage = true;

		if (mute_enemies && !spectator &&
		    (G_IsFFAGame() ||
		    (G_IsTeamGame() &&
		     player.userinfo.team != consoleplayer().userinfo.team)))
			filtermessage = true;
	}

	const char* name = player.userinfo.netname.c_str();

	if (message_visibility == 0)
	{
		if (strnicmp(message, "/me ", 4) == 0)
			Printf(filtermessage ? PRINT_FILTERCHAT : PRINT_CHAT, "* %s %s\n", name, &message[4]);
		else
			Printf(filtermessage ? PRINT_FILTERCHAT : PRINT_CHAT, "%s: %s\n", name,
			       message);

		if (show_messages && !filtermessage)
			S_Sound(CHAN_INTERFACE, gameinfo.chatSound, 1, ATTN_NONE);
	}
	else if (message_visibility == 1)
	{
		if (strnicmp(message, "/me ", 4) == 0)
			Printf(PRINT_TEAMCHAT, "* %s %s\n", name, &message[4]);
		else
			Printf(PRINT_TEAMCHAT, "%s: %s\n", name, message);

		if (show_messages)
			S_Sound(CHAN_INTERFACE, "misc/teamchat", 1, ATTN_NONE);
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

void CL_UpdatePlayer()
{
	byte who = MSG_ReadByte();
	player_t *p = &idplayer(who);

	MSG_ReadLong();	// Read and ignore for now

	fixed_t x = MSG_ReadLong();
	fixed_t y = MSG_ReadLong();
	fixed_t z = MSG_ReadLong();

	angle_t angle = MSG_ReadShort() << FRACBITS;
	angle_t pitch = MSG_ReadShort() << FRACBITS;

	int frame = MSG_ReadByte();
	fixed_t momx = MSG_ReadLong();
	fixed_t momy = MSG_ReadLong();
	fixed_t momz = MSG_ReadLong();

	int invisibility = MSG_ReadByte();

	if	(!validplayer(*p) || !p->mo)
		return;

	// Mark the gametic this update arrived in for prediction code
	p->tic = gametic;

	// GhostlyDeath -- Servers will never send updates on spectators
	if (p->spectator && (p != &consoleplayer()))
		p->spectator = 0;

	// [Russell] - hack, read and set invisibility flag
	p->powers[pw_invisibility] = invisibility;
	if (p->powers[pw_invisibility])
		p->mo->flags |= MF_SHADOW;
	else
		p->mo->flags &= ~MF_SHADOW;

	// This is a very bright frame. Looks cool :)
	if (frame == PLAYER_FULLBRIGHTFRAME)
		frame = 32773;

	// denis - fixme - security
	if(!p->mo->sprite || (p->mo->frame&FF_FRAMEMASK) >= sprites[p->mo->sprite].numframes)
		return;

	p->last_received = gametic;
	last_player_update = gametic;

	// [SL] 2012-02-21 - Save the position information to a snapshot
	int snaptime = last_svgametic;
	PlayerSnapshot newsnap(snaptime);
	newsnap.setAuthoritative(true);

	newsnap.setX(x);
	newsnap.setY(y);
	newsnap.setZ(z);
	newsnap.setMomX(momx);
	newsnap.setMomY(momy);
	newsnap.setMomZ(momz);
	newsnap.setAngle(angle);
	newsnap.setPitch(pitch);
	newsnap.setFrame(frame);

	// Mark the snapshot as continuous unless the player just teleported
	// and lerping should be disabled
	newsnap.setContinuous(!CL_PlayerJustTeleported(p));
	CL_ClearPlayerJustTeleported(p);

	p->snapshots.addSnapshot(newsnap);
}

ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, BOOL dropped);

void CL_UpdatePlayerState()
{
	byte id = MSG_ReadByte();
	int health = MSG_ReadVarint();
	int armortype = MSG_ReadVarint();
	int armorpoints = MSG_ReadVarint();
	int lives = MSG_ReadVarint();
	weapontype_t weap = static_cast<weapontype_t>(MSG_ReadVarint());

	byte cardByte = MSG_ReadByte();
	std::bitset<6> cardBits(cardByte);

	int ammo[NUMAMMO];
	for (int i = 0; i < NUMAMMO; i++)
		ammo[i] = MSG_ReadVarint();

	statenum_t stnum[NUMPSPRITES] = {S_NULL, S_NULL};
	for (int i = 0; i < NUMPSPRITES; i++)
	{
		unsigned int state = MSG_ReadUnVarint();
		if (state >= NUMSTATES)
		{
			continue;
		}
		stnum[i] = static_cast<statenum_t>(state);
	}

	int powerups[NUMPOWERS];
	for (int i = 0; i < NUMPOWERS; i++)
		powerups[i] = MSG_ReadVarint();

	player_t& player = idplayer(id);
	if (!validplayer(player) || !player.mo)
		return;

	player.health = player.mo->health = health;
	player.armortype = armortype;
	player.armorpoints = armorpoints;
	player.lives = lives;

	player.readyweapon = weap;
	player.pendingweapon = wp_nochange;

	for (int i = 0; i < NUMCARDS; i++)
		player.cards[i] = cardBits[i];

	if (!player.weaponowned[weap])
		P_GiveWeapon(&player, weap, false);

	for (int i = 0; i < NUMAMMO; i++)
		player.ammo[i] = ammo[i];

	for (int i = 0; i < NUMPSPRITES; i++)
		P_SetPsprite(&player, i, stnum[i]);

	for (int i = 0; i < NUMPOWERS; i++)
		player.powers[i] = powerups[i];

}

//
// CL_UpdateLocalPlayer
//
void CL_UpdateLocalPlayer(void)
{
	player_t &p = consoleplayer();

	// The server has processed the ticcmd that the local client sent
	// during the the tic referenced below
	p.tic = MSG_ReadLong();

	fixed_t x = MSG_ReadLong();
	fixed_t y = MSG_ReadLong();
	fixed_t z = MSG_ReadLong();

	fixed_t momx = MSG_ReadLong();
	fixed_t momy = MSG_ReadLong();
	fixed_t momz = MSG_ReadLong();

	byte waterlevel = MSG_ReadByte();

	int snaptime = last_svgametic;
	PlayerSnapshot newsnapshot(snaptime);
	newsnapshot.setAuthoritative(true);
	newsnapshot.setX(x);
	newsnapshot.setY(y);
	newsnapshot.setZ(z);
	newsnapshot.setMomX(momx);
	newsnapshot.setMomY(momy);
	newsnapshot.setMomZ(momz);
	newsnapshot.setWaterLevel(waterlevel);

	// Mark the snapshot as continuous unless the player just teleported
	// and lerping should be disabled
	newsnapshot.setContinuous(!CL_PlayerJustTeleported(&p));
	CL_ClearPlayerJustTeleported(&p);

	consoleplayer().snapshots.addSnapshot(newsnapshot);
}


//
// CL_SaveSvGametic
//
// Receives the server's gametic at the time the packet was sent.  It will be
// sent back to the server with the next cmd.
//
// [SL] 2011-05-11
void CL_SaveSvGametic(void)
{
	byte t = MSG_ReadByte();

	int newtic = (last_svgametic & 0xFFFFFF00) + t;

	if (last_svgametic > newtic + 127)
		newtic += 256;

	last_svgametic = newtic;

	#ifdef _WORLD_INDEX_DEBUG_
	Printf(PRINT_HIGH, "Gametic %i, received world index %i\n", gametic, last_svgametic);
	#endif	// _WORLD_INDEX_DEBUG_
}

//
// CL_SendPingReply
//
// Replies to a server's ping request
//
// [SL] 2011-05-11 - Changed from CL_ResendSvGametic to CL_SendPingReply
// for clarity since it sends timestamps, not gametics.
//
void CL_SendPingReply(void)
{
	int svtimestamp = MSG_ReadLong();
	MSG_WriteMarker (&net_buffer, clc_pingreply);
	MSG_WriteLong (&net_buffer, svtimestamp);
}

//
// CL_UpdatePing
// Update ping value
//
void CL_UpdatePing(void)
{
	player_t &p = idplayer(MSG_ReadByte());
	p.ping = MSG_ReadLong();
}


//
// CL_UpdateIntTimeLeft
// Changes the value of level.inttimeleft
//
void CL_UpdateIntTimeLeft(void)
{
	level.inttimeleft = MSG_ReadShort();	// convert from seconds to tics
}


//
// CL_SpawnMobj
//
void CL_SpawnMobj()
{
	AActor  *mo;

	fixed_t x = MSG_ReadLong();
	fixed_t y = MSG_ReadLong();
	fixed_t z = MSG_ReadLong();
	angle_t angle = MSG_ReadLong();

	unsigned short type = MSG_ReadShort();
	uint32_t netid = MSG_ReadUnVarint();
	byte rndindex = MSG_ReadByte();
	SWORD state = MSG_ReadShort();

	if(type >= NUMMOBJTYPES)
		return;

	P_ClearId(netid);

	mo = new AActor (x, y, z, (mobjtype_t)type);

	// denis - puff hack
	if(mo->type == MT_PUFF)
	{
		mo->momz = FRACUNIT;
		mo->tics -= M_Random () & 3;
		if (mo->tics < 1)
			mo->tics = 1;
	}

	mo->angle = angle;
	P_SetThingId(mo, netid);
	mo->rndindex = rndindex;

	if (state < NUMSTATES)
		P_SetMobjState(mo, (statenum_t)state);

	if(mo->flags & MF_MISSILE)
	{
		AActor* target = P_FindThingById(MSG_ReadUnVarint());
		if(target)
			mo->target = target->ptr();
		CL_SetMobjSpeedAndAngle();
	}

    if (serverside && mo->flags & MF_COUNTKILL)
		level.total_monsters++;

	if (connected && (mo->flags & MF_MISSILE ) && mo->info->seesound)
		S_Sound (mo, CHAN_VOICE, mo->info->seesound, 1, ATTN_NORM);

	if (mo->type == MT_IFOG)
		S_Sound (mo, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);

	if (mo->type == MT_TFOG)
	{
		if (level.time)	// don't play sound on first tic of the level
			S_Sound (mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
	}

	if (type == MT_FOUNTAIN)
	{
		mo->effects = int(MSG_ReadByte()) << FX_FOUNTAINSHIFT;
	}

	if (type == MT_ZDOOMBRIDGE)
	{
		mo->radius = int(MSG_ReadByte()) << FRACBITS;
		mo->height = int(MSG_ReadByte()) << FRACBITS;
	}
}

//
// CL_Corpse
// Called after killed thing is created.
//
void CL_Corpse(void)
{
	AActor* mo = P_FindThingById(MSG_ReadUnVarint());
	int frame = MSG_ReadByte();
	int tics = MSG_ReadByte();

	if(tics == 0xFF)
		tics = -1;

	// already spawned as gibs?
	if (!mo || mo->state - states == S_GIBS)
		return;

	if((frame&FF_FRAMEMASK) >= sprites[mo->sprite].numframes)
		return;

	mo->frame = frame;
	mo->tics = tics;

	// from P_KillMobj
	mo->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);
	mo->flags |= MF_CORPSE|MF_DROPOFF;
	mo->height >>= 2;
	mo->flags &= ~MF_SOLID;

	if (mo->player)
		mo->player->playerstate = PST_DEAD;
}

//
// CL_TouchSpecialThing
//
void CL_TouchSpecialThing (void)
{
	AActor* mo = P_FindThingById(MSG_ReadUnVarint());

	if(!consoleplayer().mo || !mo)
		return;

	P_GiveSpecial(&consoleplayer(), mo);
}


//
// CL_SpawnPlayer
//
void CL_SpawnPlayer()
{
	byte		playernum;
	player_t	*p;
	AActor		*mobj;
	fixed_t		x = 0, y = 0, z = 0;
	uint32_t	netid;
	angle_t		angle = 0;
	int			i;

	playernum = MSG_ReadByte();
	netid = MSG_ReadUnVarint();
	p = &CL_FindPlayer(playernum);

	angle = MSG_ReadLong();
	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();

	P_ClearId(netid);

	// first disassociate the corpse
	if (p->mo)
	{
		p->mo->player = NULL;
		p->mo->health = 0;
	}

	G_PlayerReborn (*p);

	mobj = new AActor (x, y, z, MT_PLAYER);

	mobj->momx = mobj->momy = mobj->momz = 0;

	// set color translations for player sprites
	mobj->translation = translationref_t(translationtables + 256*playernum, playernum);
	mobj->angle = angle;
	mobj->pitch = 0;
	mobj->player = p;
	mobj->health = p->health;
	P_SetThingId(mobj, netid);

	p->mo = p->camera = mobj->ptr();
	p->fov = 90.0f;
	p->playerstate = PST_LIVE;
	p->refire = 0;
	p->damagecount = 0;
	p->bonuscount = 0;
	p->extralight = 0;
	p->fixedcolormap = 0;

	p->xviewshift = 0;
	p->viewheight = VIEWHEIGHT;

	p->attacker = AActor::AActorPtr();
	p->viewz = z + VIEWHEIGHT;

	// spawn a teleport fog
	// tfog = new AActor (x, y, z, MT_TFOG);

	// setup gun psprite
	P_SetupPsprites (p);

	// give all cards in death match mode
	if(sv_gametype != GM_COOP)
		for (i = 0; i < NUMCARDS; i++)
			p->cards[i] = true;

	if(p->id == consoleplayer_id)
	{
		// denis - if this concerns the local player, restart the status bar
		ST_Start ();

		// [SL] 2012-04-23 - Clear predicted sectors
		movingsectors.clear();
	}

	if (p->id == displayplayer().id)
	{
		// [SL] 2012-03-08 - Resync with the server's incoming tic since we don't care
		// about players/sectors jumping to new positions when the displayplayer spawns
		CL_ResyncWorldIndex();
	}

	if (level.behavior && !p->spectator && p->playerstate == PST_LIVE)
	{
		if (p->deathcount)
			level.behavior->StartTypedScripts(SCRIPT_Respawn, p->mo);
		else
			level.behavior->StartTypedScripts(SCRIPT_Enter, p->mo);
	}

	int snaptime = last_svgametic;
	PlayerSnapshot newsnap(snaptime, p);
	newsnap.setAuthoritative(true);
	newsnap.setContinuous(false);
	p->snapshots.clearSnapshots();
	p->snapshots.addSnapshot(newsnap);
}

//
// CL_PlayerInfo
// denis - your personal arsenal, as supplied by the server
//
void CL_PlayerInfo()
{
	player_t* p = &consoleplayer();

	uint16_t booleans = MSG_ReadShort();
	for (int i = 0; i < NUMWEAPONS; i++)
		p->weaponowned[i] = booleans & 1 << i;
	for (int i = 0; i < NUMCARDS; i++)
		p->cards[i] = booleans & 1 << (i + NUMWEAPONS);
	p->backpack = booleans & 1 << (NUMWEAPONS + NUMCARDS);

	for (int i = 0; i < NUMAMMO; i++)
	{
		p->maxammo[i] = MSG_ReadVarint();
		p->ammo[i] = MSG_ReadVarint();
	}

	p->health = MSG_ReadVarint();
	p->armorpoints = MSG_ReadVarint();
	p->armortype = MSG_ReadVarint();
	p->lives = MSG_ReadVarint();

	weapontype_t newweapon = static_cast<weapontype_t>(MSG_ReadVarint());
	if (newweapon > NUMWEAPONS) // bad weapon number, choose something else
		newweapon = wp_fist;

	if (newweapon != p->readyweapon)
		p->pendingweapon = newweapon;

	for (int i = 0; i < NUMPOWERS; i++)
		p->powers[i] = MSG_ReadVarint();
}

//
// CL_SetMobjSpeedAndAngle
//
void CL_SetMobjSpeedAndAngle(void)
{
	AActor *mo;

	uint32_t netid = MSG_ReadUnVarint();
	mo = P_FindThingById(netid);

	angle_t angle = MSG_ReadLong();
	fixed_t momx = MSG_ReadLong();
	fixed_t momy = MSG_ReadLong();
	fixed_t momz = MSG_ReadLong();

	if (!mo)
		return;

	if (mo->player)
	{
		// [SL] 2013-07-21 - Save the position information to a snapshot
		int snaptime = last_svgametic;
		PlayerSnapshot newsnap(snaptime);
		newsnap.setAuthoritative(true);

		newsnap.setMomX(momx);
		newsnap.setMomY(momy);
		newsnap.setMomZ(momz);

		mo->player->snapshots.addSnapshot(newsnap);
	}
	else
	{
		mo->angle = angle;
		mo->momx = momx;
		mo->momy = momy;
		mo->momz = momz;
	}
}

//
// CL_ExplodeMissile
//
void CL_ExplodeMissile(void)
{
    AActor *mo;
	uint32_t netid;

	netid = MSG_ReadUnVarint();
	mo = P_FindThingById(netid);

	if (!mo)
		return;

	P_ExplodeMissile(mo);
}


//
// CL_RailTrail
//
void CL_RailTrail()
{
	v3double_t start, end;

	start.x = double(MSG_ReadShort());
	start.y = double(MSG_ReadShort());
	start.z = double(MSG_ReadShort());
	end.x = double(MSG_ReadShort());
	end.y = double(MSG_ReadShort());
	end.z = double(MSG_ReadShort());

	P_DrawRailTrail(start, end);
}

//
// CL_UpdateMobjInfo
//
void CL_UpdateMobjInfo(void)
{
	uint32_t netid = MSG_ReadUnVarint();
	int flags = MSG_ReadLong();
	//int flags2 = MSG_ReadLong();

	AActor *mo = P_FindThingById(netid);

	if (!mo)
		return;

	mo->flags = flags;
	//mo->flags2 = flags2;
}


//
// CL_RemoveMobj
//
void CL_RemoveMobj(void)
{
	uint32_t netid = MSG_ReadUnVarint();

	AActor *mo = P_FindThingById(netid);
	if (mo && mo->player && mo->player->id == displayplayer_id)
		displayplayer_id = consoleplayer_id;

	P_ClearId(netid);
}


//
// CL_DamagePlayer
//
void CL_DamagePlayer(void)
{
	uint32_t netid = MSG_ReadUnVarint();
	int healthDamage = MSG_ReadShort();
	int armorDamage = MSG_ReadByte();

	AActor* actor = P_FindThingById(netid);

	if (!actor || !actor->player)
		return;

	player_t *p = actor->player;
	p->health -= healthDamage;
	p->mo->health = p->health;
	p->armorpoints -= armorDamage;

	if (p->health < 0)
		p->health = 0;
	if (p->armorpoints < 0)
		p->armorpoints = 0;

	if (healthDamage > 0)
	{
		p->damagecount += healthDamage;

		if (p->damagecount > 100)
			p->damagecount = 100;

		if(p->mo->info->painstate)
			P_SetMobjState(p->mo, p->mo->info->painstate);
	}
}

extern int MeansOfDeath;

//
// CL_KillMobj
//
void CL_KillMobj()
{
	uint32_t srcid = MSG_ReadUnVarint();
	uint32_t tgtid = MSG_ReadUnVarint();
	uint32_t infid = MSG_ReadUnVarint();
	int health = MSG_ReadVarint();
	::MeansOfDeath = MSG_ReadVarint();
	bool joinkill = MSG_ReadBool();
	int lives = MSG_ReadVarint();

	AActor* source = P_FindThingById(srcid);
	AActor* target = P_FindThingById(tgtid);
	AActor* inflictor = P_FindThingById(infid);

	if (!target)
		return;

	target->health = health;

	if (!serverside && target->flags & MF_COUNTKILL)
		level.killed_monsters++;

	if (target->player == &consoleplayer())
		for (size_t i = 0; i < MAXSAVETICS; i++)
			localcmds[i].clear();

	if (target->player && lives >= 0)
		target->player->lives = lives;

	P_KillMobj(source, target, inflictor, joinkill);
}

///////////////////////////////////////////////////////////
///// CL_Fire* called when someone uses a weapon  /////////
///////////////////////////////////////////////////////////

// [tm512] attempt at squashing weapon desyncs.
// The server will send us what weapon we fired, and if that
// doesn't match the weapon we have up at the moment, fix it
// and request that we get a full update of playerinfo - apr 14 2012
void CL_FireWeapon (void)
{
	player_t *p = &consoleplayer ();
	weapontype_t firedweap = (weapontype_t) MSG_ReadByte ();
	int servertic = MSG_ReadLong ();

	if (firedweap != p->readyweapon)
	{
		DPrintf("CL_FireWeapon: weapon misprediction\n");
		A_ForceWeaponFire(p->mo, firedweap, servertic);

		// Request the player's ammo status from the server
		MSG_WriteMarker (&net_buffer, clc_getplayerinfo);
	}

}

//
// CL_FirePistol
//
void CL_FirePistol(void)
{
	player_t &p = idplayer(MSG_ReadByte());

	if(!validplayer(p))
		return;

	if(!p.mo)
		return;

	if (&p != &consoleplayer())
		S_Sound (p.mo, CHAN_WEAPON, "weapons/pistol", 1, ATTN_NORM);
}

//
// CL_FireShotgun
//
void CL_FireShotgun(void)
{
	player_t &p = idplayer(MSG_ReadByte());

	if(!validplayer(p))
		return;

	if(!p.mo)
		return;

	if (&p != &consoleplayer())
		S_Sound (p.mo, CHAN_WEAPON,  "weapons/shotgf", 1, ATTN_NORM);
}

//
// CL_FireSSG
//
void CL_FireSSG(void)
{
	player_t &p = idplayer(MSG_ReadByte());

	if(!validplayer(p))
		return;

	if(!p.mo)
		return;

	if (&p != &consoleplayer())
		S_Sound (p.mo, CHAN_WEAPON, "weapons/sshotf", 1, ATTN_NORM);
}


//
// CL_FireChainGun
//
void CL_FireChainGun(void)
{
	player_t &p = idplayer(MSG_ReadByte());

	if(!validplayer(p))
		return;

	if(!p.mo)
		return;

	if (&p != &consoleplayer())
		S_Sound (p.mo, CHAN_WEAPON, "weapons/chngun", 1, ATTN_NORM);
}

//
// CL_ChangeWeapon
// [ML] From Zdaemon .99
//
void CL_ChangeWeapon (void)
{
	player_t *player = &consoleplayer();
	weapontype_t newweapon = (weapontype_t)MSG_ReadByte();

	// ensure that the client has the weapon
	player->weaponowned[newweapon] = true;

	// [SL] 2011-09-22 - Only change the weapon if the client doesn't already
	// have that weapon up.
	if (player->readyweapon != newweapon)
		player->pendingweapon = newweapon;
}


//
// CL_Sound
//
void CL_Sound(void)
{
	uint32_t netid = MSG_ReadUnVarint();
	int x = MSG_ReadLong();
	int y = MSG_ReadLong();
	byte channel = MSG_ReadByte();
	byte sfx_id = MSG_ReadByte();
	byte attenuation = MSG_ReadByte();
	byte vol = MSG_ReadByte();

	AActor *mo = P_FindThingById (netid);

	float volume = vol/(float)255;

	if(mo)
		S_SoundID (mo, channel, sfx_id, volume, attenuation); // play at thing location
	else
		S_SoundID (x, y, channel, sfx_id, volume, attenuation); // play at approximate thing location
}

void CL_SoundOrigin(void)
{
	int x = MSG_ReadLong();
	int y = MSG_ReadLong();
	byte channel = MSG_ReadByte();
	byte sfx_id = MSG_ReadByte();
	byte attenuation = MSG_ReadByte();
	byte vol = MSG_ReadByte();

	float volume = vol/(float)255;

	AActor *mo = consoleplayer().mo;

	if(!mo)
		return;

	S_SoundID (x, y, channel, sfx_id, volume, attenuation);
}

//
// CL_ClearSectorSnapshots
//
// Removes all sector snapshots at the start of a map, etc
//
void CL_ClearSectorSnapshots()
{
	sector_snaps.clear();
}

//
// CL_SecretEvent
// Client interpretation of a secret found by another player
//
void CL_SecretEvent()
{
	player_t& player = idplayer(MSG_ReadByte());
	unsigned short sectornum = (unsigned short)MSG_ReadShort();
	short special = MSG_ReadShort();

	if (!sectors || sectornum >= numsectors)
		return;

	sector_t* sector = &sectors[sectornum];
	sector->special = special;

	// Don't show other secrets if requested
	if (!hud_revealsecrets || hud_revealsecrets > 2)
		return;

	std::string buf;
	StrFormat(buf, "%s%s %sfound a secret!\n", TEXTCOLOR_YELLOW, player.userinfo.netname.c_str(), TEXTCOLOR_NORMAL);
	Printf(buf.c_str());

	if (hud_revealsecrets == 1)
		S_Sound(CHAN_INTERFACE, "misc/secret", 1, ATTN_NONE);
}

//
// CL_UpdateSector
// Updates floorheight and ceilingheight of a sector.
//
void CL_UpdateSector(void)
{
	unsigned short sectornum = (unsigned short)MSG_ReadShort();
	unsigned short floorheight = MSG_ReadShort();
	unsigned short ceilingheight = MSG_ReadShort();

	unsigned short fp = MSG_ReadShort();
	unsigned short cp = MSG_ReadShort();
	short special = MSG_ReadShort();

	if (!sectors || sectornum >= numsectors)
		return;

	sector_t *sector = &sectors[sectornum];
	P_SetCeilingHeight(sector, ceilingheight << FRACBITS);
	P_SetFloorHeight(sector, floorheight << FRACBITS);

	if(fp >= numflats)
		fp = numflats;

	sector->floorpic = fp;

	if(cp >= numflats)
		cp = numflats;

	sector->ceilingpic = cp;
	sector->special = special;
	sector->moveable = true;

	P_ChangeSector(sector, false);

	SectorSnapshot snap(last_svgametic, sector);
	sector_snaps[sectornum].addSnapshot(snap);
}

//
// CL_UpdateMovingSector
// Updates floorheight and ceilingheight of a sector.
//
void CL_UpdateMovingSector(void)
{
	unsigned short sectornum = (unsigned short)MSG_ReadShort();

    fixed_t ceilingheight = MSG_ReadShort() << FRACBITS;
    fixed_t floorheight = MSG_ReadShort() << FRACBITS;

	byte movers = MSG_ReadByte();
	movertype_t ceiling_mover = static_cast<movertype_t>(movers & 0x0F);
	movertype_t floor_mover = static_cast<movertype_t>((movers & 0xF0) >> 4);

	if (ceiling_mover == SEC_ELEVATOR)
		floor_mover = SEC_INVALID;
	if (ceiling_mover == SEC_PILLAR)
		floor_mover = SEC_INVALID;

	SectorSnapshot snap(last_svgametic);

	snap.setCeilingHeight(ceilingheight);
	snap.setFloorHeight(floorheight);

	if (floor_mover == SEC_FLOOR)
	{
		// Floors/Stairbuilders
		snap.setFloorMoverType(SEC_FLOOR);
		snap.setFloorType(static_cast<DFloor::EFloor>(MSG_ReadByte()));
		snap.setFloorStatus(MSG_ReadByte());
		snap.setFloorCrush(MSG_ReadBool());
		snap.setFloorDirection(char(MSG_ReadByte()));
		snap.setFloorSpecial(MSG_ReadShort());
		snap.setFloorTexture(MSG_ReadShort());
		snap.setFloorDestination(MSG_ReadShort() << FRACBITS);
		snap.setFloorSpeed(MSG_ReadLong());
		snap.setResetCounter(MSG_ReadLong());
		snap.setOrgHeight(MSG_ReadShort() << FRACBITS);
		snap.setDelay(MSG_ReadLong());
		snap.setPauseTime(MSG_ReadLong());
		snap.setStepTime(MSG_ReadLong());
		snap.setPerStepTime(MSG_ReadLong());
		snap.setFloorOffset(MSG_ReadShort() << FRACBITS);
		snap.setFloorChange(MSG_ReadByte());

		int LineIndex = MSG_ReadLong();

		if (!lines || LineIndex >= numlines || LineIndex < 0)
			snap.setFloorLine(NULL);
		else
			snap.setFloorLine(&lines[LineIndex]);
	}

	if (floor_mover == SEC_PLAT)
	{
		// Platforms/Lifts
		snap.setFloorMoverType(SEC_PLAT);
		snap.setFloorSpeed(MSG_ReadLong());
		snap.setFloorLow(MSG_ReadShort() << FRACBITS);
		snap.setFloorHigh(MSG_ReadShort() << FRACBITS);
		snap.setFloorWait(MSG_ReadLong());
		snap.setFloorCounter(MSG_ReadLong());
		snap.setFloorStatus(MSG_ReadByte());
		snap.setOldFloorStatus(MSG_ReadByte());
		snap.setFloorCrush(MSG_ReadBool());
		snap.setFloorTag(MSG_ReadShort());
		snap.setFloorType(MSG_ReadByte());
		snap.setFloorOffset(MSG_ReadShort() << FRACBITS);
		snap.setFloorLip(MSG_ReadShort() << FRACBITS);
	}

	if (ceiling_mover == SEC_CEILING)
	{
		// Ceilings / Crushers
		snap.setCeilingMoverType(SEC_CEILING);
		snap.setCeilingType(MSG_ReadByte());
		snap.setCeilingLow(MSG_ReadShort() << FRACBITS);
		snap.setCeilingHigh(MSG_ReadShort() << FRACBITS);
		snap.setCeilingSpeed(MSG_ReadLong());
		snap.setCrusherSpeed1(MSG_ReadLong());
		snap.setCrusherSpeed2(MSG_ReadLong());
		snap.setCeilingCrush(MSG_ReadBool());
		snap.setSilent(MSG_ReadBool());
		snap.setCeilingDirection(char(MSG_ReadByte()));
		snap.setCeilingTexture(MSG_ReadShort());
		snap.setCeilingSpecial(MSG_ReadShort());
		snap.setCeilingTag(MSG_ReadShort());
		snap.setCeilingOldDirection(char(MSG_ReadByte()));
    }

	if (ceiling_mover == SEC_DOOR)
	{
		// Doors
		snap.setCeilingMoverType(SEC_DOOR);
		snap.setCeilingType(static_cast<DDoor::EVlDoor>(MSG_ReadByte()));
		snap.setCeilingHigh(MSG_ReadShort() << FRACBITS);
		snap.setCeilingSpeed(MSG_ReadLong());
		snap.setCeilingWait(MSG_ReadLong());
		snap.setCeilingCounter(MSG_ReadLong());
		snap.setCeilingStatus(MSG_ReadByte());

		int LineIndex = MSG_ReadLong();

		// If the moving sector's line is -1, it is likely a type 666 door
		if (!lines || LineIndex >= numlines || LineIndex < 0)
			snap.setCeilingLine(NULL);
		else
			snap.setCeilingLine(&lines[LineIndex]);
	}

	if (ceiling_mover == SEC_ELEVATOR)
    {
		// Elevators
		snap.setCeilingMoverType(SEC_ELEVATOR);
		snap.setFloorMoverType(SEC_ELEVATOR);
		snap.setCeilingType(static_cast<DElevator::EElevator>(MSG_ReadByte()));
		snap.setFloorType(snap.getCeilingType());
		snap.setCeilingStatus(MSG_ReadByte());
		snap.setFloorStatus(snap.getCeilingStatus());
		snap.setCeilingDirection(char(MSG_ReadByte()));
		snap.setFloorDirection(snap.getCeilingDirection());
		snap.setFloorDestination(MSG_ReadShort() << FRACBITS);
		snap.setCeilingDestination(MSG_ReadShort() << FRACBITS);
		snap.setCeilingSpeed(MSG_ReadLong());
		snap.setFloorSpeed(snap.getCeilingSpeed());
	}

	if (ceiling_mover == SEC_PILLAR)
	{
		// Pillars
		snap.setCeilingMoverType(SEC_PILLAR);
		snap.setFloorMoverType(SEC_PILLAR);
		snap.setCeilingType(static_cast<DPillar::EPillar>(MSG_ReadByte()));
		snap.setFloorType(snap.getCeilingType());
		snap.setCeilingStatus(MSG_ReadByte());
		snap.setFloorStatus(snap.getCeilingStatus());
		snap.setFloorSpeed(MSG_ReadLong());
		snap.setCeilingSpeed(MSG_ReadLong());
		snap.setFloorDestination(MSG_ReadShort() << FRACBITS);
		snap.setCeilingDestination(MSG_ReadShort() << FRACBITS);
		snap.setCeilingCrush(MSG_ReadBool());
		snap.setFloorCrush(snap.getCeilingCrush());
	}

	if (!sectors || sectornum >= numsectors)
		return;

	snap.setSector(&sectors[sectornum]);

	sector_snaps[sectornum].addSnapshot(snap);
}


//
// CL_CheckMissedPacket
//
void CL_CheckMissedPacket(void)
{
	int    sequence;
	short  size;

	sequence = MSG_ReadLong();
	size = MSG_ReadShort();

	for (int n=0; n<256; n++)
	{
		// skip a duplicated packet
		if (packetseq[n] == sequence)
		{
            MSG_ReadChunk(size);

			#ifdef _DEBUG
                Printf (PRINT_WARNING, "warning: duplicate packet\n");
			#endif
			return;
		}
	}
}

// Decompress the packet sequence
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
void CL_Decompress(int sequence)
{
	if(!MSG_BytesLeft() || MSG_NextByte() != svc_compressed)
		return;
	else
		MSG_ReadByte();

	byte method = MSG_ReadByte();

#if 0
	if(method & adaptive_mask)
		MSG_DecompressAdaptive(compressor.codec_for_received(method & adaptive_select_mask ? 1 : 0));
	else
	{
		// otherwise compressed packets can still contain codec updates
		compressor.codec_for_received(method & adaptive_select_mask ? 1 : 0);
	}
#endif

	if(method & minilzo_mask)
		MSG_DecompressMinilzo();
#if 0
	if(method & adaptive_record_mask)
		compressor.ack_sent(net_message.ptr(), MSG_BytesLeft());
#endif
}

//
// CL_ReadPacketHeader
//
void CL_ReadPacketHeader(void)
{
	unsigned int sequence = MSG_ReadLong();

	MSG_WriteMarker(&net_buffer, clc_ack);
	MSG_WriteLong(&net_buffer, sequence);

	CL_Decompress(sequence);

	packetseq[packetnum] = sequence;
	packetnum++;

	netgraph.addPacketIn();
}

void CL_GetServerSettings(void)
{
	cvar_t *var = NULL, *prev = NULL;

	// TODO: REMOVE IN 0.7 - We don't need this loop anymore
	while (MSG_ReadByte() != 2)
	{
		std::string CvarName = MSG_ReadString();
		std::string CvarValue = MSG_ReadString();

		var = cvar_t::FindCVar(CvarName.c_str(), &prev);

		// GhostlyDeath <June 19, 2008> -- Read CVAR or dump it
		if (var)
		{
			if (var->flags() & CVAR_SERVERINFO)
				var->Set(CvarValue.c_str());
		}
		else
		{
			// [Russell] - create a new "temporary" cvar, CVAR_AUTO marks it
			// for cleanup on program termination
			// [AM] We have no way of telling of cvars are CVAR_NOENABLEDISABLE,
			//      so let's set it on all cvars.
			var = new cvar_t(CvarName.c_str(), NULL, "", CVARTYPE_NONE,
			                 CVAR_SERVERINFO | CVAR_AUTO | CVAR_UNSETTABLE |
			                 CVAR_NOENABLEDISABLE);
			var->Set(CvarValue.c_str());
		}
	}

	// Nes - update the skies in case sv_freelook is changed.
	R_InitSkyMap();
}

//
// CL_FinishedFullUpdate
//
// Takes care of any business that needs to be done once the client has a full
// view of the game world.
//
void CL_FinishedFullUpdate()
{
	recv_full_update = true;
}

void CL_StartFullUpdate()
{
	recv_full_update = false;
}

//
// CL_SetMobjState
//
void CL_SetMobjState()
{
	AActor *mo = P_FindThingById (MSG_ReadUnVarint() );
	SWORD s = MSG_ReadShort();

	if (!mo || s >= NUMSTATES)
		return;

	P_SetMobjState (mo, (statenum_t)s);
}

// ---------------------------------------------------------------------------------------------------------
//	CL_ForceSetTeam
//	Allows server to force set a players team setting
// ---------------------------------------------------------------------------------------------------------

void CL_ForceSetTeam (void)
{
	unsigned int t = MSG_ReadShort ();

	if(t < NUMTEAMS || t == TEAM_NONE)
		consoleplayer().userinfo.team = (team_t)t;

	// Setting the cl_team will send a playerinfo packet back to the server.
	// Unfortunately, this is unavoidable until we rework the team system.
	cl_team.Set(GetTeamInfo(consoleplayer().userinfo.team)->ColorStringUpper.c_str());
}

//
// CL_Actor_Movedir
//
void CL_Actor_Movedir()
{
	AActor* actor = P_FindThingById(MSG_ReadUnVarint());
	BYTE movedir = MSG_ReadByte();
    SDWORD movecount = MSG_ReadLong();

	if (!actor || movedir >= 8)
		return;

	actor->movedir = movedir;
	actor->movecount = movecount;
}

//
// CL_Actor_Target
//
void CL_Actor_Target()
{
	AActor* actor = P_FindThingById(MSG_ReadUnVarint());
	AActor* target = P_FindThingById(MSG_ReadUnVarint());

	if (!actor || !target)
		return;

	actor->target = target->ptr();
}

//
// CL_Actor_Tracer
//
void CL_Actor_Tracer()
{
	AActor* actor = P_FindThingById(MSG_ReadUnVarint());
	AActor* tracer = P_FindThingById(MSG_ReadUnVarint());

	if (!actor || !tracer)
		return;

	actor->tracer = tracer->ptr();
}

//
// CL_MobjTranslation
//
void CL_MobjTranslation()
{
	AActor* mo = P_FindThingById(MSG_ReadUnVarint());
	byte table = MSG_ReadByte();

	if (!mo)
		return;

	if (table <= MAXPLAYERS)
		mo->translation = translationref_t(translationtables + 256 * table, table);
	else
		mo->translation = translationref_t(translationtables + 256 * table);
}

void P_SetButtonTexture(line_t* line, short texture);
//
// CL_Switch
// denis - switch state and timing
// Note: this will also be called for doors
void CL_Switch()
{
	unsigned l = MSG_ReadLong();
	byte switchactive = MSG_ReadByte();
	byte special = MSG_ReadByte();
	byte state = MSG_ReadByte(); //DActiveButton::EWhere
	short texture = MSG_ReadShort();
	unsigned time = MSG_ReadLong();

	if (!lines || l >= (unsigned)numlines || state >= 3)
		return;

	if(!P_SetButtonInfo(&lines[l], state, time) && switchactive) // denis - fixme - security
		P_ChangeSwitchTexture(&lines[l], lines[l].flags & ML_REPEAT_SPECIAL, recv_full_update); //only playsound if we've received the full update from the server (not setting up the map from the server)

	if (!recv_full_update && texture) // Only accept texture change from server while receiving the full update - this is to fix warmup switch desyncs
		P_SetButtonTexture(&lines[l], texture);
	lines[l].special = special;
}

void ActivateLine(AActor* mo, line_s* line, byte side, LineActivationType activationType,
                  byte special = 0, int arg0 = 0, int arg1 = 0, int arg2 = 0,
                  int arg3 = 0, int arg4 = 0)
{
	// [SL] 2012-03-07 - If this is a player teleporting, add this player to
	// the set of recently teleported players.  This is used to flush past
	// positions since they cannot be used for interpolation.
	if (line && (mo && mo->player) &&
		(line->special == Teleport || line->special == Teleport_NoFog ||
			line->special == Teleport_NoStop || line->special == Teleport_Line))
	{
		teleported_players.insert(mo->player->id);

		// [SL] 2012-03-21 - Server takes care of moving players that teleport.
		// Don't allow client to process it since it screws up interpolation.
		return;
	}

	// [SL] 2012-04-25 - Clients will receive updates for sectors so they do not
	// need to create moving sectors on their own in response to svc_activateline
	if (line && P_LineSpecialMovesSector(line->special))
		return;

	s_SpecialFromServer = true;

	switch (activationType)
	{
	case LineCross:
		if (line)
			P_CrossSpecialLine(line - lines, side, mo);
		break;
	case LineUse:
		if (line)
			P_UseSpecialLine(mo, line, side);
		break;
	case LineShoot:
		if (line)
			P_ShootSpecialLine(mo, line);
		break;
	case LinePush:
		if (line)
			P_PushSpecialLine(mo, line, side);
		break;
	case LineACS:
		LineSpecials[special](line, mo, arg0, arg1, arg2, arg3, arg4);
		break;
	default:
		break;
	}

	s_SpecialFromServer = false;
}

void CL_ActivateLine(void)
{
	unsigned linenum = MSG_ReadLong();
	AActor* mo = P_FindThingById(MSG_ReadUnVarint());
	byte side = MSG_ReadByte();
	LineActivationType activationType = (LineActivationType)MSG_ReadByte();

	if (!lines || linenum >= (unsigned)numlines)
		return;

	ActivateLine(mo, &lines[linenum], side, activationType);
}

void CL_ConsolePlayer(void)
{
	displayplayer_id = consoleplayer_id = MSG_ReadByte();
	digest = MSG_ReadString();
}

bool IsGameModeFFA()
{
	return sv_gametype == GM_DM && sv_maxplayers > 2;
}

bool IsGameModeDuel()
{
	return sv_gametype == GM_DM && sv_maxplayers == 2;
}

//
// CL_LoadMap
//
// Read wad & deh filenames and map name from the server and loads
// the appropriate wads & map.
//
void CL_LoadMap()
{
	bool splitnetdemo = (netdemo.isRecording() && cl_splitnetdemos) || forcenetdemosplit;
	forcenetdemosplit = false;

	if (splitnetdemo)
		netdemo.stopRecording();

	size_t wadcount = MSG_ReadUnVarint();
	OWantFiles newwadfiles;
	newwadfiles.reserve(wadcount);
	for (size_t i = 0; i < wadcount; i++)
	{
		std::string name = MSG_ReadString();
		std::string hash = MSG_ReadString();

		OWantFile file;
		if (!OWantFile::makeWithHash(file, name, OFILE_WAD, hash))
		{
			Printf(PRINT_WARNING,
			       "Could not construct wanted file \"%s\" that server requested.\n",
			       name.c_str());
			CL_QuitNetGame();
			return;
		}
		newwadfiles.push_back(file);
	}

	size_t patchcount = MSG_ReadUnVarint();
	OWantFiles newpatchfiles;
	newpatchfiles.reserve(patchcount);
	for (size_t i = 0; i < patchcount; i++)
	{
		std::string name = MSG_ReadString();
		std::string hash = MSG_ReadString();

		OWantFile file;
		if (!OWantFile::makeWithHash(file, name, OFILE_DEH, hash))
		{
			Printf(PRINT_WARNING,
			       "Could not construct wanted patch \"%s\" that server requested.\n",
			       name.c_str());
			CL_QuitNetGame();
			return;
		}
		newpatchfiles.push_back(file);
	}

	const char *mapname = MSG_ReadString();
	int server_level_time = MSG_ReadVarint();

	// Load the specified WAD and DEH files and change the level.
	// if any WADs are missing, reconnect to begin downloading.
	G_LoadWad(newwadfiles, newpatchfiles);

	if (!missingfiles.empty())
	{
		OWantFile missing_file = missingfiles.front();
		QuitAndTryDownload(missing_file);
		return;
	}

	// [SL] 2012-12-02 - Force the music to stop when the new map uses
	// the same music lump name that is currently playing. Otherwise,
	// the music from the old wad continues to play...
	S_StopMusic();

	G_InitNew (mapname);

	// [AM] Sync the server's level time with the client.
	::level.time = server_level_time;

	movingsectors.clear();
	teleported_players.clear();

	CL_ClearSectorSnapshots();
	for (Players::iterator it = players.begin();it != players.end();++it)
		it->snapshots.clearSnapshots();

	// reset the world_index (force it to sync)
	CL_ResyncWorldIndex();
	last_svgametic = 0;

	CTF_CheckFlags(consoleplayer());

	gameaction = ga_nothing;

	// Autorecord netdemo or continue recording in a new file
	if (!(netdemo.isPlaying() || netdemo.isRecording() || netdemo.isPaused()))
	{
		std::string filename;

		bool bCanAutorecord = (sv_gametype == GM_COOP && cl_autorecord_coop) 
		|| (IsGameModeFFA() && cl_autorecord_deathmatch)
		|| (IsGameModeDuel() && cl_autorecord_duel)
		|| (sv_gametype == GM_TEAMDM && cl_autorecord_teamdm)
		|| (sv_gametype == GM_CTF && cl_autorecord_ctf);

		size_t param = Args.CheckParm("-netrecord");
		if (param && Args.GetArg(param + 1))
			filename = Args.GetArg(param + 1);

		if (((splitnetdemo || cl_autorecord) && bCanAutorecord) || param)
		{
			if (filename.empty())
				filename = CL_GenerateNetDemoFileName();
			else
				filename = CL_GenerateNetDemoFileName(filename);

			// NOTE(jsd): Presumably a warning is already printed.
			if (filename.empty())
			{
				netdemo.stopRecording();
				return;
			}

			netdemo.startRecording(filename);
		}
	}

	// write the map index to the netdemo
	if (netdemo.isRecording())
		netdemo.writeMapChange();
}

void CL_ResetMap()
{
	// Destroy every actor with a netid that isn't a player.  We're going to
	// get the contents of the map with a full update later on anyway.
	AActor* mo;
	TThinkerIterator<AActor> iterator;
	while ((mo = iterator.Next()))
	{
		if (mo->netid && mo->type != MT_PLAYER)
		{
			mo->Destroy();
		}
	}

	//destroy all moving sector effects and sounds
	for (int i = 0; i < numsectors; i++)
	{
		if (sectors[i].floordata)
		{
			S_StopSound(sectors[i].soundorg);
			sectors[i].floordata->Destroy();
		}

		if (sectors[i].ceilingdata)
		{
			S_StopSound(sectors[i].soundorg);
			sectors[i].ceilingdata->Destroy();
		}
	}

	P_DestroyButtonThinkers();

	// You don't get to keep cards.  This isn't communicated anywhere else.
	if (sv_gametype == GM_COOP)
		P_ClearPlayerCards(consoleplayer());

	// write the map index to the netdemo
	if (netdemo.isRecording() && recv_full_update)
		netdemo.writeMapChange();
}

void CL_EndGame()
{
	Host_EndGame ("\nServer disconnected\n");
}

void CL_FullGame()
{
	Host_EndGame ("Server is full\n");
}

void CL_ExitLevel()
{
	gameaction = ga_completed;

	if (netdemo.isRecording())
		netdemo.writeIntermission();
}

void CL_Noop()
{
	// Nothing to see here. Move along.
}

void CL_Clear()
{
	size_t left = MSG_BytesLeft();
	MSG_ReadChunk(left);
}

// Set local levelstate.
void CL_LevelState()
{
	SerializedLevelState sls;
	sls.state = static_cast<LevelState::States>(MSG_ReadVarint());
	sls.countdown_done_time = MSG_ReadVarint();
	sls.ingame_start_time = MSG_ReadVarint();
	sls.round_number = MSG_ReadVarint();
	sls.last_wininfo_type = static_cast<WinInfo::WinType>(MSG_ReadVarint());
	sls.last_wininfo_id = MSG_ReadVarint();
	::levelstate.unserialize(sls);
}

// Set level locals.
void CL_LevelLocals()
{
	byte flags = MSG_ReadByte();

	if (flags & SVC_LL_TIME)
		::level.time = MSG_ReadVarint();

	if (flags & SVC_LL_TOTALS)
	{
		::level.total_secrets = MSG_ReadVarint();
		::level.total_items = MSG_ReadVarint();
		::level.total_monsters = MSG_ReadVarint();
	}

	if (flags & SVC_LL_SECRETS)
		::level.found_secrets = MSG_ReadVarint();

	if (flags & SVC_LL_ITEMS)
		::level.found_items = MSG_ReadVarint();

	if (flags & SVC_LL_MONSTERS)
		::level.killed_monsters = MSG_ReadVarint();

	if (flags & SVC_LL_MONSTER_RESPAWNS)
		::level.respawned_monsters = MSG_ReadVarint();
}

// client source (once)
typedef void (*client_callback)();
typedef std::map<svc_t, client_callback> cmdmap;
cmdmap cmds;

//
// CL_AllowPackets
//
void CL_InitCommands(void)
{
	cmds[svc_abort]				= &CL_EndGame;
	cmds[svc_loadmap]			= &CL_LoadMap;
	cmds[svc_resetmap]			= &CL_ResetMap;
	cmds[svc_playerinfo]		= &CL_PlayerInfo;
	cmds[svc_consoleplayer]		= &CL_ConsolePlayer;
	cmds[svc_playermembers]		= &CL_PlayerMembers;
	cmds[svc_moveplayer]		= &CL_UpdatePlayer;
	cmds[svc_updatelocalplayer]	= &CL_UpdateLocalPlayer;
	cmds[svc_levellocals]		= &CL_LevelLocals;
	cmds[svc_userinfo]			= &CL_SetupUserInfo;
	cmds[svc_teammembers]		= &CL_TeamMembers;
	cmds[svc_playerstate]		= &CL_UpdatePlayerState;

	cmds[svc_updateping]		= &CL_UpdatePing;
	cmds[svc_spawnmobj]			= &CL_SpawnMobj;
	cmds[svc_mobjspeedangle]	= &CL_SetMobjSpeedAndAngle;
	cmds[svc_mobjinfo]			= &CL_UpdateMobjInfo;
	cmds[svc_explodemissile]	= &CL_ExplodeMissile;
	cmds[svc_removemobj]		= &CL_RemoveMobj;

	cmds[svc_killmobj]			= &CL_KillMobj;
	cmds[svc_movemobj]			= &CL_MoveMobj;
	cmds[svc_damagemobj]		= &CL_DamageMobj;
	cmds[svc_corpse]			= &CL_Corpse;
	cmds[svc_spawnplayer]		= &CL_SpawnPlayer;
//	cmds[svc_spawnhiddenplayer]	= &CL_SpawnHiddenPlayer;
	cmds[svc_damageplayer]		= &CL_DamagePlayer;
	cmds[svc_firepistol]		= &CL_FirePistol;
	cmds[svc_fireweapon]		= &CL_FireWeapon;

	cmds[svc_fireshotgun]		= &CL_FireShotgun;
	cmds[svc_firessg]			= &CL_FireSSG;
	cmds[svc_firechaingun]		= &CL_FireChainGun;
	cmds[svc_changeweapon]		= &CL_ChangeWeapon;
	cmds[svc_railtrail]			= &CL_RailTrail;
	cmds[svc_connectclient]		= &CL_ConnectClient;
	cmds[svc_disconnectclient]	= &CL_DisconnectClient;
	cmds[svc_activateline]		= &CL_ActivateLine;
	cmds[svc_sector]			= &CL_UpdateSector;
	cmds[svc_movingsector]		= &CL_UpdateMovingSector;
	cmds[svc_switch]			= &CL_Switch;
	cmds[svc_print]				= &CL_Print;
    cmds[svc_midprint]          = &CL_MidPrint;
	cmds[svc_say]				= &CL_Say;
    cmds[svc_pingrequest]       = &CL_SendPingReply;
	cmds[svc_svgametic]			= &CL_SaveSvGametic;
	cmds[svc_mobjtranslation]	= &CL_MobjTranslation;
	cmds[svc_inttimeleft]		= &CL_UpdateIntTimeLeft;

	cmds[svc_startsound]		= &CL_Sound;
	cmds[svc_soundorigin]		= &CL_SoundOrigin;
	cmds[svc_mobjstate]			= &CL_SetMobjState;
	cmds[svc_actor_movedir]		= &CL_Actor_Movedir;
	cmds[svc_actor_target]		= &CL_Actor_Target;
	cmds[svc_actor_tracer]		= &CL_Actor_Tracer;
	cmds[svc_missedpacket]		= &CL_CheckMissedPacket;
	cmds[svc_forceteam]			= &CL_ForceSetTeam;

	cmds[svc_ctfevent]			= &CL_CTFEvent;
	cmds[svc_secretevent]		= &CL_SecretEvent;
	cmds[svc_serversettings]	= &CL_GetServerSettings;
	cmds[svc_disconnect]		= &CL_EndGame;
	cmds[svc_full]				= &CL_FullGame;
	cmds[svc_reconnect]			= &CL_Reconnect;
	cmds[svc_exitlevel]			= &CL_ExitLevel;

	cmds[svc_challenge]			= &CL_Clear;
	cmds[svc_launcher_challenge]= &CL_Clear;

	cmds[svc_levelstate]		= &CL_LevelState;

	cmds[svc_touchspecial]      = &CL_TouchSpecialThing;

	cmds[svc_netdemocap]        = &CL_LocalDemoTic;
	cmds[svc_netdemostop]       = &CL_NetDemoStop;
	cmds[svc_netdemoloadsnap]	= &CL_NetDemoLoadSnap;
	cmds[svc_fullupdatedone]	= &CL_FinishedFullUpdate;
	cmds[svc_fullupdatestart]	= &CL_StartFullUpdate;

	cmds[svc_vote_update] = &CL_VoteUpdate;
	cmds[svc_maplist] = &CL_Maplist;
	cmds[svc_maplist_update] = &CL_MaplistUpdate;
	cmds[svc_maplist_index] = &CL_MaplistIndex;

	cmds[svc_playerqueuepos] = &CL_UpdatePlayerQueuePos;
	cmds[svc_executelinespecial] = &CL_ExecuteLineSpecial;
	cmds[svc_executeacsspecial] = &CL_ACSExecuteSpecial;
	cmds[svc_lineupdate] = &CL_LineUpdate;
	cmds[svc_linesideupdate] = &CL_LineSideUpdate;
	cmds[svc_sectorproperties] = &CL_SectorSectorPropertiesUpdate;
	cmds[svc_thinkerupdate] = &CL_ThinkerUpdate;
}

//
// CL_ParseCommands
//
void CL_ParseCommands(void)
{
	std::vector<svc_t>	history;
	svc_t				cmd = svc_abort;

	static bool once = true;
	if(once)CL_InitCommands();
	once = false;

	while(connected)
	{
		size_t byteStart = net_message.BytesRead();

		cmd = (svc_t)MSG_ReadByte();
		history.push_back(cmd);

		if(cmd == (svc_t)-1)
			break;

		cmdmap::iterator i = cmds.find(cmd);
		if(i == cmds.end())
		{
			CL_QuitNetGame();
			Printf(PRINT_HIGH, "CL_ParseCommands: Unknown server message %d following: \n", (int)cmd);

			for(size_t j = 0; j < history.size(); j++)
				Printf(PRINT_HIGH, "CL_ParseCommands: message #%d [%d %s]\n", j, history[j], svc_info[history[j]].getName());
			Printf(PRINT_HIGH, "\n");
			break;
		}

		i->second();

		if (net_message.overflowed)
		{
			CL_QuitNetGame();
			Printf(PRINT_HIGH, "CL_ParseCommands: Bad server message\n");
			Printf(PRINT_HIGH, "CL_ParseCommands: %d(%s) overflowed\n",
					   (int)cmd,
					   svc_info[cmd].getName());
			Printf(PRINT_HIGH, "CL_ParseCommands: It was command number %d in the packet\n",
                                           (int)history.back());
			for(size_t j = 0; j < history.size(); j++)
				Printf(PRINT_HIGH, "CL_ParseCommands: message #%d [%d %s]\n", j, history[j], svc_info[history[j]].getName());
		}

		// Measure length of each message, so we can keep track of bandwidth.
		if (net_message.BytesRead() < byteStart)
			Printf(PRINT_HIGH, "CL_ParseCommands: end byte (%d) < start byte (%d)\n", net_message.BytesRead(), byteStart);

		netgraph.addTrafficIn(net_message.BytesRead() - byteStart);
	}
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

	// GhostlyDeath -- If we are spectating, tell the server of our new position
	if (p->spectator)
	{
		MSG_WriteMarker(&net_buffer, clc_spectate);
		MSG_WriteByte(&net_buffer, 5);
		MSG_WriteLong(&net_buffer, p->mo->x);
		MSG_WriteLong(&net_buffer, p->mo->y);
		MSG_WriteLong(&net_buffer, p->mo->z);
	}

	MSG_WriteMarker(&net_buffer, clc_move);

	// Write current client-tic.  Server later sends this back to client
	// when sending svc_updatelocalplayer so the client knows which ticcmds
	// need to be used for client's positional prediction.
    MSG_WriteLong(&net_buffer, gametic);

	for (int i = 9; i >= 0; i--)
	{
		NetCommand blank_netcmd;
		NetCommand* netcmd;

		if (gametic >= i)
			netcmd = &localcmds[(gametic - i) % MAXSAVETICS];
		else
			netcmd = &blank_netcmd;		// write a blank netcmd since not enough gametics have passed

		netcmd->write(&net_buffer);
	}

	int bytesWritten = NET_SendPacket(net_buffer, serveraddr);
	netgraph.addTrafficOut(bytesWritten);

	outrate += net_buffer.size();
    SZ_Clear(&net_buffer);
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

void CL_LocalDemoTic()
{
	player_t* clientPlayer = &consoleplayer();
	fixed_t x, y, z;
	fixed_t momx, momy, momz;
	fixed_t pitch, viewz, viewheight, deltaviewheight;
	angle_t angle;
	int jumpTics, reactiontime;
	byte waterlevel;

	clientPlayer->cmd.clear();
	clientPlayer->cmd.buttons = MSG_ReadByte();
	clientPlayer->cmd.impulse = MSG_ReadByte();
	clientPlayer->cmd.yaw = MSG_ReadShort();
	clientPlayer->cmd.forwardmove = MSG_ReadShort();
	clientPlayer->cmd.sidemove = MSG_ReadShort();
	clientPlayer->cmd.upmove = MSG_ReadShort();
	clientPlayer->cmd.pitch = MSG_ReadShort();

	waterlevel = MSG_ReadByte();
	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();
	momx = MSG_ReadLong();
	momy = MSG_ReadLong();
	momz = MSG_ReadLong();
	angle = MSG_ReadLong();
	pitch = MSG_ReadLong();
	viewz = MSG_ReadLong();
	viewheight = MSG_ReadLong();
	deltaviewheight = MSG_ReadLong();
	jumpTics = MSG_ReadLong();
	reactiontime = MSG_ReadLong();
	clientPlayer->readyweapon = static_cast<weapontype_t>(MSG_ReadByte());
	clientPlayer->pendingweapon = static_cast<weapontype_t>(MSG_ReadByte());

	if(clientPlayer->mo)
	{
		clientPlayer->mo->x = x;
		clientPlayer->mo->y = y;
		clientPlayer->mo->z = z;
		clientPlayer->mo->momx = momx;
		clientPlayer->mo->momy = momy;
		clientPlayer->mo->momz = momz;
		clientPlayer->mo->angle = angle;
		clientPlayer->mo->pitch = pitch;
		clientPlayer->viewz = viewz;
		clientPlayer->viewheight = viewheight;
		clientPlayer->deltaviewheight = deltaviewheight;
		clientPlayer->jumpTics = jumpTics;
		clientPlayer->mo->reactiontime = reactiontime;
		clientPlayer->mo->waterlevel = waterlevel;
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

void CL_UpdatePlayerQueuePos()
{
	player_t &player = idplayer(MSG_ReadByte());
	byte queuePos = MSG_ReadByte();

	if (player.id == consoleplayer_id)
	{
		if (queuePos > 0 && player.QueuePosition == 0)
		{
			std::ostringstream ss;
			ss << "Position in line to play: " << (int)queuePos << "\n";
			Printf(PRINT_HIGH, ss.str().c_str());
		}
		else if (player.spectator && queuePos == 0 && player.QueuePosition > 0)
		{
			Printf(PRINT_HIGH, "You have been removed from the queue.\n");
		}
	}

	player.QueuePosition = queuePos;
}

void CL_ExecuteLineSpecial()
{
	byte special = MSG_ReadByte();
	uint16_t lineid = MSG_ReadShort();
	AActor* activator = P_FindThingById(MSG_ReadUnVarint());
	int arg0 = MSG_ReadVarint();
	int arg1 = MSG_ReadVarint();
	int arg2 = MSG_ReadVarint();
	int arg3 = MSG_ReadVarint();
	int arg4 = MSG_ReadVarint();

	if (lineid != 0xFFFF && lineid > numlines)
		return;

	line_s* line = NULL;
	if (lineid != 0xFFFF)
		line = &lines[lineid];

	ActivateLine(activator, line, 0, LineACS, special, arg0, arg1, arg2, arg3, arg4);
}

void CL_ACSExecuteSpecial()
{
	byte special = MSG_ReadByte();
	uint32_t netid = MSG_ReadUnVarint();
	byte count = MSG_ReadByte();
	if (count >= 8)
		count = 8;

	static int acsArgs[16];
	for (unsigned int i = 0; i < count; i++)
		acsArgs[i] = MSG_ReadVarint();

	const char* print = MSG_ReadString();

	AActor* activator = P_FindThingById(netid);

	switch (special)
	{
	case DLevelScript::PCD_CLEARINVENTORY:
		DLevelScript::ACS_ClearInventory(activator);
		break;

	case DLevelScript::PCD_SETLINETEXTURE:
		DLevelScript::ACS_SetLineTexture(acsArgs, count);
		break;

	case DLevelScript::PCD_ENDPRINT:
	case DLevelScript::PCD_ENDPRINTBOLD:
		DLevelScript::ACS_Print(special, activator, print);
		break;

	case DLevelScript::PCD_SETMUSIC:
	case DLevelScript::PCD_SETMUSICDIRECT:
	case DLevelScript::PCD_LOCALSETMUSIC:
	case DLevelScript::PCD_LOCALSETMUSICDIRECT:
		DLevelScript::ACS_ChangeMusic(special, activator, acsArgs, count);
		break;

	case DLevelScript::PCD_SECTORSOUND:
	case DLevelScript::PCD_AMBIENTSOUND:
	case DLevelScript::PCD_LOCALAMBIENTSOUND:
	case DLevelScript::PCD_ACTIVATORSOUND:
	case DLevelScript::PCD_THINGSOUND:
		DLevelScript::ACS_StartSound(special, activator, acsArgs, count);
		break;

	case DLevelScript::PCD_SETLINEBLOCKING:
		DLevelScript::ACS_SetLineBlocking(acsArgs, count);
		break;

	case DLevelScript::PCD_SETLINEMONSTERBLOCKING:
		DLevelScript::ACS_SetLineMonsterBlocking(acsArgs, count);
		break;

	case DLevelScript::PCD_SETLINESPECIAL:
		DLevelScript::ACS_SetLineSpecial(acsArgs, count);
		break;

	case DLevelScript::PCD_SETTHINGSPECIAL:
		DLevelScript::ACS_SetThingSpecial(acsArgs, count);
		break;

	case DLevelScript::PCD_FADERANGE:
		DLevelScript::ACS_FadeRange(activator, acsArgs, count);
		break;

	case DLevelScript::PCD_CANCELFADE:
		DLevelScript::ACS_CancelFade(activator);
		break;

	case DLevelScript::PCD_CHANGEFLOOR:
	case DLevelScript::PCD_CHANGECEILING:
		DLevelScript::ACS_ChangeFlat(special, acsArgs, count);
		break;

	case DLevelScript::PCD_SOUNDSEQUENCE:
		DLevelScript::ACS_SoundSequence(acsArgs, count);
		break;

	default:
		Printf(PRINT_HIGH, "Invalid ACS special: %d", special);
		break;
	}
}

void CL_LineUpdate()
{
	uint16_t id = MSG_ReadShort();
	short flags = MSG_ReadShort();
	byte lucency = MSG_ReadByte();

	if (id < numlines)
	{
		line_t* line = &lines[id];
		line->flags = flags;
		line->lucency = lucency;
	}
}

void CL_LineSideUpdate()
{
	uint16_t id = MSG_ReadShort();
	byte side = MSG_ReadByte();
	int changes = MSG_ReadByte();

	side_t* currentSidedef;
	side_t empty;

	if (id < numlines && side < 2 && lines[id].sidenum[side] != R_NOSIDE)
		currentSidedef = sides + lines[id].sidenum[side];
	else
		currentSidedef = &empty;

	for (int i = 0, prop = 1; prop < SDPC_Max; i++)
	{
		prop = 1 << i;
		if ((prop & changes) == 0)
			continue;

		switch (prop)
		{
		case SDPC_TexTop:
			currentSidedef->toptexture = MSG_ReadShort();
			break;
		case SDPC_TexMid:
			currentSidedef->midtexture = MSG_ReadShort();
			break;
		case SDPC_TexBottom:
			currentSidedef->bottomtexture = MSG_ReadShort();
			break;
		default:
			break;
		}
	}
}

void CL_SectorSectorPropertiesUpdate()
{
	uint16_t secnum = MSG_ReadShort();
	int changes = MSG_ReadShort();

	sector_t* sector;
	sector_t empty;

	if (secnum > -1 && secnum < numsectors)
	{
		sector = &sectors[secnum];
	}
	else
	{
		sector = &empty;
		extern dyncolormap_t NormalLight;
		empty.colormap = &NormalLight;
	}

	for (int i = 0, prop = 1; prop < SPC_Max; i++)
	{
		prop = 1 << i;
		if ((prop & changes) == 0)
			continue;

		switch (prop)
		{
		case SPC_FlatPic:
			sector->floorpic = MSG_ReadShort();
			sector->ceilingpic = MSG_ReadShort();
			break;
		case SPC_LightLevel:
			sector->lightlevel = MSG_ReadShort();
			break;
		case SPC_Color:
		{
			byte r = MSG_ReadByte();
			byte g = MSG_ReadByte();
			byte b = MSG_ReadByte();
			sector->colormap = GetSpecialLights(r, g, b,
				sector->colormap->fade.getr(), sector->colormap->fade.getg(), sector->colormap->fade.getb());
		}
			break;
		case SPC_Fade:
		{
			byte r = MSG_ReadByte();
			byte g = MSG_ReadByte();
			byte b = MSG_ReadByte();
			sector->colormap = GetSpecialLights(sector->colormap->color.getr(), sector->colormap->color.getg(), sector->colormap->color.getb(),
				r, g, b);
		}
			break;
		case SPC_Gravity:
			*(int*)&sector->gravity = MSG_ReadLong();
			break;
		case SPC_Panning:
			sector->ceiling_xoffs = MSG_ReadLong();
			sector->ceiling_yoffs = MSG_ReadLong();
			sector->floor_xoffs = MSG_ReadLong();
			sector->floor_yoffs = MSG_ReadLong();
			break;
		case SPC_Scale:
			sector->ceiling_xscale = MSG_ReadLong();
			sector->ceiling_yscale = MSG_ReadLong();
			sector->floor_xscale = MSG_ReadLong();
			sector->floor_yscale = MSG_ReadLong();
			break;
		case SPC_Rotation:
			sector->floor_angle = MSG_ReadLong();
			sector->ceiling_angle = MSG_ReadLong();
			break;
		case SPC_AlignBase:
			sector->base_ceiling_angle = MSG_ReadLong();
			sector->base_ceiling_yoffs = MSG_ReadLong();
			sector->base_floor_angle = MSG_ReadLong();
			sector->base_floor_yoffs = MSG_ReadLong();
		default:
			break;
		}
	}
}

void CL_ThinkerUpdate()
{
	ThinkerType type = (ThinkerType)MSG_ReadByte();

	switch (type)
	{
	case TT_Scroller: {
		DScroller::EScrollType scrollType = (DScroller::EScrollType)MSG_ReadByte();
		fixed_t dx = MSG_ReadLong();
		fixed_t dy = MSG_ReadLong();
		int affectee = MSG_ReadLong();
		if (numsides <= 0 || numsectors <= 0)
			break;
		if (affectee < 0)
			break;
		if (scrollType == DScroller::sc_side && affectee > numsides)
			break;
		if (scrollType != DScroller::sc_side && affectee > numsectors)
			break;

		new DScroller(scrollType, dx, dy, -1, affectee, 0);
	}
	break;
	case TT_FireFlicker: {
		short secnum = MSG_ReadShort();
		int min = MSG_ReadShort();
		int max = MSG_ReadShort();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DFireFlicker(&sectors[secnum], max, min);
	}
	break;
	case TT_Flicker: {
		short secnum = MSG_ReadShort();
		int min = MSG_ReadShort();
		int max = MSG_ReadShort();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DFlicker(&sectors[secnum], max, min);
	}
	break;
	case TT_LightFlash: {
		short secnum = MSG_ReadShort();
		int min = MSG_ReadShort();
		int max = MSG_ReadShort();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DLightFlash(&sectors[secnum], min, max);
	}
	break;
	case TT_Strobe: {
		short secnum = MSG_ReadShort();
		int min = MSG_ReadShort();
		int max = MSG_ReadShort();
		int dark = MSG_ReadShort();
		int bright = MSG_ReadShort();
		int count = MSG_ReadByte();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
		{
			DStrobe* strobe = new DStrobe(&sectors[secnum], max, min, bright, dark);
			strobe->SetCount(count);
		}
	}
	break;
	case TT_Glow: {
		short secnum = MSG_ReadShort();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DGlow(&sectors[secnum]);
	}
	break;
	case TT_Glow2: {
		short secnum = MSG_ReadShort();
		int start = MSG_ReadShort();
		int end = MSG_ReadShort();
		int tics = MSG_ReadShort();
		bool oneShot = MSG_ReadByte();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DGlow2(&sectors[secnum], start, end, tics, oneShot);
	}
	break;
	case TT_Phased: {
		short secnum = MSG_ReadShort();
		int base = MSG_ReadShort();
		int phase = MSG_ReadByte();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DPhased(&sectors[secnum], base, phase);
	}
	break;
	default:
		break;
	}
}

void OnChangedSwitchTexture (line_t *line, int useAgain) {}
void OnActivatedLine (line_t *line, AActor *mo, int side, LineActivationType activationType) {}

VERSION_CONTROL (cl_main_cpp, "$Id$")
