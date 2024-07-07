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

static void TryToConnect(DWORD server_token)
{
	if (!ClientState::get().isConnecting())
		return;

	if (ClientState::get().canRetryConnect())
	{
		Printf("Joining server...\n");

		SZ_Clear(&write_buffer);
		MSG_WriteLong(&write_buffer, PROTO_CHALLENGE); // send challenge
		MSG_WriteLong(&write_buffer, server_token);    // confirm server token
		MSG_WriteShort(&write_buffer, version);        // send client version
		MSG_WriteByte(&write_buffer,
		              0); // send type of connection (play/spectate/rcon/download)

		// GhostlyDeath -- Send more version info
		if (gameversiontosend)
			MSG_WriteLong(&write_buffer, gameversiontosend);
		else
			MSG_WriteLong(&write_buffer, GAMEVER);

		CL_SendUserInfo(); // send userinfo

		// [SL] The "rate" CVAR has been deprecated. Now just send a hard-coded
		// maximum rate that the server will ignore.
		const int rate = 0xFFFF;
		MSG_WriteLong(&write_buffer, rate);
		MSG_WriteString(&write_buffer, ClientState::get().getPasswordHash().c_str());

		NET_SendPacket(write_buffer, ClientState::get().getAddress());
		SZ_Clear(&write_buffer);

		ClientState::get().onSentConnect();
	}
}

//------------------------------------------------------------------------------

void CL_RequestConnectInfo()
{
	if (!ClientState::get().isConnecting())
		return;

	::gamestate = GS_CONNECTING;

	if (ClientState::get().canRetryConnect())
	{
		PrintFmt("Connecting to {}...\n",
		         NET_AdrToString(ClientState::get().getAddress()));

		SZ_Clear(&write_buffer);
		MSG_WriteLong(&write_buffer, LAUNCHER_CHALLENGE);
		NET_SendPacket(write_buffer, ClientState::get().getAddress());

		ClientState::get().onSentConnect();
	}
}

bool CL_PrepareConnect()
{
	G_CleanupDemo(); // stop demos from playing before D_DoomWadReboot wipes out Zone
	                 // memory

	cvar_t::C_BackupCVars(CVAR_SERVERINFO);

	DWORD server_token = MSG_ReadLong();
	server_host = MSG_ReadString();

	bool recv_teamplay_stats = 0;
	gameversiontosend = 0;

	byte playercount = MSG_ReadByte(); // players
	MSG_ReadByte();                    // max_players

	std::string server_map = MSG_ReadString();
	byte server_wads = MSG_ReadByte();

	Printf("Found server at %s.\n\n", NET_AdrToString(ClientState::get().getAddress()));
	Printf("> Hostname: %s\n", server_host.c_str());

	std::vector<std::string> newwadnames;
	newwadnames.reserve(server_wads);
	for (byte i = 0; i < server_wads; i++)
	{
		newwadnames.push_back(MSG_ReadString());
	}

	MSG_ReadBool();                        // deathmatch
	MSG_ReadByte();                        // skill
	recv_teamplay_stats |= MSG_ReadBool(); // teamplay
	recv_teamplay_stats |= MSG_ReadBool(); // ctf

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
			Printf(PRINT_WARNING,
			       "Could not construct wanted file \"%s\" that server requested.\n",
			       newwadnames.at(i).c_str());
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}

		Printf("> %s\n   %s\n", file.getBasename().c_str(),
		       file.getWantedMD5().getHexCStr());
	}

	// Download website - needed for HTTP downloading to work.
	sv_downloadsites.Set(MSG_ReadString());

	// Receive conditional teamplay information
	if (recv_teamplay_stats)
	{
		MSG_ReadLong();

		for (size_t i = 0; i < NUMTEAMS; i++)
		{
			bool enabled = MSG_ReadBool();

			if (enabled)
				MSG_ReadLong();
		}
	}

	Printf("> Map: %s\n", server_map.c_str());

	version = MSG_ReadShort();
	if (version > VERSION)
		version = VERSION;
	if (version < 62)
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
		Printf(PRINT_HIGH, "> Server Version %i.%i.%i\n", major, minor, patch);

		std::string msg = VersionMessage(::gameversion, GAMEVER, NULL);
		if (!msg.empty())
		{
			Printf(PRINT_WARNING, "%s", msg.c_str());
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}
	}
	else
	{
		// [AM] Not worth sorting out what version it actually is.
		std::string msg = VersionMessage(MAKEVER(0, 3, 0), GAMEVER, NULL);
		Printf(PRINT_WARNING, "%s", msg.c_str());
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
			Printf(PRINT_WARNING,
			       "Could not construct wanted file \"%s\" that server requested.\n",
			       filename.c_str());
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}

		Printf("> %s\n", file.getBasename().c_str());
	}

	// TODO: Allow deh/bex file downloads
	Printf("\n");
	bool ok = D_DoomWadReboot(newwadfiles, newpatchfiles);
	if (!ok && missingfiles.empty())
	{
		Printf(PRINT_WARNING, "Could not load required set of WAD files.\n");
		CL_QuitNetGame(NQ_ABORT);
		return false;
	}
	else if (!ok && !missingfiles.empty() || cl_forcedownload)
	{
		if (::missingCommercialIWAD)
		{
			Printf(PRINT_WARNING,
			       "Server requires commercial IWAD that was not found.\n");
			CL_QuitNetGame(NQ_ABORT);
			return false;
		}

		OWantFile missing_file;
		if (missingfiles.empty()) // cl_forcedownload
		{
			missing_file = newwadfiles.back();
		}
		else // client is really missing a file
		{
			missing_file = missingfiles.front();
		}

		CL_QuitAndTryDownload(missing_file);
		return false;
	}

	recv_full_update = false;

	ClientState::get().onGotServerInfo();
	TryToConnect(server_token);
	return true;
}

bool CL_Connect()
{
	players.clear();

	CL_ClearMessages();
	ClientState::get().onConnected();

	multiplayer = true;
	network_game = true;
	serverside = false;
	simulated_connection = netdemo.isPlaying();

	MSG_SetOffset(0, buf_t::BT_SSET);
	if (!CL_ReadPacketHeader())
		return false;

	// Parsing the header means we acked the first packet, send that off.
	MSG_WriteCLC(&write_buffer, CLC_Ack(ClientState::get().getRecentAck(),
	                                    ClientState::get().getAckBits()));
	NET_SendPacket(write_buffer, ClientState::get().getAddress());
	SZ_Clear(&write_buffer);

	if (!CL_ReadAndParseMessages())
		return false;

	if (gameaction == ga_fullconsole) // Host_EndGame was called
		return false;

	D_SetupUserInfo();

	// raise the weapon
	if (validplayer(consoleplayer()))
		consoleplayer().psprites[ps_weapon].sy = 32 * FRACUNIT + 0x6000;

	noservermsgs = false;
	last_received = gametic;

	gamestate = GS_CONNECTED;

	return true;
}

void CL_TickConnecting()
{
	if (!NET_GetPacket(::net_message, ::net_from))
		return;

	// denis - don't accept candy from strangers
	if (gamestate != GS_CONNECTING || !ClientState::get().isValidAddress(net_from))
		return;

	if (netdemo.isRecording())
		netdemo.capture(&net_message);

	int type = MSG_ReadLong();

	if (type == MSG_CHALLENGE)
	{
		CL_PrepareConnect();
	}
	else if (type == 0)
	{
		if (!CL_Connect())
		{
			ClientState::get().onDisconnect();
		}
	}
	else
	{
		// we are already connected to this server, quit first
		MSG_WriteCLC(&write_buffer, odaproto::clc::Disconnect{});
		NET_SendPacket(write_buffer, ClientState::get().getAddress());

		Printf(PRINT_WARNING,
		       "Got unknown challenge %d while connecting, disconnecting.\n", type);
	}
}

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
