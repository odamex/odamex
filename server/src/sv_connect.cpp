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

#include "sv_connect.h"

#include <set>
#include <sstream>

#include "d_main.h"
#include "g_game.h"
#include "i_net.h"
#include "i_system.h"
#include "md5.h"
#include "p_unlag.h"
#include "sv_banlist.h"
#include "sv_main.h"
#include "sv_sqp.h"
#include "sv_sqpold.h"
#include "svc_message.h"

EXTERN_CVAR(join_password)
EXTERN_CVAR(sv_clientcount)
EXTERN_CVAR(sv_email)
EXTERN_CVAR(sv_maxclients)
EXTERN_CVAR(sv_maxrate)
EXTERN_CVAR(sv_motd)

//------------------------------------------------------------------------------

/**
 * @brief Disconnect an older client using the older protocol.
 */
static void DisconnectOldClient()
{
	int cl_version = MSG_ReadShort();
	byte connection_type = MSG_ReadByte();
	std::string VersionStr;

	int GameVer = 0;
	if (cl_version == VERSION)
	{
		GameVer = MSG_ReadLong();
	}
	else
	{
		// Assume anything older is 0.3.0.
		GameVer = MAKEVER(0, 3, 0);
	}

	int cl_maj, cl_min, cl_pat;
	BREAKVER(GameVer, cl_maj, cl_min, cl_pat);

	std::string msg = VersionMessage(GAMEVER, GameVer, ::sv_email.cstring());
	if (msg.empty())
	{
		// Failsafe.
		StrFormat(msg,
		          "Your version of Odamex does not match the server %s.\nFor updates, "
		          "visit https://odamex.net/\n",
		          DOTVERSIONSTR);
	}

	// Send using the old protocol mechanism without relying on any defines
	const byte old_svc_disconnect = 2;
	const byte old_svc_print = 28;
	const int old_PRINT_HIGH = 2;

	static buf_t smallbuf(1024);

	MSG_WriteLong(&smallbuf, 0);

	MSG_WriteByte(&smallbuf, old_svc_print);
	MSG_WriteByte(&smallbuf, old_PRINT_HIGH);
	MSG_WriteString(&smallbuf, msg.c_str());

	MSG_WriteByte(&smallbuf, old_svc_disconnect);

	NET_SendPacket(smallbuf, ::net_from);

	Printf("%s disconnected (version mismatch %d.%d.%d).\n", NET_AdrToString(::net_from),
	       cl_maj, cl_min, cl_pat);
}

/**
 * @brief Get next free player. Will use the lowest available player id.
 */
static Players::iterator GetFreeClient()
{
	extern std::set<byte> free_player_ids;

	if (players.size() >= sv_maxclients)
		return players.end();

	if (free_player_ids.empty())
	{
		// list of free ids needs to be initialized
		for (int i = 1; i < MAXPLAYERS; i++)
			free_player_ids.insert(i);
	}

	players.emplace_back(player_t{});
	players.back().client.reset(new client_t());
	players.back().playerstate = PST_CONTACT;

	// generate player id
	std::set<byte>::iterator id = free_player_ids.begin();
	players.back().id = *id;
	free_player_ids.erase(id);

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());

	// Return iterator pointing to the just-inserted player
	Players::iterator it = players.end();
	return --it;
}

static bool CheckClientVersion(client_t* cl, Players::iterator it)
{
	int GameVer = 0;
	std::string VersionStr;
	std::string OurVersionStr(DOTVERSIONSTR);
	bool AllowConnect = true;
	int cl_major = 0;
	int cl_minor = 0;
	int cl_patch = 0;
	int sv_major, sv_minor, sv_patch;
	BREAKVER(GAMEVER, sv_major, sv_minor, sv_patch);

	switch (cl->version)
	{
	case VERSION:
		GameVer = MSG_ReadLong();
		BREAKVER(GameVer, cl_major, cl_minor, cl_patch);

		StrFormat(VersionStr, "%d.%d.%d", cl_major, cl_minor, cl_patch);

		cl->packedversion = GameVer;

		// Major and minor versions must be identical, client is allowed
		// to have a newer patch.
		if (VersionCompat(GAMEVER, GameVer) == 0)
			AllowConnect = true;
		else
			AllowConnect = false;
		break;
	case 64:
		VersionStr = "0.2a or 0.3";
		break;
	case 63:
		VersionStr = "Pre-0.2";
		break;
	case 62:
		VersionStr = "0.1a";
		break;
	default:
		VersionStr = "Unknown";
		break;
	}

	// GhostlyDeath -- removes the constant AllowConnects above
	if (cl->version != VERSION)
		AllowConnect = false;

	// GhostlyDeath -- boot em
	if (!AllowConnect)
	{
		std::string msg = VersionMessage(GAMEVER, GameVer, ::sv_email.cstring());
		if (msg.empty())
		{
			// Failsafe.
			StrFormat(
			    msg,
			    "Your version of Odamex does not match the server %s.\nFor updates, "
			    "visit https://odamex.net/\n",
			    DOTVERSIONSTR);
		}

		// GhostlyDeath -- Now we tell them our built up message and boot em
		cl->displaydisconnect = false; // Don't spam the players

		SV_QueueReliable(*cl, SVC_Print(PRINT_WARNING, msg));
		SV_QueueReliable(*cl, SVC_Disconnect());
		SV_SendQueuedPackets(*cl);

		// GhostlyDeath -- And we tell the server
		Printf("%s disconnected (version mismatch %s).\n", NET_AdrToString(::net_from),
		       VersionStr.c_str());
	}

	return AllowConnect;
}

//------------------------------------------------------------------------------

//
//	SV_ConnectClient
//
//	Called when a client connects
//
void SV_ConnectClient()
{
	int challenge = MSG_ReadLong();

	// New querying system
	if (SV_QryParseEnquiry(challenge) == 0)
		return;

	if (challenge == LAUNCHER_CHALLENGE) // for Launcher
	{
		SV_SendServerInfo();
		return;
	}

	if (challenge != PROTO_CHALLENGE && challenge != MSG_CHALLENGE)
		return;

	if (!SV_IsValidToken(MSG_ReadLong()))
		return;

	Printf("%s is trying to connect...\n", NET_AdrToString(net_from));

	// Show old challenges the door only after we've validated their token.
	if (challenge == MSG_CHALLENGE)
	{
		DisconnectOldClient();
		return;
	}

	// find an open slot
	Players::iterator it = GetFreeClient();

	if (it == players.end()) // a server is full
	{
		Printf("%s disconnected (server full).\n", NET_AdrToString(net_from));

		SVCMessages msg;
		buf_t buffer(MAX_UDP_SIZE);
		msg.queueUnreliable(SVC_Disconnect("Server is full\n"));
		msg.writePacket(buffer, I_MSTime(), true);

		NET_SendPacket(buffer, ::net_from);
		return;
	}

	player_t* player = &(*it);
	client_t* cl = player->client.get();

	// clear and reinitialize client network info
	cl->address = net_from;
	cl->last_received = gametic;
	cl->reliable_bps = 0;
	cl->unreliable_bps = 0;
	cl->lastcmdtic = 0;
	cl->lastclientcmdtic = 0;
	cl->allow_rcon = false;
	cl->displaydisconnect = false;
	cl->msg.clear();

	SZ_Clear(&cl->netbuf);
	SZ_Clear(&cl->reliablebuf);

	cl->sequence = 0;
	cl->last_sequence = -1;
	cl->packetnum = 0;

	// generate a random string
	std::stringstream ss;
	ss << time(NULL) << level.time << VERSION << NET_AdrToString(net_from);
	cl->digest = MD5SUM(ss.str());

	// Set player time
	player->JoinTime = time(NULL);

	cl->version = MSG_ReadShort();
	byte connection_type = MSG_ReadByte();

	// [SL] 2011-05-11 - Register the player with the reconciliation system
	// for unlagging
	Unlag::getInstance().registerPlayer(player->id);

	// Check if the client uses the same version as the server.
	if (!CheckClientVersion(cl, it))
	{
		SV_DropClient(*player);
		return;
	}

	// Get the userinfo from the client.
	clc_t userinfo = (clc_t)MSG_ReadByte();
	if (userinfo != clc_userinfo)
	{
		SV_InvalidateClient(*player, "Client didn't send any userinfo");
		return;
	}

	if (!SV_SetupUserInfo(*player))
		return;

	// [SL] Read and ignore deprecated client rate. Clients now always use sv_maxrate.
	MSG_ReadLong();
	cl->rate = int(sv_maxrate);

	// Check if the IP is banned from our list or not.
	if (SV_BanCheck(cl))
	{
		cl->displaydisconnect = false;
		SV_DropClient(*player);
		return;
	}

	// Check if the user entered a good password (if any)
	std::string passhash = MSG_ReadString();
	if (strlen(join_password.cstring()) && MD5SUM(join_password.cstring()) != passhash)
	{
		Printf("%s disconnected (password failed).\n", NET_AdrToString(net_from));

		SV_QueueReliable(
		    *cl,
		    SVC_Print(PRINT_HIGH,
		              "Server is passworded, no password specified or bad password.\n"));
		SV_SendQueuedPackets(*cl);
		SV_DropClient(*player);
		return;
	}

	// send consoleplayer number
	SV_QueueReliable(*cl, SVC_ConsolePlayer(*player, cl->digest));
	if (!SV_SendQueuedPackets(*cl))
	{
		Printf("%s disconnected (queued server packet contains no data).\n",
		       NET_AdrToString(net_from));
		SV_DropClient(*player);
		return;
	}
}

void SV_ConnectClient2(player_t& player)
{
	extern bool step_mode;

	client_t* cl = player.client.get();

	// [AM] FIXME: I don't know if it's safe to set players as PST_ENTER
	//             this early.
	player.playerstate = PST_LIVE;

	// [Toke] send server settings
	SV_SendServerSettings(player);

	cl->displaydisconnect = true;

	SV_BroadcastUserInfo(player);

	// Newly connected players get ENTER state.
	P_ClearPlayerScores(player, SCORES_CLEAR_ALL);
	player.playerstate = PST_ENTER;

	if (!step_mode)
	{
		player.spectator = true;
		for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
		{
			SV_QueueReliable(*pit, SVC_PlayerMembers(player, SVC_PM_SPECTATOR));
		}
	}

	// Send a map name
	SV_QueueReliable(
	    player, SVC_LoadMap(::wadfiles, ::patchfiles, level.mapname.c_str(), level.time));

	// [SL] 2011-12-07 - Force the player to jump to intermission if not in a level
	if (gamestate == GS_INTERMISSION)
	{
		SV_QueueReliable(*cl, odaproto::svc::ExitLevel());
	}

	G_DoReborn(player);
	SV_ClientFullUpdate(player);

	SV_BroadcastPrintf("%s has connected.\n", player.userinfo.netname.c_str());

	// tell others clients about it
	for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
	{
		SV_QueueReliable(*pit, SVC_ConnectClient(player));
	}

	// Notify this player of other player's queue positions
	SV_SendPlayerQueuePositions(&player, true);

	// Send out the server's MOTD.
	SV_MidPrint((char*)sv_motd.cstring(), &player, 6);
}
