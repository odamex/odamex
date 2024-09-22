// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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
//  Serverside connection sequence.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "cl_connect.h"

#include "am_map.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "cl_download.h"
#include "cl_main.h"
#include "cl_parse.h"
#include "cl_state.h"
#include "cl_replay.h"
#include "clc_message.h"
#include "d_main.h"
#include "g_game.h"
#include "p_mobj.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"

EXTERN_CVAR(cl_downloadsites)
EXTERN_CVAR(cl_forcedownload)
EXTERN_CVAR(cl_serverdownload)
EXTERN_CVAR(debug_disconnect)
EXTERN_CVAR(mute_enemies)
EXTERN_CVAR(mute_spectators)
EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_allowjump)
EXTERN_CVAR(sv_allowredscreen)
EXTERN_CVAR(sv_downloadsites)
EXTERN_CVAR(sv_freelook)

bool forcenetdemosplit = false; // need to split demo due to svc_reconnect
bool recv_full_update = false;
std::string server_host = ""; // hostname of server

//------------------------------------------------------------------------------

void CL_Reconnect()
{
	recv_full_update = false;

	if (netdemo.isRecording())
		forcenetdemosplit = true;

	ClientReplay::getInstance().reset();

	if (ClientState::get().isConnected())
	{
		MSG_WriteCLC(&write_buffer, odaproto::clc::Disconnect{});
		NET_SendPacket(write_buffer, ClientState::get().getAddress());
		SZ_Clear(&write_buffer);
		gameaction = ga_fullconsole;

		P_ClearAllNetIds();
	}

	ClientState::get().onDisconnect();
	simulated_connection =
	    false; // Ch0wW : don't block people connect to a server after playing a demo

	if (!ClientState::get().startReconnect())
	{
		PrintFmt("Could not reconnect, no prior address.\n");
	}
}

void CL_QuitNetGame2(const netQuitReason_e reason, const char* file, const int line)
{
	if (ClientState::get().isConnected())
	{
		SZ_Clear(&write_buffer);
		MSG_WriteCLC(&write_buffer, odaproto::clc::Disconnect{});
		NET_SendPacket(write_buffer, ClientState::get().getAddress());
		SZ_Clear(&write_buffer);
		sv_gametype = GM_COOP;
		ClientReplay::getInstance().reset();
	}

	if (paused)
	{
		paused = false;
		S_ResumeSound();
	}

	ClientState::get().onDisconnect();
	gameaction = ga_fullconsole;
	noservermsgs = false;
	AM_Stop();

	serverside = clientside = true;
	network_game = false;
	simulated_connection =
	    false; // Ch0wW : don't block people connect to a server after playing a demo

	sv_freelook = 1;
	sv_allowjump = 1;
	sv_allowexit = 1;
	sv_allowredscreen = 1;

	mute_spectators = 0.f;
	mute_enemies = 0.f;

	P_ClearAllNetIds();

	{
		// [jsd] unlink player pointers from AActors; solves crash in R_ProjectSprites
		// after a svc_disconnect message.
		for (Players::iterator it = players.begin(); it != players.end(); it++)
		{
			player_s& player = *it;
			if (player.mo)
			{
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
		Printf("Disconnected from server\n");
		break;
	case NQ_ABORT:
		Printf("Connection attempt aborted\n");
		break;
	case NQ_PROTO:
		Printf("Disconnected from server: Unrecoverable protocol error\n");
		break;
	}

	if (::debug_disconnect)
		Printf("  (%s:%d)\n", file, line);
}

void CL_QuitAndTryDownload(const OWantFile& missing_file)
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
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	if (!cl_serverdownload)
	{
		// Downloading is disabled client-side
		Printf(PRINT_WARNING,
		       "Unable to find \"%s\". Downloading is disabled on your client.  Go to "
		       "Options > Network Options to enable downloading.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	if (netdemo.isPlaying())
	{
		// Playing a netdemo and unable to download from the server
		Printf(PRINT_WARNING,
		       "Unable to find \"%s\".  Cannot download while playing a netdemo.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame(NQ_DISCONNECT);
		return;
	}

	if (sv_downloadsites.str().empty() && cl_downloadsites.str().empty())
	{
		// Nobody has any download sites configured.
		Printf("Unable to find \"%s\".  Both your client and the server have no "
		       "download sites configured.\n",
		       missing_file.getBasename().c_str());
		CL_QuitNetGame(NQ_DISCONNECT);
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
	CL_QuitNetGame(NQ_SILENT);

	// Start the download.
	CL_StartDownload(downloadsites, missing_file, DL_RECONNECT);
}

//------------------------------------------------------------------------------

BEGIN_COMMAND(connect)
{
	if (argc <= 1)
	{
		Printf("Usage: connect ip[:port] [password]\n");
		Printf("\n");
		Printf("Connect to a server, with optional port number");
		Printf(" and/or password\n");
		Printf("eg: connect 127.0.0.1\n");
		Printf("eg: connect 192.168.0.1:12345 secretpass\n");

		return;
	}

	// Ch0wW : don't block people connect to a server after playing a demo
	::simulated_connection = false;
	C_FullConsole();
	gamestate = GS_CONNECTING;

	CL_QuitNetGame(NQ_SILENT);

	bool ok = false;
	if (argc > 2)
	{
		ok = ClientState::get().startConnect(argv[1], argv[2]);
	}
	else
	{
		ok = ClientState::get().startConnect(argv[1]);
	}

	if (!ok)
	{
		PrintFmt("Could not resolve host \"{}\"\n", argv[1]);
		return;
	}
}
END_COMMAND(connect)

BEGIN_COMMAND(reconnect)
{
	CL_Reconnect();
}
END_COMMAND(reconnect)

BEGIN_COMMAND(disconnect)
{
	CL_QuitNetGame(NQ_SILENT);
}
END_COMMAND(disconnect)
