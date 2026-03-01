// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2026 by The Odamex Team.
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
#include "r_interp.h"
#include "cl_demo.h"
#include "cl_download.h"
#include "cl_maplist.h"
#include "cl_vote.h"
#include "p_mobj.h"
#include "p_snapshot.h"
#include "p_lnspec.h"
#include "cl_netgraph.h"
#include "p_pspr.h"
#include "clc_message.h"
#include "g_levelstate.h"
#include "v_text.h"
#include "hu_stuff.h"
#include "p_acs.h"
#include "i_input.h"
#include "i_time.h"

#include "g_gametype.h"
#include "cl_parse.h"
#include "cl_replay.h"

#include "m_consolecommandstream.h"

#include "OdaMessenger.h"
#include "CanarySocket.h"

#include <bitset>
#include <set>
#include <sstream>

#include "server.pb.h"

#if _MSC_VER == 1310
#pragma optimize("",off)
#endif

#ifndef _WIN32
#   include <netinet/in.h>
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

bool      noservermsgs;
int       last_received;

// [SL] 2012-03-17 - world_index is the gametic on the server that the client
// is currently simulating.  world_index_accum is a continuous accumulator that
// is used to advance world_index if appropriate.
int       world_index = 0;
float     world_index_accum = 0.0f;

int       last_svgametic = 0;
int       last_player_update = 0;

bool      recv_full_update = false;

std::string connectpasshash = "";

bool      connected;
netadr_t  serveraddr; // address of a server
netadr_t  lastconaddr;

extern NetGraph netgraph;

OdaMessenger messenger;
static std::unique_ptr<CanarySocketClient> s_canary;

// denis - unique session key provided by the server
std::string digest;

std::string server_host = "";	// hostname of server

// [SL] 2011-06-27 - Class to record and playback network recordings
NetDemo netdemo;
// [SL] 2011-07-06 - not really connected (playing back a netdemo)
bool forcenetdemosplit = false;		// need to split demo due to svc_reconnect

odaproto::clc::PlayerInput localcmds[MAXSAVETICS];

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
EXTERN_CVAR(debug_disconnect)

static argb_t enemycolor, teamcolor;

void P_PlayerLeavesGame(player_t* player);

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
argb_t CL_GetPlayerColor(const player_t& player)
{
	argb_t base_color(255, player.userinfo.color[1], player.userinfo.color[2], player.userinfo.color[3]);
	argb_t shade_color = base_color;

	bool teammate = false;
	if (G_IsCoopGame())
		teammate = true;
	if (G_IsFFAGame())
		teammate = false;
	if (G_IsTeamGame())
	{
		teammate = P_AreTeammates(consoleplayer(), player);
		base_color = GetTeamInfo(player.userinfo.team)->Color;
	}
	if (player.id != consoleplayer_id && !consoleplayer().spectator)
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

	for (auto& player : players)
		R_BuildPlayerTranslation(player.id, CL_GetPlayerColor(player));
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
void CL_TryToConnect(DWORD server_token);

bool M_FindFreeName(std::string &filename, const std::string &extension);

void CL_SimulateWorld();

// some doom functions (not csDoom)
void D_Display(void);
void D_DoAdvanceDemo(void);
void M_Ticker(void);

size_t P_NumPlayersInGame();
void G_PlayerReborn (player_t &player);
void P_KillMobj (AActor *source, AActor *target, const AActor *inflictor, bool joinkill);
void P_SetPsprite (player_t& player, int position, int32_t stnum);
void P_ExplodeMissile (AActor* mo);
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
	static constexpr float CORRECTION_PERIOD = 1.0f / 16.0f;

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
    PrintFmt("{}", msg);
	CL_QuitNetGame(NQ_SILENT);
}

void CL_QuitNetGame2(const netQuitReason_e reason, const char* file, const int line)
{
	if(connected)
	{
        CL_CompleteDisconnect(reason);

		sv_gametype = GM_COOP;
		ClientReplay::getInstance().reset();
	}

	if (paused)
	{
		paused = false;
		S_ResumeMusic();
	}

	memset (&serveraddr, 0, sizeof(serveraddr));
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

	{
		// [jsd] unlink player pointers from AActors; solves crash in R_ProjectSprites after a svc_disconnect message.
		for (auto& player : players) {
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

	demoplayback = false;

	// Reset the palette to default
	V_ResetPalette();

	cvar_t::C_RestoreCVars();

	switch (reason)
	{
	default: // Also NQ_SILENT
		break;
	case NQ_DISCONNECT:
		PrintFmt("Disconnected from server\n");
		break;
	case NQ_ABORT:
		PrintFmt("Connection attempt aborted\n");
		break;
	case NQ_PROTO:
		PrintFmt("Disconnected from server: Unrecoverable protocol error\n");
		break;
	}

	if (::debug_disconnect)
		PrintFmt("  ({}:{})\n", file, line);
}

void CL_CompleteDisconnect(netQuitReason_e reason)
{
    const dtime_t oneTicInNanosec = static_cast<dtime_t>(1000000000.0 / static_cast<double>(TICRATE));

	if (connected and reason != NQ_SERVER_DROP)
	{
		messenger.Clear();

        // Again, make sure that we allow for immediate retransmits.
        messenger.SetRetransmitDelay(0);

		MSG_WriteMarker(&messenger.ReliableBuf().Obtain(), clc_disconnect);
		messenger.SendAll(gametic, serveraddr);

        const dtime_t disconnectStartTime   = I_GetTime();
        const dtime_t disconnectTimeoutTime = disconnectStartTime + I_ConvertTimeFromMs(2000);

        while (connected and I_GetTime() < disconnectTimeoutTime)
        {
            I_Sleep(oneTicInNanosec);

            messenger.HandleRetransmissions(gametic, serveraddr);
            if (NET_GetPacket())
            {
                messenger.Receive(::net_message);
            }

            if (messenger.NextReceivedPacket(::net_message))
            {
                CL_ParseCommands();
            }

            if (connected == false)
            {
                // We received the DisconnectClient message that announced our departure.
                // Do one final send to make sure the server gets our Ack.
                messenger.SendAll(gametic, serveraddr);
            }
        }

        if (connected)
        {
            PrintFmt(PRINT_WARNING, "Server did not acknowledge the disconnection\n");
        }
    }

    connected = false;

    messenger = OdaMessenger();
    P_ClearAllNetIds();
    s_canary.reset();
	gameaction = ga_fullconsole;
}

void CL_Reconnect(void)
{
	recv_full_update = false;

	if (netdemo.isRecording())
		forcenetdemosplit = true;

	ClientReplay::getInstance().reset();

	if (connected)
	{
        CL_CompleteDisconnect(NQ_SILENT);
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
		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_spy);
		MSG_WriteByte(&netBuf, newid);
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

			P_FriendlyEffects(); // Mark any new friendly monsters with an effect

			return;
		}
	} while (it != sentinal);
}

extern bool advancedemo;
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

		OInterpolation::getInstance().ticGameInterpolation();

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
	const std::string cmd = M_ConsoleInput();
	if (cmd.length())
		AddCommandString(cmd);

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
				PrintFmt("level.time {}, prndindex {}, {} {} {}\n",
				         level.time, prndindex, players.begin()->mo->x, players.begin()->mo->y, players.begin()->mo->z);
			else
 				PrintFmt("level.time %d, prndindex %d\n", level.time, prndindex);
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
	    PrintFmt("Usage: connect ip[:port] [password]\n");
	    PrintFmt("\n");
	    PrintFmt("Connect to a server, with optional port number");
	    PrintFmt(" and/or password\n");
	    PrintFmt("eg: connect 127.0.0.1\n");
	    PrintFmt("eg: connect 192.168.0.1:12345 secretpass\n");

	    return;
	}

	simulated_connection = false;	// Ch0wW : don't block people connect to a server after playing a demo

	C_FullConsole();
	gamestate = GS_CONNECTING;

	CL_QuitNetGame(NQ_SILENT);

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
			PrintFmt("Could not resolve host {}\n", target);
			memset(&serveraddr, 0, sizeof(serveraddr));
		}
	}

	connecttimeout = 0;
}
END_COMMAND (connect)


BEGIN_COMMAND (disconnect)
{
	CL_QuitNetGame(NQ_SILENT);
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
	for (const auto& player : players) {
		if (player.ingame()) {
			mplayers[player.id] = player.userinfo.netname;
		}
	}

	// Print them, ordered by player id.
	PrintFmt("PLAYERS IN GAME:\n");
	for (const auto& [id, name] : mplayers) {
		PrintFmt("{:>3d}. {}\n", id, name);
	}
	PrintFmt("{} {}\n", mplayers.size(), mplayers.size() == 1 ? "PLAYER" : "PLAYERS");
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
			PrintFmt("Bad player number\n");
			return;
		}
		else
			player = &p;
	}

	if (!validplayer(*player))
	{
		PrintFmt("Not a valid player\n");
		return;
	}

	const std::string color = fmt::format("#{:02X}{:02X}{:02X}",
		player->userinfo.color[1], player->userinfo.color[2], player->userinfo.color[3]);

	PrintFmt(PRINT_HIGH, "---------------[player info]----------- \n");
	PrintFmt(PRINT_HIGH, " userinfo.netname - {:s} \n",		player->userinfo.netname);

	if (sv_gametype == GM_CTF || sv_gametype == GM_TEAMDM) {
		PrintFmt(PRINT_HIGH, " userinfo.team    - {:s} \n",
		       GetTeamInfo(player->userinfo.team)->ColorizedTeamName());
	}
	PrintFmt(PRINT_HIGH, " userinfo.aimdist - {:d} \n",		player->userinfo.aimdist >> FRACBITS);
	PrintFmt(PRINT_HIGH, " userinfo.color   - {:s} \n",		color);
	PrintFmt(PRINT_HIGH, " userinfo.gender  - {:d} \n",		player->userinfo.gender);
	PrintFmt(PRINT_HIGH, " time             - {:d} \n",		player->GameTime);
	PrintFmt(PRINT_HIGH, " spectator        - {:d} \n",		player->spectator);
	PrintFmt(PRINT_HIGH, " downloader       - {:d} \n",		player->playerstate == PST_DOWNLOAD);
	PrintFmt(PRINT_HIGH, "--------------------------------------- \n");
}
END_COMMAND (playerinfo)


BEGIN_COMMAND (kill)
{
    if (sv_allowcheats || G_IsCoopGame())
        MSG_WriteMarker(&messenger.NetBuf().Obtain(), clc_kill);
    else
        PrintFmt("You must run the server with '+set sv_allowcheats 1' or disable sv_keepkeys to enable this command.\n");
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
            size_t FieldLength = Cvar->name().length();

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
    PrintFmt("\n{1:>{0}} - Value\n", MaxFieldLength, "Name");

    // Data
	for (const auto& varname : server_cvars)
	{
		cvar_t *dummy;
		Cvar = cvar_t::FindCVar(varname.c_str(), &dummy);

		PrintFmt("{1:>{0}} - {2}\n",
			     MaxFieldLength,
			     Cvar->name(),
			     Cvar->str());
	}

    PrintFmt("\n");
}
END_COMMAND (serverinfo)


BEGIN_COMMAND (rcon)
{
	if (connected && argc > 1)
	{
		char  command[256];

		strncpy(command, args, ARRAY_LENGTH(command) - 1);
		command[255] = '\0';

		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_rcon);
		MSG_WriteString(&netBuf, command);
	}
}
END_COMMAND (rcon)


BEGIN_COMMAND (rcon_password)
{
	if (connected && argc > 1)
	{
		bool login = true;

		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_rcon_password);
		MSG_WriteByte(&netBuf, login);

		std::string password = argv[1];
		MSG_WriteString(&netBuf, MD5SUM(password + digest).c_str());
	}
}
END_COMMAND (rcon_password)

BEGIN_COMMAND (rcon_logout)
{
	if (connected)
	{
		bool login = false;

		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_rcon_password);
		MSG_WriteByte(&netBuf, login);
		MSG_WriteString(&netBuf, "");
	}
}
END_COMMAND (rcon_logout)


BEGIN_COMMAND (playerteam)
{
	if (G_IsTeamGame())
		PrintFmt("Your are in the {} team.\n", V_GetTeamColor(consoleplayer().userinfo.team));
	else
		PrintFmt("You need to play a team-based gamemode in order to use this command.\n");
}
END_COMMAND (playerteam)

BEGIN_COMMAND (changeteams)
{
	int iTeam = (int)consoleplayer().userinfo.team;
	iTeam = (iTeam + 1) % sv_teamsinplay.asInt();
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
		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_spectate);
		MSG_WriteByte(&netBuf, true);
	}
}
END_COMMAND (spectate)

BEGIN_COMMAND(ready)
{
	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_netcmd);
	MSG_WriteString(&netBuf, "ready");
	MSG_WriteByte(&netBuf, 0);
}
END_COMMAND(ready)

static void NetCmdHelp()
{
	PrintFmt(PRINT_HIGH,
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

	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_netcmd);
	MSG_WriteString(&netBuf, argv[1]);

	// Pass additional arguments as separate strings.  Avoids argument
	// parsing at the opposite end.
	byte netargc = MIN<size_t>(argc - 2, 0xFF);
	MSG_WriteByte(&netBuf, netargc);
	for (size_t i = 0; i < netargc; i++)
	{
		MSG_WriteString(&netBuf, argv[i + 2]);
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

	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_spectate);
	MSG_WriteByte(&netBuf, false);
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
			PrintFmt(PRINT_HIGH, "Unfollowing player '{}'.\n", spyplayername);

			// revert to not spying:
			displayplayer_id = consoleplayer_id;
		} else {
			PrintFmt(PRINT_HIGH, "Expecting player name.  Try 'players' to list all player names.\n");
		}

		// clear last player name:
		spyplayername = "";
	} else {
		// remember player name in case of disconnect/reconnect e.g. level change:
		spyplayername = argv[1];

		PrintFmt(PRINT_HIGH, "Following player '{}'. Use 'spy' with no player name to unfollow.\n",
			     spyplayername);
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
	if (var.str().empty())
		var.RestoreDefault();
}

EXTERN_CVAR(cl_netdemodir)

//
// CL_GenerateNetDemoFileName
//
//
std::string CL_GenerateNetDemoFileName(const std::string &filename = cl_netdemoname.str())
{
	const std::string expanded_filename(M_ExpandTokens(filename));
	std::string newfilename(expanded_filename);
	newfilename = M_GetNetDemoFileName(newfilename, cl_netdemodir);

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
		found = M_GetNetDemoFileName(filename, cl_netdemodir);
		if (found.empty())
		{
			PrintFmt(PRINT_WARNING, "Could not find demo {}.\n", filename);
			return;
		}
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
		PrintFmt(PRINT_HIGH, "Already recording a netdemo.  Please stop recording before "\
		         "beginning a new netdemo recording.\n");
		return;
	}

	if (!connected || simulated_connection)
	{
		PrintFmt(PRINT_HIGH, "You must be connected to a server to record a netdemo.\n");
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
		PrintFmt(PRINT_HIGH, "Demo resumed.\n");
	}
	else if (netdemo.isPlaying())
	{
		netdemo.pause();
		paused = true;
		PrintFmt(PRINT_HIGH, "Demo paused.\n");
	}
}
END_COMMAND(netpause)

BEGIN_COMMAND(netplay)
{
	if(argc <= 1)
	{
		PrintFmt(PRINT_HIGH, "Usage: netplay <demoname>\n");
		return;
	}

	if (!connected)
	{
 		G_CheckDemoStatus();	// cleans up vanilla demo or single player game
	}

	CL_QuitNetGame(NQ_SILENT);
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

	PrintFmt(PRINT_HIGH, "\n{}\n", netdemo.getFileName());
	PrintFmt(PRINT_HIGH, "============================================\n");
	PrintFmt(PRINT_HIGH, "Total time: {} seconds\n", totaltime);
	PrintFmt(PRINT_HIGH, "Current position: {} seconds ({}%)\n",
		curtime, curtime * 100 / totaltime);
	PrintFmt(PRINT_HIGH, "Number of maps: {}\n", maptimes.size());
	for (size_t i = 0; i < maptimes.size(); i++)
	{
		PrintFmt(PRINT_HIGH, "> {:02d} Starting time: {} seconds\n",
			i + 1, maptimes[i]);
	}
}
END_COMMAND(netdemostats)

BEGIN_COMMAND(netff)
{
	if (netdemo.isPlaying())
		netdemo.nextSnapshot();
	else if (netdemo.isPaused())
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
void CL_SendUserInfo(buf_t& netBuf)
{
	UserInfo* coninfo = &consoleplayer().userinfo;
	D_SetupUserInfo();

	MSG_WriteMarker	(&netBuf, clc_userinfo);
	MSG_WriteString	(&netBuf, coninfo->netname.c_str());
	MSG_WriteByte	(&netBuf, coninfo->team); // [Toke]
	MSG_WriteLong	(&netBuf, coninfo->gender);

	for (int i = 3; i >= 0; i--)
		MSG_WriteByte(&netBuf, coninfo->color[i]);

	// [SL] place holder for deprecated skins
	MSG_WriteString	(&netBuf, "");

	MSG_WriteLong	(&netBuf, coninfo->aimdist);
	MSG_WriteBool	(&netBuf, true);	// [SL] deprecated "cl_unlag" CVAR
	MSG_WriteBool	(&netBuf, coninfo->predict_weapons);
	MSG_WriteByte	(&netBuf, (char)coninfo->switchweapon);
	for (const auto& pref : coninfo->weapon_prefs)
	{
		MSG_WriteByte (&netBuf, pref);
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

		p = &players.emplace_back();
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

		P_FriendlyEffects(); // Mark any new friendly monsters with an effect

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
		R_BuildPlayerTranslation(player.id, CL_GetPlayerColor(player));
	}

	P_ClearPlayerPowerups(player);	// Remove all current powerups

	// GhostlyDeath -- If the player matches our display player...
	CL_CheckDisplayPlayer();
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

		PrintFmt(PRINT_HIGH, "Connecting to {}...\n", NET_AdrToString(serveraddr));

		buf_t netBuf {MAX_UDP_PACKET};
		MSG_WriteLong(&netBuf, LAUNCHER_CHALLENGE);
		NET_SendPacket(netBuf, serveraddr);
	}

	connecttimeout--;
}

/**
 * @brief Quit the network game while attempting to download a file.
 *
 * @param missing_file Missing file to attempt to download.
 */
void CL_QuitAndTryDownload(const OWantFile& missing_file)
{
	// Need to set this here, otherwise we render a frame of wild pointers
	// filled with garbage data.
	gamestate = GS_FULLCONSOLE;

	if (missing_file.getBasename().empty())
	{
		PrintFmt(PRINT_WARNING,
		         "Tried to download an empty file.  This is probably a bug "
		         "in the client where an empty file is considered missing.\n");
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	if (!cl_serverdownload)
	{
		// Downloading is disabled client-side
		PrintFmt(PRINT_WARNING,
		         "Unable to find \"{}\". Downloading is disabled on your client.  Go to "
		         "Options > Network Options to enable downloading.\n",
		         missing_file.getBasename());
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	if (netdemo.isPlaying())
	{
		// Playing a netdemo and unable to download from the server
		PrintFmt(PRINT_WARNING,
		         "Unable to find \"{}\".  Cannot download while playing a netdemo.\n",
		         missing_file.getBasename());
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	if (sv_downloadsites.str().empty() && cl_downloadsites.str().empty())
	{
		// Nobody has any download sites configured.
		PrintFmt("Unable to find \"{}\".  Both your client and the server have no "
		         "download sites configured.\n",
		         missing_file.getBasename());
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	// Gather our server and client sites.
	StringTokens serversites = TokenizeString(sv_downloadsites.str(), " ");
	StringTokens clientsites = TokenizeString(cl_downloadsites.str(), " ");

	// Shuffle the sites so we evenly distribute our requests.
	std::shuffle(serversites.begin(), serversites.end(), rng);
	std::shuffle(clientsites.begin(), clientsites.end(), rng);

	// Combine them into one big site list.
	Websites downloadsites;
	downloadsites.reserve(serversites.size() + clientsites.size());
	downloadsites.insert(downloadsites.end(), serversites.begin(), serversites.end());
	downloadsites.insert(downloadsites.end(), clientsites.begin(), clientsites.end());

	// Disconnect from the server before we start the download.
	PrintFmt(PRINT_HIGH, "Need to download \"{}\", disconnecting from server...\n",
	         missing_file.getBasename());
	CL_QuitNetGame(NQ_SILENT);

	// Start the download.
	CL_StartDownload(downloadsites, missing_file, DL_RECONNECT);
}

//
// [denis] CL_PrepareConnect
// Process server info and switch to the right wads...
//
bool CL_PrepareConnect()
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

	PrintFmt("Found server at {}.\n\n", NET_AdrToString(::serveraddr));
	PrintFmt("> Hostname: {}\n", server_host);

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
		const std::string hashStr = MSG_ReadString();
		OMD5Hash hash;
		OMD5Hash::makeFromHexStr(hash, hashStr);
		if (!OWantFile::makeWithHash(file, newwadnames.at(i), OFILE_WAD, hash))
		{
			PrintFmt(PRINT_WARNING,
			         "Could not construct wanted file \"{}\" that server requested.\n",
			         newwadnames.at(i));
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}

		PrintFmt("> {}\n   {}\n", file.getBasename(),
		         file.getWantedMD5().getHexStr());
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

	PrintFmt("> Map: {}\n", server_map);

	version = MSG_ReadShort();
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

		int major, minor, patch;
		BREAKVER(gameversion, major, minor, patch);
		PrintFmt(PRINT_HIGH, "> Server Version {}.{}.{}\n", major, minor, patch);

		std::string msg = VersionMessage(::gameversion, GAMEVER, NULL);
		if (!msg.empty())
		{
			PrintFmt(PRINT_WARNING, "{}", msg);
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}
	}
	else
	{
		// [AM] Not worth sorting out what version it actually is.
		std::string msg = VersionMessage(MAKEVER(0, 3, 0), GAMEVER, NULL);
		PrintFmt(PRINT_WARNING, "{}", msg);
		CL_QuitNetGame(NQ_ABORT);
		return false;
	}

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
			PrintFmt(PRINT_WARNING,
			         "Could not construct wanted file \"{}\" that server requested.\n",
			         filename);
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}

		PrintFmt("> {}\n", file.getBasename());
	}

	// TODO: Allow deh/bex file downloads
	PrintFmt("\n");
	bool ok = D_DoomWadReboot(newwadfiles, newpatchfiles);
	if (!ok && missingfiles.empty())
	{
		PrintFmt(PRINT_WARNING, "Could not load required set of WAD files.\n");
		CL_QuitNetGame(NQ_ABORT);
		return false;
	}
	else if ((!ok && !missingfiles.empty()) || cl_forcedownload)
	{
		if (::missingCommercialIWAD)
		{
			PrintFmt(PRINT_WARNING,
			         "Server requires commercial IWAD that was not found.\n");
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}

		OWantFile missing_file;
		if (missingfiles.empty())				// cl_forcedownload
		{
			missing_file = newwadfiles.back();
		}
		else									// client is really missing a file
		{
			missing_file = missingfiles.front();
		}

		CL_QuitAndTryDownload(missing_file);
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
bool CL_Connect()
{
	players.clear();

	connected = true;
	multiplayer = true;
	network_game = true;
	serverside = false;
	simulated_connection = netdemo.isPlaying();

	if (not simulated_connection)
	{
		sockaddr_in tcpAddress;
		sockaddr_in udpAddress;

		NetadrToSockadr(&serveraddr, &tcpAddress);

		NET_GetSockaddr(udpAddress);

		s_canary = std::make_unique<CanarySocketClient>();
		s_canary->Connect(tcpAddress, udpAddress);
	}

	messenger = OdaMessenger();
	messenger.SetMaxRate(20);               // FIXME: total guess.
	messenger.SetPacketsPerRetransmit(10);  // To align with the size of the traditional cmd buffer.
    messenger.SetRetransmitDelay(0);        // This causes an immediate retransmit to relieve the risk of
                                            // packet loss on commands from the client.  Reliability comes
                                            // at the cost of _potential_ additionald latency, and the
                                            // slight increase in packets/tic is worth latency mitigation...
	// Rewind!
	// CL_Connect is only called after we already know that the sequence is 0, so we can just let
	// the messenger do its thing.
	::net_message.SeekRead(0, buf_t::BT_START);

	if (messenger.Receive(::net_message) == MessageResultEnum::ABORT)
	{
		CL_QuitNetGame(NQ_PROTO);
	}
	else
	{
		PrintFmt("Requesting server state...\n");
		messenger.NextReceivedPacket(::net_message);
		CL_ParseCommands();
	}

	messenger.SendAll(gametic, ::serveraddr);

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
		PrintFmt(PRINT_HIGH, "using alternate port {}\n", localport);
    }
    else
		localport = CLIENTPORT;

    // set up a socket and net_message buffer
    InitNetCommon();

    messenger.Clear();

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

		PrintFmt("Joining server...\n");

		messenger.Clear();

		// The following is part of the connection sequence that doesn't play
		// nicely with the rest of the messaging...  This is why we do direct
		// unmanaged packet sends.
		//
		buf_t netBuf {MAX_UDP_PACKET};
		MSG_WriteLong(&netBuf, PROTO_CHALLENGE); // send challenge
		MSG_WriteLong(&netBuf, server_token); // confirm server token
		MSG_WriteShort(&netBuf, version); // send client version
		MSG_WriteByte(&netBuf, 0); // send type of connection (play/spectate/rcon/download)

		// GhostlyDeath -- Send more version info
		if (gameversiontosend)
			MSG_WriteLong(&netBuf, gameversiontosend);
		else
			MSG_WriteLong(&netBuf, GAMEVER);

		CL_SendUserInfo(netBuf); // send userinfo

		// [SL] The "rate" CVAR has been deprecated. Now just send a hard-coded
		// maximum rate that the server will ignore.
		constexpr int rate = 0xFFFF;
		MSG_WriteLong(&netBuf, rate);

		MSG_WriteString(&netBuf, connectpasshash.c_str());

		NET_SendPacket(netBuf, serveraddr);
	}

	connecttimeout--;
}

//
// CL_PlayerJustTeleported
//
// Returns true if we have received a svc_activateline message from the server
// involving this player and teleportation
//
bool CL_PlayerJustTeleported(const player_t& player)
{
	if (teleported_players.find(player.id) != teleported_players.end())
		return true;

	return false;
}

//
// CL_ClearPlayerJustTeleported
//
void CL_ClearPlayerJustTeleported(const player_t& player)
{
	teleported_players.erase(player.id);
}

ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, bool dropped);

//
// CL_ClearSectorSnapshots
//
// Removes all sector snapshots at the start of a map, etc
//
void CL_ClearSectorSnapshots()
{
	sector_snaps.clear();
}

/**
 * @brief Read the header of the packet and prepare the rest of it for reading.
 *
 * @return False if the packet was set aside for reliability sequencing, otherwise true.
 */
MessageResultEnum CL_ReadPacketHeader()
{
	::netgraph.addTrafficIn(::net_message.size());
	return ::messenger.Receive(::net_message);
}

// Returns true if all is good, false if we need to bail out of further processing.
MessageResultEnum CL_AcceptNetMessage()
{
	if (::messenger.NextReceivedPacket(::net_message))
	{
		if (netdemo.isRecording())
		{
			netdemo.capture(&::net_message);
		}

		CL_ParseCommands();

		if (gameaction == ga_fullconsole) // Host_EndGame was called
		{
			return MessageResultEnum::ABORT;
		}
		return MessageResultEnum::ACCEPT;
	}
	return MessageResultEnum::DEFER;
}

MessageResultEnum CL_ProcessCurrentReliableMessages()
{
	auto result = CL_AcceptNetMessage();
	while (result == MessageResultEnum::ACCEPT)
	{
		result = CL_AcceptNetMessage();
	}
	return result;
}


void CL_Clear()
{
	size_t left = MSG_BytesLeft();
	MSG_ReadChunk(left);
}

static std::string SVCName(byte header)
{
	std::string svc = ::svc_info[header].getName();
	if (svc.empty())
	{
		svc = fmt::sprintf("svc_%u", header);
	}
	return svc;
}

//
// CL_ParseCommands
//
void CL_ParseCommands()
{
	while (connected)
	{
		if (::net_message.BytesLeftToRead() == 0)
		{
			break;
		}

		size_t byteStart = ::net_message.BytesRead();
		parseError_e res = CL_ParseCommand();
		if (res != PERR_OK || ::net_message.overflowed)
		{
			const Protos& protos = CL_GetTicProtos();

			std::string err;
			if (res == PERR_UNKNOWN_HEADER)
			{
				err = "Unknown message header";
			}
			else if (res == PERR_UNKNOWN_MESSAGE)
			{
				err = "Message is not known to message decoder";
			}
			else if (res == PERR_BAD_DECODE)
			{
				err = "Could not decode message";
			}
			else if (::net_message.overflowed)
			{
				err = "Message overflowed";
			}
			else
			{
				err = "Unknown error";
			}

			if (!protos.empty())
			{
				PrintFmt(PRINT_WARNING, "CL_ParseCommands: {}\n", err);

				for (Protos::const_iterator it = protos.begin(); it != protos.end(); ++it)
				{
					char latest = (it == protos.end() - 1) ? '>' : ' ';
					ptrdiff_t idx = it - protos.begin() + 1;
					std::string svc = SVCName(it->header);
					size_t siz = it->size;
					PrintFmt(PRINT_WARNING, "{:c} {:>2d} [{}] {}b\n", latest, idx, svc,
					         siz);
				}
			}
			else
			{
				PrintFmt(PRINT_WARNING, "CL_ParseCommands: {}\n", err);
			}

			CL_QuitNetGame(NQ_PROTO);
		}

		// Measure length of each message, so we can keep track of bandwidth.
		if (::net_message.BytesRead() < byteStart)
		{
			PrintFmt("CL_ParseCommands: end byte ({}) < start byte ({})\n",
			         ::net_message.BytesRead(), byteStart);
		}
	}
}


void CL_SaveCmd(void)
{
	odaproto::clc::PlayerInput& netcmd = localcmds[gametic % MAXSAVETICS];
	CLC_PackPlayerInputMessageFromPlayer(netcmd, consoleplayer());
	netcmd.set_tic(gametic);
	netcmd.set_world_index(world_index);
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
		buf_t& netBuf = messenger.NetBuf().Obtain();

		MSG_WriteMarker(&netBuf, clc_spectate);
		MSG_WriteByte(&netBuf, 5);
		MSG_WriteLong(&netBuf, p->mo->x);
		MSG_WriteLong(&netBuf, p->mo->y);
		MSG_WriteLong(&netBuf, p->mo->z);
	}

	odaproto::clc::PlayerInput& currentNetcmd = localcmds[gametic % MAXSAVETICS];

	// Write current client-tic.  Server later sends this back to client
	// when sending svc_updatelocalplayer so the client knows which ticcmds
	// need to be used for client's positional prediction.
	currentNetcmd.set_tic(gametic);
	MSG_WriteSVC(messenger.ReliableBuf(), currentNetcmd);

	messenger.SendAll(gametic, serveraddr);

	const int retransmittedByteCount = messenger.HandleRetransmissions(gametic, serveraddr);

	const int currentSendSize    = messenger.GetLastSendSize();
	const int totalSentByteCount = currentSendSize + retransmittedByteCount;

	netgraph.setReliableNonContiguousRetransmits(messenger.GetNonContiguousRetransmitPackets());
	netgraph.setReliableSendDepth(messenger.GetPendingAckCount());
	netgraph.addTrafficOut(totalSentByteCount);
	outrate += totalSentByteCount;
}

//
// CL_PlayerTimes
//
void CL_PlayerTimes()
{
	for (auto& player : players)
	{
		if (player.ingame())
			player.GameTime++;
	}
}

//
// CL_SendCheat
//
void CL_SendCheat(int cheats)
{
	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_cheat);
	MSG_WriteByte(&netBuf, 0);
	MSG_WriteShort(&netBuf, cheats);
}

//
// CL_SendCheat
//
void CL_SendGiveCheat(const char* item)
{
	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_cheat);
	MSG_WriteByte(&netBuf, 1);
	MSG_WriteString(&netBuf, item);
}

//
// CL_SendSummonCheat
//
void CL_SendSummonCheat(const char* summon)
{
	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_cheat);
	MSG_WriteByte(&netBuf, 2);
	MSG_WriteString(&netBuf, summon);
}

//
// CL_SendSummonFriendCheat
//
void CL_SendSummonFriendCheat(const char* summon)
{
	buf_t& netBuf = messenger.NetBuf().Obtain();
	MSG_WriteMarker(&netBuf, clc_cheat);
	MSG_WriteByte(&netBuf, 3);
	MSG_WriteString(&netBuf, summon);
}


void PickupMessage (const AActor *toucher, const char *message)
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
		PrintFmt(PRINT_PICKUP, "{}\n", message);
	}
}

//
// void WeaponPickupMessage (weapontype_t &Weapon)
//
// This is used for displaying weaponstay messages, it is inevitably a hack
// because weaponstay is a hack
void WeaponPickupMessage (const AActor *toucher, const weapontype_t &Weapon)
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
	for (const auto& [sectornum, snapmanager] : sector_snaps)
	{
		if (sectornum >= numsectors)
			continue;

		sector_t *sector = &sectors[sectornum];

		// will this sector be handled when predicting sectors?
		if (cl_predictsectors && CL_SectorIsPredicting(sector))
			continue;

		// Fetch the snapshot for this world_index and run the sector's
		// thinkers to play any sector sounds
		SectorSnapshot snap = snapmanager.getSnapshot(world_index);
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
	for (auto& player : players)
	{
		if (!player.mo || player.spectator)
			continue;

		// Consoleplayer is handled in CL_PredictWorld
		if (player.id == consoleplayer_id)
			continue;

		PlayerSnapshot snap = player.snapshots.getSnapshot(world_index);
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
				player.mo->prevx = player.mo->x;
				player.mo->prevy = player.mo->y;
				player.mo->prevz = player.mo->z;
				player.mo->prevangle = player.mo->angle;
				player.mo->prevpitch = player.mo->pitch;

				PlayerSnapshot prevsnap = player.snapshots.getSnapshot(world_index - 1);

				v3fixed_t offset;
				M_SetVec3Fixed(&offset, prevsnap.getX() - player.mo->x,
										prevsnap.getY() - player.mo->y,
										prevsnap.getZ() - player.mo->z);

				fixed_t dist = M_LengthVec3Fixed(&offset);
				if (dist > 2 * FRACUNIT)
				{
					#ifdef _SNAPSHOT_DEBUG_
					PrintFmt(PRINT_HIGH, "Snapshot {}, Correcting extrapolation error of {}\n",
							 world_index, dist >> FRACBITS);
					#endif	// _SNAPSHOT_DEBUG_

					static constexpr fixed_t correction_amount = FRACUNIT * 0.80f;
					M_ScaleVec3Fixed(&offset, &offset, correction_amount);

					// Apply a smoothing offset to the current snapshot
					snap.setX(snap.getX() - offset.x);
					snap.setY(snap.getY() - offset.y);
					snap.setZ(snap.getZ() - offset.z);
				}
			}

			int oldframe = player.mo->frame;
			snap.toPlayer(player);

			if (player.playerstate != PST_LIVE)
				player.mo->frame = oldframe;

			if (!snap.isContinuous())
			{
				// [SL] Save the position after to the new update so this position
				// won't be interpolated.
				player.mo->prevx = player.mo->x;
				player.mo->prevy = player.mo->y;
				player.mo->prevz = player.mo->z;
				player.mo->prevangle = player.mo->angle;
				player.mo->prevpitch = player.mo->pitch;
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
	static constexpr int MAX_BEHIND = 16;
	static constexpr int MAX_AHEAD = 16;

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

		PrintFmt(PRINT_HIGH, "Gametic {}, world_index {}, Resynching world index ({}).\n",
			gametic, world_index, reason);
		#endif // _WORLD_INDEX_DEBUG_

		CL_ResyncWorldIndex();
	}

	// Not using interpolation?  Use the last update always
	if (!cl_interp)
		world_index = last_svgametic;

	#ifdef _WORLD_INDEX_DEBUG_
	PrintFmt(PRINT_HIGH, "Gametic {}, simulating world_index {}\n",
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
		PrintFmt(PRINT_HIGH, "Gametic {}, increasing world index by {}.\n",
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
