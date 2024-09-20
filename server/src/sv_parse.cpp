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
//  Handlers for messages sent from the client.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "sv_parse.h"

#include "client.pb.h"

#include "c_console.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "i_net.h"
#include "i_system.h"
#include "m_cheat.h"
#include "md5.h"
#include "sv_main.h"
#include "sv_maplist.h"
#include "sv_vote.h"
#include "svc_message.h"

EXTERN_CVAR(rcon_password);
EXTERN_CVAR(sv_allowcheats);
EXTERN_CVAR(sv_flooddelay);
EXTERN_CVAR(sv_globalspectatorchat);

//-----------------------------------------------------------------------------

/**
 * Send a message to everybody.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
static void SendSay(player_t& player, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_CHAT, "<CHAT> * %s %s\n", player.userinfo.netname.c_str(),
		       &message[4]);
	else
		Printf(PRINT_CHAT, "<CHAT> %s: %s\n", player.userinfo.netname.c_str(), message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		SV_QueueReliable(*it, SVC_Say(false, player.id, message));
	}
}

/**
 * Send a message to teammates of a player.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
static void SendTeamSay(player_t& player, const char* message)
{
	const char* team = GetTeamInfo(player.userinfo.team)->ColorStringUpper.c_str();

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		bool spectator = it->spectator || !it->ingame();

		// Player needs to be on the same team
		if (spectator || it->userinfo.team != player.userinfo.team)
			continue;

		SV_QueueReliable(*it, SVC_Say(true, player.id, message));
	}
}

/**
 * Send a message to all spectators.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
static void SendSpecSay(player_t& player, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_TEAMCHAT, "<SPEC> * %s %s\n", player.userinfo.netname.c_str(),
		       &message[4]);
	else
		Printf(PRINT_TEAMCHAT, "<SPEC> %s: %s\n", player.userinfo.netname.c_str(),
		       message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		bool spectator = it->spectator || !it->ingame();

		if (!spectator)
			continue;

		SV_QueueReliable(*it, SVC_Say(true, player.id, message));
	}
}

/**
 * Send a message to a specific player from a specific other player.
 *
 * @param player  Sending player.
 * @param dplayer Player to send to.
 * @param message Message to send.
 */
static void SendPrivMsg(player_t& player, player_t& dplayer, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_CHAT, "<PRIVMSG> * %s (to %s) %s\n", player.userinfo.netname.c_str(),
		       dplayer.userinfo.netname.c_str(), &message[4]);
	else
		Printf(PRINT_CHAT, "<PRIVMSG> %s (to %s): %s\n", player.userinfo.netname.c_str(),
		       dplayer.userinfo.netname.c_str(), message);

	SV_QueueReliable(dplayer, SVC_Say(true, player.id, message));

	// [LM] Send a duplicate message to the sender, so he knows the message
	//      went through.
	if (player.id != dplayer.id)
	{
		SV_QueueReliable(player, SVC_Say(true, player.id, message));
	}
}

//-----------------------------------------------------------------------------

static void SV_Disconnect(player_t& player)
{
	odaproto::clc::Disconnect msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	SV_DisconnectClient(player);
}

/**
 * @brief Show a chat string and send it to others clients.
 */
static void SV_Say(player_t& player)
{
	odaproto::clc::Say msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	byte message_visibility = msg.visibility();
	std::string message(msg.message());

	StripColorCodes(message);

	if (!ValidString(message))
	{
		SV_InvalidateClient(player, "Chatstring contains invalid characters");
		return;
	}

	if (message.empty() || message.length() > MAX_CHATSTR_LEN)
		return;

	// Flood protection
	if (player.LastMessage.Time)
	{
		const dtime_t min_diff = I_ConvertTimeFromMs(1000) * sv_flooddelay;
		dtime_t diff = I_GetTime() - player.LastMessage.Time;

		if (diff < min_diff)
			return;

		player.LastMessage.Time = 0;
	}

	if (player.LastMessage.Time == 0)
	{
		player.LastMessage.Time = I_GetTime();
		player.LastMessage.Message = message;
	}

	bool spectator = player.spectator || !player.ingame();

	if (message_visibility == 0)
	{
		if (spectator && !sv_globalspectatorchat)
			SendSpecSay(player, message.c_str());
		else
			SendSay(player, message.c_str());
	}
	else if (message_visibility == 1)
	{
		if (spectator)
			SendSpecSay(player, message.c_str());
		else if (G_IsTeamGame())
			SendTeamSay(player, message.c_str());
		else
			SendSay(player, message.c_str());
	}
}

/**
 * @brief Extracts a player's ticcmd message from their network buffer and
 *        queues the ticcmd for later processing.  The client always sends
 *        its previous ticcmd followed by its current ticcmd just in case
 *        there is a dropped packet.
 */
static void SV_Move(player_t& player)
{
	odaproto::clc::Move msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	client_t* cl = player.client;

	// The client-tic at the time this message was sent.  The server stores
	// this and sends it back the next time it tells the client
	int tic = msg.tic();

	buf_t buffer{MAX_UDP_SIZE};
	buffer.WriteChunk(msg.cmds().data(), msg.cmds().size());

	// Read the last 10 ticcmds from the client and add any new ones
	// to the cmdqueue
	for (int i = 9; i >= 0; i--)
	{
		NetCommand netcmd;
		netcmd.read(&buffer);
		netcmd.setTic(tic - i);

		if (netcmd.getTic() > cl->lastclientcmdtic && gamestate == GS_LEVEL)
		{
			if (!player.spectator)
				player.cmdqueue.push(netcmd);
			cl->lastclientcmdtic = netcmd.getTic();
			cl->lastcmdtic = gametic;
		}
	}
}

static void SV_ClientUserInfo(player_t& who)
{
	if (!SV_SetupUserInfo(who))
		return;

	SV_BroadcastUserInfo(who);
}

/**
 * @brief Calculates ping using gametic which was sent by SV_SendGametic and
 *        current gametic.
 */
static void SV_PingReply(player_t& player)
{
	odaproto::clc::PingReply msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	dtime_t ping = I_MSTime() - msg.ms_time();

	if (ping > 999)
		ping = 999;

	player.ping = ping;
}

static void SV_RCon(player_t& player)
{
	odaproto::clc::RCon msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	std::string str(msg.command());
	StripColorCodes(str);

	if (player.client->allow_rcon)
	{
		Printf(PRINT_HIGH, "RCON command from %s - %s -> %s",
		       player.userinfo.netname.c_str(), NET_AdrToString(net_from), str.c_str());
		AddCommandString(str);
	}
}

/**
 * @author denis
 */
static void SV_RConPassword(player_t& player)
{
	odaproto::clc::RConPassword msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	client_t* cl = player.client;

	if (msg.is_login())
	{
		std::string challenge = msg.passhash();
		std::string password = rcon_password.cstring();

		// Don't display login messages again if the client is already logged in
		if (cl->allow_rcon)
			return;

		if (!password.empty() && MD5SUM(password + cl->digest) == challenge)
		{
			cl->allow_rcon = true;
			Printf(PRINT_HIGH, "RCON login from %s - %s", player.userinfo.netname.c_str(),
			       NET_AdrToString(cl->address));
		}
		else
		{
			Printf(PRINT_HIGH, "RCON login failure from %s - %s",
			       player.userinfo.netname.c_str(), NET_AdrToString(cl->address));
			SV_QueueReliable(*cl, SVC_Print(PRINT_HIGH, "Bad password\n"));
		}
	}
	else
	{
		if (cl->allow_rcon)
		{
			Printf("RCON logout from %s - %s", player.userinfo.netname.c_str(),
			       NET_AdrToString(cl->address));
			cl->allow_rcon = false;
		}
	}
}

static void SV_Spectate(player_t& player)
{
	odaproto::clc::Spectate msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	// [LM] Code has three possible values; true, false and 5.  True specs the
	//      player, false unspecs him and 5 updates the server with the spec's
	//      new position.
	byte Code = byte(msg.command());

	if (!player.ingame())
		return;

	if (Code == 5)
	{
		// GhostlyDeath -- Prevent Cheaters
		if (!player.spectator || !player.mo)
		{
			return;
		}

		// GhostlyDeath -- Code 5! Anyway, this just updates the player for "antiwallhack"
		// fun
		player.mo->x = msg.pos().x();
		player.mo->y = msg.pos().y();
		player.mo->z = msg.pos().z();
	}
	else
	{
		SV_SetPlayerSpec(player, Code);
	}
}

static void SV_Kill(player_t& player)
{
	odaproto::clc::Kill msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	if (player.mo && player.suicidedelay == 0 && gamestate == GS_LEVEL &&
	    (sv_allowcheats || G_IsCoopGame()))
	{
		return;
	}

	if (!player.mo)
		return;

	// WHY do you want to commit suicide in the intermission screen ?!?!
	if (gamestate != GS_LEVEL)
		return;

	// merry suicide!
	P_DamageMobj(player.mo, NULL, NULL, 10000, MOD_SUICIDE);
	// player.mo->player = NULL;
	// player.mo = NULL;
}

//
// SV_Cheat
//
void SV_Cheat(player_t& player)
{
	odaproto::clc::Cheat msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	switch (msg.cheat_type_case())
	{
	case odaproto::clc::Cheat::kCheatNumber: {
		if (!CHEAT_AreCheatsEnabled())
			return;

		int oldCheats = player.cheats;
		CHEAT_DoCheat(&player, msg.cheat_number());

		if (player.cheats != oldCheats)
		{
			for (Players::iterator it = players.begin(); it != players.end(); ++it)
			{
				client_t* cl = it->client;
				SV_SendPlayerStateUpdate(cl, &player);
			}
		}

		break;
	}
	case odaproto::clc::Cheat::kGiveItem: {
		if (!CHEAT_AreCheatsEnabled())
			return;

		CHEAT_GiveTo(&player, msg.give_item().c_str());

		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			client_t* cl = it->client;
			SV_SendPlayerStateUpdate(cl, &player);
		}
		break;
	}
	default:
		break;
	}
}

/**
 * @brief Handle callvote commands from the client.
 */
static void SV_CallVote(player_t& player)
{
	odaproto::clc::CallVote msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	vote_type_t votecmd = vote_type_t(msg.vote_type());
	size_t argc = msg.args().size();

	DPrintf("SV_Callvote: Got votecmd %s from player %d, %d additional arguments.\n",
	        vote_type_cmd[votecmd], player.id, argc);

	std::vector<std::string> arguments(argc);
	for (int i = 0; i < argc; i++)
	{
		arguments[i] = msg.args()[i];
		DPrintf("SV_Callvote: arguments[%d] = \"%s\"\n", i, arguments[i].c_str());
	}

	SV_CallvoteHandler(player, votecmd, arguments);
}

static void SV_MapList(player_t& who)
{
	odaproto::clc::MapList msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(who, "Could not decode message");
		return;
	}

	maplist_status_t status = maplist_status_t(msg.status());
	SV_MaplistHandler(who, status);
}

static void SV_MaplistUpdate(player_t& who)
{
	odaproto::clc::MapListUpdate msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(who, "Could not decode message");
		return;
	}

	SV_MaplistUpdateHandler(who);
}

static void SV_GetPlayerInfo(player_t& who)
{
	odaproto::clc::GetPlayerInfo msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(who, "Could not decode message");
		return;
	}

	SV_SendPlayerInfo(who);
}

/**
 * @brief Interpret a "netcmd" string from a client.
 *
 * @param player Player who sent the netcmd.
 */
void SV_NetCmd(player_t& player)
{
	odaproto::clc::NetCmd msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	if (msg.args_size() == 0)
	{
		return;
	}

	std::vector<std::string> netargs;
	netargs.reserve(size_t(msg.args_size()));
	for (int i = 0; i < msg.args_size(); i++)
	{
		netargs.push_back(msg.args().at(i));
	}

	if (netargs.at(0) == "help")
	{
		SV_HelpCmd(player);
	}
	else if (netargs.at(0) == "motd")
	{
		SV_MOTDCmd(player);
	}
	else if (netargs.at(0) == "ready")
	{
		SV_ReadyCmd(player);
	}
	else if (netargs.at(0) == "vote")
	{
		SV_VoteCmd(player, netargs);
	}
}

static void SV_SpyPlayer(player_t& viewer)
{
	odaproto::clc::Spy msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(viewer, "Could not decode message");
		return;
	}

	byte id = byte(msg.pid());

	player_t& other = idplayer(id);
	if (!validplayer(other) || !P_CanSpy(viewer, other))
		return;

	viewer.spying = id;
	SV_SendPlayerStateUpdate(viewer.client, &other);
}

/**
 * @brief Show a chat string and show it to a single other client.
 */
static void SV_PrivMsg(player_t& player)
{
	odaproto::clc::PrivMsg msg;
	if (!MSG_ReadProto(msg))
	{
		SV_InvalidateClient(player, "Could not decode message");
		return;
	}

	player_t& dplayer = idplayer(msg.pid());

	std::string str(msg.message());
	StripColorCodes(str);

	if (!ValidString(str))
	{
		SV_InvalidateClient(player, "Private Message contains invalid characters");
		return;
	}

	if (!validplayer(dplayer))
		return;

	if (str.empty() || str.length() > MAX_CHATSTR_LEN)
		return;

	// In competitive gamemodes, don't allow spectators to message players.
	if (!G_IsCoopGame() && player.spectator && !dplayer.spectator)
		return;

	// Flood protection
	if (player.LastMessage.Time)
	{
		const dtime_t min_diff = I_ConvertTimeFromMs(1000) * sv_flooddelay;
		dtime_t diff = I_GetTime() - player.LastMessage.Time;

		if (diff < min_diff)
			return;

		player.LastMessage.Time = 0;
	}

	if (player.LastMessage.Time == 0)
	{
		player.LastMessage.Time = I_GetTime();
		player.LastMessage.Message = str;
	}

	SendPrivMsg(player, dplayer, str.c_str());
}

//-----------------------------------------------------------------------------

void SV_ParseMessages(player_t& who)
{
	while (validplayer(who))
	{
		clc_t cmd = (clc_t)MSG_ReadByte();

		if (cmd == (clc_t)-1)
			break;

#define CL_MSG(header, func) \
	case header:             \
		func(who);           \
		break

		switch (cmd)
		{
			CL_MSG(clc_disconnect, SV_Disconnect);
			CL_MSG(clc_say, SV_Say);
			CL_MSG(clc_move, SV_Move);
			CL_MSG(clc_userinfo, SV_ClientUserInfo);
			CL_MSG(clc_pingreply, SV_PingReply);
			CL_MSG(clc_ack, SV_AcknowledgePacket);
			CL_MSG(clc_rcon, SV_RCon);
			CL_MSG(clc_rcon_password, SV_RConPassword);
			CL_MSG(clc_spectate, SV_Spectate);
			CL_MSG(clc_kill, SV_Kill);
			CL_MSG(clc_cheat, SV_Cheat);
			CL_MSG(clc_callvote, SV_CallVote);
			CL_MSG(clc_maplist, SV_MapList);
			CL_MSG(clc_maplist_update, SV_MaplistUpdate);
			CL_MSG(clc_getplayerinfo, SV_GetPlayerInfo);
			CL_MSG(clc_netcmd, SV_NetCmd);
			CL_MSG(clc_spy, SV_SpyPlayer);
			CL_MSG(clc_privmsg, SV_PrivMsg);

		default:
			Printf("SV_ParseCommands: Unknown client message %d.\n", (int)cmd);
			SV_DropClient(who);
			return;
		}

		if (net_message.overflowed)
		{
			Printf("SV_ReadClientMessage: badread %d(%s)\n", (int)cmd,
			       clc_info[cmd].getName());
			SV_DropClient(who);
			return;
		}
	}
}
