// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//     Clientside handling of incoming packets.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "cl_connection.h"

#include "cl_connect.h"
#include "cl_main.h"
#include "cl_parse.h"
#include "cl_state.h"
#include "clc_message.h"
#include "d_event.h"
#include "d_main.h"
#include "g_game.h"
#include "w_wad.h"

//------------------------------------------------------------------------------

EXTERN_CVAR(sv_downloadsites)
EXTERN_CVAR(cl_forcedownload)

extern int netin;
extern int netout;
extern int outrate;
extern bool recv_full_update;
extern std::string server_host;

//------------------------------------------------------------------------------

/**
 * @brief Handle connection process.
 */
static void TickConnecting()
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
		CL_HandleServerInfo();
	}
	else if (type == 0)
	{
		if (!CL_HandleFirstPacket())
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

/**
 * @brief Start the actual connection.
 *
 * @param dwServerToken Server token to connect with.
 */
static void TryToConnect(uint32_t dwServerToken)
{
	if (!ClientState::get().isConnecting())
		return;

	if (ClientState::get().canRetryConnect())
	{
		Printf("Joining server...\n");

		SZ_Clear(&write_buffer);
		MSG_WriteLong(&write_buffer, PROTO_CHALLENGE); // send challenge
		MSG_WriteLong(&write_buffer, dwServerToken);   // confirm server token
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

/**
 * @brief Tick the network game while connected.
 *
 * @return True if we should continue, false if we need to early-return.
 */
static bool TickConnected()
{
	static int realrate = 0;

	for (;;)
	{
		const int packet_size = NET_GetPacket(net_message, net_from);
		if (packet_size == 0)
			break;

		// denis - don't accept candy from strangers
		if (!NET_CompareAdr(ClientState::get().getAddress(), net_from))
			break;

		realrate += packet_size;
		last_received = gametic;
		noservermsgs = false;

		if (!CL_ReadPacketHeader())
			continue;

		if (netdemo.isRecording())
			netdemo.capture(&net_message);

		if (!CL_ReadAndParseMessages())
			return false;

		if (gameaction == ga_fullconsole) // Host_EndGame was called
			return false;
	}

	if (!(gametic % TICRATE))
	{
		netin = realrate;
		realrate = 0;
	}

	CL_SaveCmd(); // save console commands
	if (!noservermsgs)
		CL_SendCmd(); // send console commands to the server
	else
		CL_KeepAlive(); // send acks to keep the connection alive.

	if (!(gametic % TICRATE))
	{
		netout = outrate;
		outrate = 0;
	}

	if (gametic - last_received > 65)
		noservermsgs = true;

	return true;
}

//------------------------------------------------------------------------------

bool CL_HandleFirstPacket()
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

//------------------------------------------------------------------------------

bool CL_HandleServerInfo()
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

//------------------------------------------------------------------------------

void CL_HandleIncomingPackets()
{
	if (!ClientState::get().isConnected())
	{
		TickConnecting();
	}
	else
	{
		if (!TickConnected())
			return;
	}
}
