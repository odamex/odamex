// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	SV_MAIN
//
//-----------------------------------------------------------------------------

#include "win32inc.h"
#ifdef _WIN32
    #include <winsock.h>
    #include <time.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#endif

#include "doomtype.h"
#include "doomstat.h"
#include "gstrings.h"
#include "d_player.h"
#include "s_sound.h"
#include "gi.h"
#include "d_net.h"
#include "g_game.h"
#include "g_level.h"
#include "p_tick.h"
#include "p_local.h"
#include "p_inter.h"
#include "sv_main.h"
#include "sv_sqp.h"
#include "sv_sqpold.h"
#include "sv_master.h"
#include "i_system.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_random.h"
#include "m_vectors.h"
#include "p_ctf.h"
#include "w_wad.h"
#include "w_ident.h"
#include "md5.h"
#include "p_mobj.h"
#include "p_unlag.h"
#include "sv_vote.h"
#include "sv_maplist.h"
#include "g_warmup.h"
#include "sv_banlist.h"
#include "d_main.h"

#include <algorithm>
#include <sstream>
#include <vector>

extern void G_DeferedInitNew (char *mapname);
extern level_locals_t level;

// Unnatural Level Progression.  True if we've used 'map' or another command
// to switch to a specific map out of order, otherwise false.
bool unnatural_level_progression;

// denis - game manipulation, but no fancy gfx
bool clientside = false, serverside = true;
bool predicting = false;
baseapp_t baseapp = server;

// [SL] 2011-07-06 - not really connected (playing back a netdemo)
// really only used clientside
bool        simulated_connection = false;

extern bool HasBehavior;
extern int mapchange;

bool step_mode = false;

std::queue<byte> free_player_ids;

// General server settings
EXTERN_CVAR(sv_motd)
EXTERN_CVAR(sv_hostname)
EXTERN_CVAR(sv_email)
EXTERN_CVAR(sv_website)
EXTERN_CVAR(sv_waddownload)
EXTERN_CVAR(sv_maxrate)
EXTERN_CVAR(sv_emptyreset)
EXTERN_CVAR(sv_emptyfreeze)
EXTERN_CVAR(sv_clientcount)
EXTERN_CVAR(sv_globalspectatorchat)
EXTERN_CVAR(sv_allowtargetnames)
EXTERN_CVAR(sv_flooddelay)
EXTERN_CVAR(sv_ticbuffer)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(sv_warmup_overtime_enable)
EXTERN_CVAR(sv_warmup_overtime)

void SexMessage (const char *from, char *to, int gender,
	const char *victim, const char *killer);
Players::iterator SV_RemoveDisconnectedPlayer(Players::iterator it);

CVAR_FUNC_IMPL (sv_maxclients)
{
	// Describes the max number of clients that are allowed to connect.
	int count = var.asInt();
	Players::iterator it = players.begin();
	while (it != players.end())
	{
		if (count <= 0)
		{
			MSG_WriteMarker(&(it->client.reliablebuf), svc_print);
			MSG_WriteByte(&(it->client.reliablebuf), PRINT_CHAT);
			MSG_WriteString(&(it->client.reliablebuf),
			                "Client limit reduced. Please try connecting again later.\n");
			SV_DropClient(*it);
			it = SV_RemoveDisconnectedPlayer(it);
		}
		else
		{
			count -= 1;
			++it;
		}
	}
}


CVAR_FUNC_IMPL (sv_maxplayers)
{
	// [Nes] - Force extras to become spectators.
	int normalcount = 0;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		bool spectator = it->spectator || !it->ingame();

		if (!spectator)
		{
			normalcount++;

			if (normalcount > var)
			{
				for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
				{
					MSG_WriteMarker(&pit->client.reliablebuf, svc_spectate);
					MSG_WriteByte(&pit->client.reliablebuf, it->id);
					MSG_WriteByte(&pit->client.reliablebuf, true);
				}
				SV_BroadcastPrintf (PRINT_HIGH, "%s became a spectator.\n", it->userinfo.netname.c_str());
				MSG_WriteMarker(&it->client.reliablebuf, svc_print);
				MSG_WriteByte(&it->client.reliablebuf, PRINT_CHAT);
				MSG_WriteString(&it->client.reliablebuf,
								"Active player limit reduced. You are now a spectator!\n");
				it->spectator = true;
				it->playerstate = PST_LIVE;
				it->joinafterspectatortime = level.time;
			}
		}
	}
}

// [AM] - Force extras on a team to become spectators.
CVAR_FUNC_IMPL (sv_maxplayersperteam)
{
	for (int i = 0; i < NUMTEAMS;i++)
	{
		int normalcount = 0;
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			bool spectator = it->spectator || !it->ingame();
			if (it->userinfo.team == i && it->ingame() && !spectator)
			{
				normalcount++;

				if (normalcount > var)
				{
					SV_SetPlayerSpec(*it, true);
					SV_PlayerPrintf(it->id, PRINT_HIGH, "Active player limit reduced. You are now a spectator!\n");
				}
			}
		}
	}
}

EXTERN_CVAR (sv_allowcheats)
EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_intermissionlimit)
EXTERN_CVAR (sv_maxcorpses)

EXTERN_CVAR (sv_weaponstay)
EXTERN_CVAR (sv_itemsrespawn)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_nomonsters)

// Action rules
EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_fragexitswitch)
EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (sv_infiniteammo)
EXTERN_CVAR (sv_keepkeys)

// Teamplay/CTF
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_friendlyfire)

// Private server settings
CVAR_FUNC_IMPL (join_password)
{
	if (strlen(var.cstring()))
		Printf(PRINT_HIGH, "join password set");
	else
		Printf(PRINT_HIGH, "join password cleared");
}

CVAR_FUNC_IMPL (rcon_password) // Remote console password.
{
	if(strlen(var.cstring()) < 5)
	{
		if(!strlen(var.cstring()))
			Printf(PRINT_HIGH, "rcon password cleared");
		else
		{
			Printf(PRINT_HIGH, "rcon password must be at least 5 characters");
			var.Set("");
		}
	}
	else
		Printf(PRINT_HIGH, "rcon password set");
}

//
//  SV_SetClientRate
//
//  Performs range checking on client's rate
//
void SV_SetClientRate(client_t &client, int rate)
{
	client.rate = clamp(rate, 1, (int)sv_maxrate);
}

EXTERN_CVAR(sv_waddownloadcap)
CVAR_FUNC_IMPL(sv_maxrate)
{
	// sv_waddownloadcap can not be larger than sv_maxrate
	if (sv_waddownloadcap > var)
		sv_waddownloadcap.Set(var);

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		// ensure no clients exceed sv_maxrate
		SV_SetClientRate(it->client, it->client.rate);
	}
}

CVAR_FUNC_IMPL (sv_waddownloadcap)
{
	// sv_waddownloadcap can not be larger than sv_maxrate
	if (var > sv_maxrate)
		var.Set(sv_maxrate);
}

client_c clients;


#define CLIENT_TIMEOUT 65 // 65 seconds

void SV_UpdateConsolePlayer(player_t &player);

void SV_CheckTeam (player_t & playernum);
team_t SV_GoodTeam (void);

void SV_SendServerSettings (player_t &pl);
void SV_ServerSettingChange (void);

// some doom functions
size_t P_NumPlayersOnTeam(team_t team);

void SV_WinCheck (void);

// [AM] Flip a coin between heads and tails
BEGIN_COMMAND (coinflip) {
	std::string result;
	CMD_CoinFlip(result);

	SV_BroadcastPrintf(PRINT_HIGH, "%s\n", result.c_str());
} END_COMMAND (coinflip)

void CMD_CoinFlip(std::string &result) {
	result = (P_Random() % 2 == 0) ? "Coin came up Heads." : "Coin came up Tails.";
	return;
}

// denis - kick player
BEGIN_COMMAND (kick) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::string error;

	size_t pid;
	std::string reason;

	if (!CMD_KickCheck(arguments, error, pid, reason)) {
		Printf(PRINT_HIGH, "kick: %s\n", error.c_str());
		return;
	}

	SV_KickPlayer(idplayer(pid), reason);
} END_COMMAND (kick)

// Kick command check.
bool CMD_KickCheck(std::vector<std::string> arguments, std::string &error,
				   size_t &pid, std::string &reason) {
	// Did we pass enough arguments?
	if (arguments.size() < 1) {
		error = "need a player id (try 'players') and optionally a reason.";
		return false;
	}

	// Did we actually pass a player number?
	std::istringstream buffer(arguments[0]);
	buffer >> pid;

	if (!buffer) {
		error = "need a valid player number.";
		return false;
	}

	// Verify that the player we found is a valid client.
	player_t &player = idplayer(pid);

	if (!validplayer(player)) {
		std::ostringstream error_ss;
		error_ss << "could not find client " << pid << ".";
		error = error_ss.str();
		return false;
	}

	// Anything that is not the first argument is the reason.
	arguments.erase(arguments.begin());
	reason = JoinStrings(arguments, " ");

	return true;
}

// Kick a player.
void SV_KickPlayer(player_t &player, const std::string &reason) {
	// Avoid a segfault from an invalid player.
	if (!validplayer(player)) {
		return;
	}

	if (reason.empty())
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked from the server!\n", player.userinfo.netname.c_str());
	else
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked from the server! (Reason: %s)\n",
					player.userinfo.netname.c_str(), reason.c_str());

	player.client.displaydisconnect = false;
	SV_DropClient(player);
}

// Invalidate a player.
//
// Usually happens when corrupted messages are passed to the server by the
// client.  Reasons should only be seen by the server admin.  Player and client
// are both presumed unusable after function is done.
void SV_InvalidateClient(player_t &player, const std::string& reason)
{
	if (&(player.client) == NULL)
	{
		Printf(PRINT_HIGH, "Player with NULL client fails security check (%s), client cannot be safely dropped.\n");
		return;
	}

	Printf(PRINT_HIGH, "%s fails security check (%s), dropping client.\n", NET_AdrToString(player.client.address), reason.c_str());
	SV_DropClient(player);
}

BEGIN_COMMAND (stepmode)
{
    if (step_mode)
        step_mode = false;
    else
        step_mode = true;

    return;
}
END_COMMAND (stepmode)

BEGIN_COMMAND (say)
{
	if (argc > 1)
	{
		std::string chat = C_ArgCombine(argc - 1, (const char **)(argv + 1));
		SV_BroadcastPrintf(PRINT_SERVERCHAT, "[console]: %s\n", chat.c_str());
	}
}
END_COMMAND (say)

void STACK_ARGS call_terms (void);

void SV_QuitCommand()
{
	call_terms();
	exit(EXIT_SUCCESS);
}

BEGIN_COMMAND (rquit)
{
	SV_SendReconnectSignal();

	SV_QuitCommand();
}
END_COMMAND (rquit)

BEGIN_COMMAND (quit)
{
	SV_QuitCommand();
}
END_COMMAND (quit)

// An alias for 'quit'
BEGIN_COMMAND (exit)
{
	SV_QuitCommand();
}
END_COMMAND (exit)


//
// SV_InitNetwork
//
void SV_InitNetwork (void)
{
	netgame = false;  // for old network code
    network_game = true;


	const char *v = Args.CheckValue ("-port");
    if (v)
    {
       localport = atoi (v);
       Printf (PRINT_HIGH, "using alternate port %i\n", localport);
    }
	else
	   localport = SERVERPORT;

	// set up a socket and net_message buffer
	InitNetCommon();

	// determine my name & address
	// NET_GetLocalAddress ();

	Printf(PRINT_HIGH, "UDP Initialized\n");

	const char *w = Args.CheckValue ("-maxclients");
	if (w)
	{
		sv_maxclients.Set(w); // denis - todo
	}

	step_mode = Args.CheckParm ("-stepmode");

	// Nes - Connect with the master servers. (If valid)
	SV_InitMasters();
}

// [AM] TODO: Turn this into Players::push_back().
Players::iterator SV_GetFreeClient(void)
{
	if (players.size() >= sv_maxclients)
		return players.end();

	if (players.empty())
	{
		while (!free_player_ids.empty())
			free_player_ids.pop();

		// list of free ids needs to be initialized
		for (int i = 1; i < MAXPLAYERS; i++)
			free_player_ids.push(i);
	}

	players.push_back(player_t());

	// generate player id
	players.back().id = free_player_ids.front();
	free_player_ids.pop();

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());

	// Return iterator pointing to the just-inserted player
	Players::iterator it = players.end();
	return --it;
}

player_t &SV_FindPlayerByAddr(void)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (NET_CompareAdr(it->client.address, net_from))
		   return *it;
	}

	return idplayer(0);
}

//
// SV_CheckTimeouts
// If a packet has not been received from a client in CLIENT_TIMEOUT
// seconds, drop the conneciton.
//
void SV_CheckTimeouts()
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (gametic - it->client.last_received == CLIENT_TIMEOUT * 35)
		    SV_DropClient(*it);
	}
}


//
// SV_RemoveDisconnectedPlayer
//
// [SL] 2011-05-18 - Destroy a player's mo actor and remove the player_t
// from the global players vector.  Update mo->player pointers.
// [AM] Updated to remove from a list instead.  Eventually, we should turn
//      this into Players::erase().
Players::iterator SV_RemoveDisconnectedPlayer(Players::iterator it)
{
	if (it == players.end() || !validplayer(*it))
		return players.end();

	int player_id = it->id;

	// remove player awareness from all actors
	AActor* mo;
	TThinkerIterator<AActor> iterator;
	while ((mo = iterator.Next()))
		mo->players_aware.unset(it->id);

	// remove this player's actor object
	if (it->mo)
	{
		if (sv_gametype == GM_CTF) //  [Toke - CTF]
			CTF_CheckFlags(*it);

		// [AM] AActor->Destroy() does not destroy the AActor for good, and also
		//      does not null the player reference.  We have to do it here to
		//      prevent actions on a zombie mobj from using a player that we
		//      already erased from the players list.
		it->mo->player = NULL;

		it->mo->Destroy();
		it->mo = AActor::AActorPtr();
	}

	// remove this player from the global players vector
	Players::iterator next;
	next = players.erase(it);
	free_player_ids.push(player_id);

	Unlag::getInstance().unregisterPlayer(player_id);

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());

	return next;
}

//
// SV_GetPackets
//
void SV_GetPackets()
{
	while (NET_GetPacket())
	{
		player_t &player = SV_FindPlayerByAddr();

		if (!validplayer(player)) // no client with net_from address
		{
			// apparently, someone is trying to connect
			if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
				SV_ConnectClient();

			continue;
		}
		else
		{
			if(player.playerstate != PST_DISCONNECT)
			{
				player.client.last_received = gametic;
				SV_ParseCommands(player);
			}
		}
	}
}


// Print a midscreen message to a client
void SV_MidPrint (const char *msg, player_t *p, int msgtime)
{
    client_t *cl = &p->client;

    MSG_WriteMarker(&cl->reliablebuf, svc_midprint);
    MSG_WriteString(&cl->reliablebuf, msg);
    if (msgtime)
        MSG_WriteShort(&cl->reliablebuf, msgtime);
    else
        MSG_WriteShort(&cl->reliablebuf, 0);
}

//
// SV_Sound
//
void SV_Sound (AActor *mo, byte channel, const char *name, byte attenuation)
{
	int sfx_id;
	client_t* cl;
	int x = 0, y = 0;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	if (mo)
	{
		x = mo->x;
		y = mo->y;
	}

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		cl = &(it->client);

		MSG_WriteMarker (&cl->netbuf, svc_startsound);
		if(mo)
			MSG_WriteShort (&cl->netbuf, mo->netid);
		else
			MSG_WriteShort (&cl->netbuf, 0);
		MSG_WriteLong (&cl->netbuf, x);
		MSG_WriteLong (&cl->netbuf, y);
		MSG_WriteByte (&cl->netbuf, channel);
		MSG_WriteByte (&cl->netbuf, sfx_id);
		MSG_WriteByte (&cl->netbuf, attenuation);
		MSG_WriteByte (&cl->netbuf, 255); // client calculates volume on its own
	}

}

void SV_Sound (player_t &pl, AActor *mo, byte channel, const char *name, byte attenuation)
{
	int sfx_id;
	int x = 0, y = 0;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	if(mo)
	{
		x = mo->x;
		y = mo->y;
	}

	client_t *cl = &pl.client;

	MSG_WriteMarker (&cl->netbuf, svc_startsound);
	if (mo == NULL)
		MSG_WriteShort (&cl->netbuf, 0);
	else
		MSG_WriteShort (&cl->netbuf, mo->netid);
	MSG_WriteLong (&cl->netbuf, x);
	MSG_WriteLong (&cl->netbuf, y);
	MSG_WriteByte (&cl->netbuf, channel);
	MSG_WriteByte (&cl->netbuf, sfx_id);
	MSG_WriteByte (&cl->netbuf, attenuation);
	MSG_WriteByte (&cl->netbuf, 255);		// client calculates volume on its own
}

//
// UV_SoundAvoidPlayer
// Sends a sound to clients, but doesn't send it to client 'player'.
//
void UV_SoundAvoidPlayer (AActor *mo, byte channel, const char *name, byte attenuation)
{
	int        sfx_id;
	client_t  *cl;

	if (!mo || !mo->player)
		return;

	player_t &pl = *mo->player;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if(&pl == &*it)
			continue;

		cl = &(it->client);

		MSG_WriteMarker(&cl->netbuf, svc_startsound);
		MSG_WriteShort(&cl->netbuf, mo->netid);
		MSG_WriteLong(&cl->netbuf, mo->x);
		MSG_WriteLong(&cl->netbuf, mo->y);
		MSG_WriteByte(&cl->netbuf, channel);
		MSG_WriteByte(&cl->netbuf, sfx_id);
		MSG_WriteByte(&cl->netbuf, attenuation);
		MSG_WriteByte(&cl->netbuf, 255); // client calculates volume on its own
	}
}

//
//	SV_SoundTeam
//	Sends a sound to players on the specified teams
//
void SV_SoundTeam (byte channel, const char* name, byte attenuation, int team)
{
	int sfx_id;

	client_t* cl;

	sfx_id = S_FindSound( name );

	if ( sfx_id > 255 || sfx_id < 0 )
	{
		Printf( PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id );
		return;
	}

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame() && it->userinfo.team == team)
		{
			cl = &(it->client);

			MSG_WriteMarker(&cl->netbuf, svc_startsound);
			// Set netid to 0 since it's not a sound originating from any player's location
			MSG_WriteShort(&cl->netbuf, 0); // netid
			MSG_WriteLong(&cl->netbuf, 0); // x
			MSG_WriteLong(&cl->netbuf, 0); // y
			MSG_WriteByte(&cl->netbuf, channel);
			MSG_WriteByte(&cl->netbuf, sfx_id);
			MSG_WriteByte(&cl->netbuf, attenuation);
			MSG_WriteByte(&cl->netbuf, 255); // client calculates volume on its own
		}
	}
}

void SV_Sound (fixed_t x, fixed_t y, byte channel, const char *name, byte attenuation)
{
	int        sfx_id;
	client_t  *cl;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		cl = &(it->client);

		MSG_WriteMarker(&cl->netbuf, svc_soundorigin);
		MSG_WriteLong(&cl->netbuf, x);
		MSG_WriteLong(&cl->netbuf, y);
		MSG_WriteByte(&cl->netbuf, channel);
		MSG_WriteByte(&cl->netbuf, sfx_id);
		MSG_WriteByte(&cl->netbuf, attenuation);
		MSG_WriteByte(&cl->netbuf, 255); // client calculates volume on its own
	}
}

//
// SV_UpdateFrags
//
void SV_UpdateFrags(player_t &player)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteMarker(&cl->reliablebuf, svc_updatefrags);
		MSG_WriteByte(&cl->reliablebuf, player.id);
		if (sv_gametype != GM_COOP)
			MSG_WriteShort(&cl->reliablebuf, player.fragcount);
		else
			MSG_WriteShort(&cl->reliablebuf, player.killcount);
		MSG_WriteShort (&cl->reliablebuf, player.deathcount);
		MSG_WriteShort(&cl->reliablebuf, player.points);
	}
}

//
// SV_SendUserInfo
//
void SV_SendUserInfo (player_t &player, client_t* cl)
{
	player_t *p = &player;

	MSG_WriteMarker	(&cl->reliablebuf, svc_userinfo);
	MSG_WriteByte	(&cl->reliablebuf, p->id);
	MSG_WriteString (&cl->reliablebuf, p->userinfo.netname.c_str());
	MSG_WriteByte	(&cl->reliablebuf, p->userinfo.team);
	MSG_WriteLong	(&cl->reliablebuf, p->userinfo.gender);

	for (int i = 3; i >= 0; i--)
		MSG_WriteByte(&cl->reliablebuf, p->userinfo.color[i]);

	// [SL] place holder for deprecated skins
	MSG_WriteString	(&cl->reliablebuf, "");

	MSG_WriteShort	(&cl->reliablebuf, time(NULL) - p->JoinTime);
}

void SV_BroadcastUserInfo(player_t &player)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
		SV_SendUserInfo(player, &(it->client));
}

/**
 * Stores a players userinfo.
 *
 * @param player Player to parse info for.
 * @return False if the client was kicked because of something seriously
 *         screwy going on with their info.
 */
bool SV_SetupUserInfo(player_t &player)
{
	// read in userinfo from packet
	std::string old_netname(player.userinfo.netname);
	std::string new_netname(MSG_ReadString());
	StripColorCodes(new_netname);

	if (new_netname.length() > MAXPLAYERNAME)
		new_netname.erase(MAXPLAYERNAME);

	if (!ValidString(new_netname))
	{
		SV_InvalidateClient(player, "Name contains invalid characters");
		return false;
	}

	team_t old_team = static_cast<team_t>(player.userinfo.team);
	team_t new_team = static_cast<team_t>(MSG_ReadByte());

	if ((new_team >= NUMTEAMS && new_team != TEAM_NONE) || new_team < 0)
	{
		SV_InvalidateClient(player, "Team preference is invalid");
		return false;
	}

	gender_t gender = static_cast<gender_t>(MSG_ReadLong());

	byte color[4];
	for (int i = 3; i >= 0; i--)
		color[i] = MSG_ReadByte();

	// [SL] place holder for deprecated skins
	MSG_ReadString();

	fixed_t aimdist = MSG_ReadLong();
	bool unlag = MSG_ReadBool();
	bool predict_weapons = MSG_ReadBool();
	byte update_rate = MSG_ReadByte();

	weaponswitch_t switchweapon = static_cast<weaponswitch_t>(MSG_ReadByte());

	byte weapon_prefs[NUMWEAPONS];
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		// sanitize the weapon preference input
		byte preflevel = MSG_ReadByte();
		if (preflevel >= NUMWEAPONS)
			preflevel = NUMWEAPONS - 1;

		weapon_prefs[i] = preflevel;
	}

	// ensure sane values for userinfo
	if (gender < 0 || gender >= NUMGENDER)
		gender = GENDER_NEUTER;

	aimdist = clamp(aimdist, 0, 5000 * 16384);
	update_rate = clamp((int)update_rate, 1, 3);

	if (switchweapon >= WPSW_NUMTYPES || switchweapon < 0)
		switchweapon = WPSW_ALWAYS;

	// [SL] 2011-12-02 - Players can update these parameters whenever they like
	player.userinfo.unlag			= unlag;
	player.userinfo.predict_weapons	= predict_weapons;
	player.userinfo.update_rate		= update_rate;
	player.userinfo.aimdist			= aimdist;
	player.userinfo.switchweapon	= switchweapon;
	memcpy(player.userinfo.weapon_prefs, weapon_prefs, sizeof(weapon_prefs));

	player.userinfo.gender			= gender;
	player.userinfo.team			= new_team;

	memcpy(player.userinfo.color, color, 4);
	memcpy(player.prefcolor, color, 4);

	// sanitize the client's name
	new_netname = TrimString(new_netname);
	if (new_netname.empty())
	{
		if (old_netname.empty())
			new_netname = "PLAYER";
		else
			new_netname = old_netname;
	}

	// Make sure that we're not duplicating any player name
	player_t& other = nameplayer(new_netname);
	if (validplayer(other))
	{
		// Give the player an enumerated name (Player2, Player3, etc.)
		if (player.id != other.id)
		{
			std::string test_netname = std::string(new_netname);

			for (int num = 2;num < MAXPLAYERS + 2;num++)
			{
				std::ostringstream buffer;
				buffer << num;
				std::string strnum = buffer.str();

				// If the enumerated name would be greater than the maximum
				// allowed netname, have the enumeration eat into the last few
				// letters of their netname.
				if (new_netname.length() + strnum.length() >= MAXPLAYERNAME)
					test_netname = new_netname.substr(0, MAXPLAYERNAME - strnum.length()) + strnum;
				else
					test_netname = new_netname + strnum;

				// Check to see if the enumerated name is taken.
				player_t& test = nameplayer(test_netname.c_str());
				if (!validplayer(test))
					break;

				// Check to see if player already has a given enumerated name.
				// Prevents `cl_name Player` from changing their own name from
				// Player2 to Player3.
				if (player.id == test.id)
					break;
			}

			new_netname = test_netname;
		}
		else
		{
			// Player is trying to be cute and change their name to an
			// automatically enumerated name that they already have.  Prevents
			// `cl_name Player2` from changing their own name from Player2 to
			// Player22.
			new_netname = old_netname;
		}
	}

	if (new_netname.length() > MAXPLAYERNAME)
		new_netname.erase(MAXPLAYERNAME);

	player.userinfo.netname = new_netname;

	// Compare names and broadcast if different.
	if (!old_netname.empty() && !iequals(new_netname, old_netname))
	{
		std::string	gendermessage;
		switch (gender) {
			case GENDER_MALE:	gendermessage = "his";  break;
			case GENDER_FEMALE:	gendermessage = "her";  break;
			default:			gendermessage = "its";  break;
		}

		SV_BroadcastPrintf(PRINT_HIGH, "%s changed %s name to %s.\n",
			old_netname.c_str(), gendermessage.c_str(), player.userinfo.netname.c_str());
	}

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		SV_CheckTeam (player);

		if (player.mo && player.userinfo.team != old_team &&
			player.ingame() && !player.spectator)
		{
			// kill player if team is changed
			P_DamageMobj (player.mo, 0, 0, 1000, 0);
			SV_BroadcastPrintf(PRINT_HIGH, "%s switched to the %s team.\n",
				player.userinfo.netname.c_str(), team_names[new_team]);
		}
	}

	return true;
}

//
//	SV_ForceSetTeam
//
//	Forces a client to join a specified team
//
void SV_ForceSetTeam (player_t &who, team_t team)
{
	client_t *cl = &who.client;

	MSG_WriteMarker (&cl->reliablebuf, svc_forceteam);

	who.userinfo.team = team;
	Printf (PRINT_HIGH, "Forcing %s to %s team\n", who.userinfo.netname.c_str(), team == TEAM_NONE ? "NONE" : team_names[team]);
	MSG_WriteShort (&cl->reliablebuf, team);
}

EXTERN_CVAR (sv_teamsinplay)

//
//	SV_CheckTeam
//
//	Checks to see if a players team is allowed and corrects if not
//
void SV_CheckTeam (player_t &player)
{
	if (sv_gametype == GM_CTF &&
			(player.userinfo.team < 0 || player.userinfo.team >= NUMTEAMS))
		SV_ForceSetTeam (player, SV_GoodTeam ());

	if (sv_gametype != GM_CTF &&
			(player.userinfo.team < 0 || player.userinfo.team >= sv_teamsinplay))
		SV_ForceSetTeam (player, SV_GoodTeam ());
}

//
//	SV_GoodTeam
//
//	Finds a working team
//
team_t SV_GoodTeam (void)
{
	int teamcount = NUMTEAMS;
	if (sv_gametype != GM_CTF && sv_teamsinplay >= 0 &&
	    sv_teamsinplay <= NUMTEAMS)
		teamcount = sv_teamsinplay;

	if (teamcount == 0)
	{
		I_Error ("Teamplay is set and no teams are enabled!\n");
		return TEAM_NONE;
	}

	// Find the smallest team
	size_t smallest_team_size = MAXPLAYERS;
	team_t smallest_team = (team_t)0;
	for (int i = 0;i < teamcount;i++)
	{
		size_t team_size = P_NumPlayersOnTeam((team_t)i);
		if (team_size < smallest_team_size)
		{
			smallest_team_size = team_size;
			smallest_team = (team_t)i;
		}
	}

	if (sv_maxplayersperteam && smallest_team_size >= sv_maxplayersperteam)
		return TEAM_NONE;

	return smallest_team;
}

//
// SV_SendMobjToClient
//
void SV_SendMobjToClient(AActor *mo, client_t *cl)
{
	if (!mo)
		return;

	MSG_WriteMarker(&cl->reliablebuf, svc_spawnmobj);
	MSG_WriteLong(&cl->reliablebuf, mo->x);
	MSG_WriteLong(&cl->reliablebuf, mo->y);
	MSG_WriteLong(&cl->reliablebuf, mo->z);
	MSG_WriteLong(&cl->reliablebuf, mo->angle);

	MSG_WriteShort(&cl->reliablebuf, mo->type);
	MSG_WriteShort(&cl->reliablebuf, mo->netid);
	MSG_WriteByte(&cl->reliablebuf, mo->rndindex);
	MSG_WriteShort(&cl->reliablebuf, (mo->state - states)); // denis - sending state fixes monster ghosts appearing under doors

	if (mo->type == MT_FOUNTAIN)
		MSG_WriteByte(&cl->reliablebuf, mo->args[0]);

	if (mo->type == MT_ZDOOMBRIDGE)
	{
		MSG_WriteByte(&cl->reliablebuf, mo->args[0]);
		MSG_WriteByte(&cl->reliablebuf, mo->args[1]);
	}

	if(mo->flags & MF_MISSILE || mobjinfo[mo->type].flags & MF_MISSILE) // denis - check type as that is what the client will be spawning
	{
		MSG_WriteShort (&cl->reliablebuf, mo->target ? mo->target->netid : 0);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);
		MSG_WriteLong (&cl->reliablebuf, mo->angle);
		MSG_WriteLong (&cl->reliablebuf, mo->momx);
		MSG_WriteLong (&cl->reliablebuf, mo->momy);
		MSG_WriteLong (&cl->reliablebuf, mo->momz);
	}
	else
	{
		if(mo->flags & MF_AMBUSH || mo->flags & MF_DROPPED)
		{
			MSG_WriteMarker(&cl->reliablebuf, svc_mobjinfo);
			MSG_WriteShort(&cl->reliablebuf, mo->netid);
			MSG_WriteLong(&cl->reliablebuf, mo->flags);
		}
	}

	// animating corpses
	if((mo->flags & MF_CORPSE) && mo->state - states != S_GIBS)
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_corpse);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);
		MSG_WriteByte (&cl->reliablebuf, mo->frame);
		MSG_WriteByte (&cl->reliablebuf, mo->tics);
	}
}

//
// SV_IsTeammate
//
bool SV_IsTeammate(player_t &a, player_t &b)
{
	// same player isn't own teammate
	if(&a == &b)
		return false;

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		if (a.userinfo.team == b.userinfo.team)
			return true;
		else
			return false;
	}
	else if (sv_gametype == GM_COOP)
		return true;

	else return false;
}

//
// [denis] SV_AwarenessUpdate
//
bool SV_AwarenessUpdate(player_t &player, AActor *mo)
{
	bool ok = false;

	if (!mo)
		return false;

	if(player.mo == mo)
		ok = true;
	else if(!mo->player)
		ok = true;
	else if (mo->flags & MF_SPECTATOR)      // GhostlyDeath -- Spectating things
		ok = false;
	else if(player.mo && mo->player && mo->player->spectator)
		ok = false;
	else if(player.mo && mo->player && SV_IsTeammate(player, *mo->player))
		ok = true;
	else if(player.mo && mo->player && true)
		ok = true;

	bool previously_ok = mo->players_aware.get(player.id);

	client_t *cl = &player.client;

	if(!ok && previously_ok)
	{
		mo->players_aware.unset(player.id);

		MSG_WriteMarker (&cl->reliablebuf, svc_removemobj);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);

		return true;
	}
	else if(!previously_ok && ok)
	{
		mo->players_aware.set(player.id);

		if(!mo->player || mo->player->playerstate != PST_LIVE)
		{
			SV_SendMobjToClient(mo, cl);
		}
		else
		{
			MSG_WriteMarker (&cl->reliablebuf, svc_spawnplayer);
			MSG_WriteByte (&cl->reliablebuf, mo->player->id);
			MSG_WriteShort (&cl->reliablebuf, mo->netid);
			MSG_WriteLong (&cl->reliablebuf, mo->angle);
			MSG_WriteLong (&cl->reliablebuf, mo->x);
			MSG_WriteLong (&cl->reliablebuf, mo->y);
			MSG_WriteLong (&cl->reliablebuf, mo->z);
		}

		return true;
	}

	return false;
}

//
// [denis] SV_SpawnMobj
// because you can't expect the constructors to send network messages!
//
void SV_SpawnMobj(AActor *mo)
{
	if (!mo)
		return;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if(mo->player)
			SV_AwarenessUpdate(*it, mo);
		else
			it->to_spawn.push(mo->ptr());
	}
}

//
// [denis] SV_IsPlayerAllowedToSee
// determine if a client should be able to see an actor
//
bool SV_IsPlayerAllowedToSee(player_t &p, AActor *mo)
{
	if (!mo)
		return false;

	if (mo->flags & MF_SPECTATOR)
		return false; // GhostlyDeath -- always false, as usual!
	else
		return mo->players_aware.get(p.id);
}

#define HARDWARE_CAPABILITY 1000

//
// SV_UpdateHiddenMobj
//
void SV_UpdateHiddenMobj (void)
{
	// denis - todo - throttle this
	AActor *mo;
	TThinkerIterator<AActor> iterator;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		player_t &pl = *it;

		if(!pl.mo)
			continue;

		int updated = 0;

		while(!pl.to_spawn.empty())
		{
			mo = pl.to_spawn.front();

			pl.to_spawn.pop();

			if(mo && !mo->WasDestroyed())
				updated += SV_AwarenessUpdate(pl, mo);

			if(updated > 16)
				break;
		}

		while ( (mo = iterator.Next() ) )
		{
			updated += SV_AwarenessUpdate(pl, mo);

			if(updated > 16)
				break;
		}
	}
}

//
// SV_UpdateSectors
// Update doors, floors, ceilings etc... that have at some point moved
//
void SV_UpdateSectors(client_t* cl)
{
	for (int sectornum = 0; sectornum < numsectors; sectornum++)
	{
		sector_t* sector = &sectors[sectornum];

		if (sector->moveable)
		{
			MSG_WriteMarker(&cl->reliablebuf, svc_sector);
			MSG_WriteShort(&cl->reliablebuf, sectornum);
			MSG_WriteShort(&cl->reliablebuf, P_FloorHeight(sector) >> FRACBITS);
			MSG_WriteShort(&cl->reliablebuf, P_CeilingHeight(sector) >> FRACBITS);
			MSG_WriteShort(&cl->reliablebuf, sector->floorpic);
			MSG_WriteShort(&cl->reliablebuf, sector->ceilingpic);
		}
	}
}

//
// SV_DestroyFinishedMovingSectors
//
// Calls Destroy() on moving sectors that are done moving.
//
void SV_DestroyFinishedMovingSectors()
{
	std::list<movingsector_t>::iterator itr;
	itr = movingsectors.begin();

	while (itr != movingsectors.end())
	{
		sector_t *sector = itr->sector;

		if (P_MovingCeilingCompleted(sector))
		{
			itr->moving_ceiling = false;
			if (sector->ceilingdata)
				sector->ceilingdata->Destroy();
			sector->ceilingdata = NULL;
		}
		if (P_MovingFloorCompleted(sector))
		{
			itr->moving_floor = false;
			if (sector->floordata)
				sector->floordata->Destroy();
			sector->floordata = NULL;
		}

		if (!itr->moving_ceiling && !itr->moving_floor)
			movingsectors.erase(itr++);
		else
			++itr;
	}
}

//
// SV_SendMovingSectorUpdate
//
//
void SV_SendMovingSectorUpdate(player_t &player, sector_t *sector)
{
	if (!sector || !validplayer(player))
		return;

	int sectornum = sector - sectors;
	if (sectornum < 0 || sectornum >= numsectors)
		return;

	buf_t *netbuf = &(player.client.netbuf);

	// Determine which moving planes are in this sector
	movertype_t floor_mover = SEC_INVALID, ceiling_mover = SEC_INVALID;

	if (sector->ceilingdata && sector->ceilingdata->IsA(RUNTIME_CLASS(DCeiling)))
		ceiling_mover = SEC_CEILING;
	if (sector->ceilingdata && sector->ceilingdata->IsA(RUNTIME_CLASS(DDoor)))
		ceiling_mover = SEC_DOOR;
	if (sector->floordata && sector->floordata->IsA(RUNTIME_CLASS(DFloor)))
		floor_mover = SEC_FLOOR;
	if (sector->floordata && sector->floordata->IsA(RUNTIME_CLASS(DPlat)))
		floor_mover = SEC_PLAT;
	if (sector->ceilingdata && sector->ceilingdata->IsA(RUNTIME_CLASS(DElevator)))
	{
		ceiling_mover = SEC_ELEVATOR;
		floor_mover = SEC_INVALID;
	}
	if (sector->ceilingdata && sector->ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
	{
		ceiling_mover = SEC_PILLAR;
		floor_mover = SEC_INVALID;
	}

	// no moving planes?  skip it.
	if (ceiling_mover == SEC_INVALID && floor_mover == SEC_INVALID)
		return;

	// Create bitfield to denote moving planes in this sector
	byte movers = byte(ceiling_mover) | (byte(floor_mover) << 4);

	MSG_WriteMarker(netbuf, svc_movingsector);
	MSG_WriteShort(netbuf, sectornum);
	MSG_WriteShort(netbuf, P_CeilingHeight(sector) >> FRACBITS);
	MSG_WriteShort(netbuf, P_FloorHeight(sector) >> FRACBITS);
	MSG_WriteByte(netbuf, movers);

	if (ceiling_mover == SEC_ELEVATOR)
	{
		DElevator *Elevator = static_cast<DElevator *>(sector->ceilingdata);

        MSG_WriteByte(netbuf, Elevator->m_Type);
        MSG_WriteByte(netbuf, Elevator->m_Status);
        MSG_WriteByte(netbuf, Elevator->m_Direction);
        MSG_WriteShort(netbuf, Elevator->m_FloorDestHeight >> FRACBITS);
        MSG_WriteShort(netbuf, Elevator->m_CeilingDestHeight >> FRACBITS);
        MSG_WriteShort(netbuf, Elevator->m_Speed >> FRACBITS);
	}

	if (ceiling_mover == SEC_PILLAR)
	{
		DPillar *Pillar = static_cast<DPillar *>(sector->ceilingdata);

        MSG_WriteByte(netbuf, Pillar->m_Type);
        MSG_WriteByte(netbuf, Pillar->m_Status);
        MSG_WriteShort(netbuf, Pillar->m_FloorSpeed >> FRACBITS);
        MSG_WriteShort(netbuf, Pillar->m_CeilingSpeed >> FRACBITS);
        MSG_WriteShort(netbuf, Pillar->m_FloorTarget >> FRACBITS);
        MSG_WriteShort(netbuf, Pillar->m_CeilingTarget >> FRACBITS);
        MSG_WriteBool(netbuf, Pillar->m_Crush);
	}

	if (ceiling_mover == SEC_CEILING)
	{
		DCeiling *Ceiling = static_cast<DCeiling *>(sector->ceilingdata);

        MSG_WriteByte(netbuf, Ceiling->m_Type);
        MSG_WriteShort(netbuf, Ceiling->m_BottomHeight >> FRACBITS);
        MSG_WriteShort(netbuf, Ceiling->m_TopHeight >> FRACBITS);
        MSG_WriteShort(netbuf, Ceiling->m_Speed >> FRACBITS);
        MSG_WriteShort(netbuf, Ceiling->m_Speed1 >> FRACBITS);
        MSG_WriteShort(netbuf, Ceiling->m_Speed2 >> FRACBITS);
        MSG_WriteBool(netbuf, Ceiling->m_Crush);
        MSG_WriteBool(netbuf, Ceiling->m_Silent);
        MSG_WriteByte(netbuf, Ceiling->m_Direction);
        MSG_WriteShort(netbuf, Ceiling->m_Texture);
        MSG_WriteShort(netbuf, Ceiling->m_NewSpecial);
        MSG_WriteShort(netbuf, Ceiling->m_Tag);
        MSG_WriteByte(netbuf, Ceiling->m_OldDirection);
	}

	if (ceiling_mover == SEC_DOOR)
	{
		DDoor *Door = static_cast<DDoor *>(sector->ceilingdata);

        MSG_WriteByte(netbuf, Door->m_Type);
        MSG_WriteShort(netbuf, Door->m_TopHeight >> FRACBITS);
        MSG_WriteShort(netbuf, Door->m_Speed >> FRACBITS);
        MSG_WriteLong(netbuf, Door->m_TopWait);
        MSG_WriteLong(netbuf, Door->m_TopCountdown);
		MSG_WriteByte(netbuf, Door->m_Status);
		// Check for an invalid m_Line (doors triggered by tag 666)
        MSG_WriteLong(netbuf, Door->m_Line ? (Door->m_Line - lines) : -1);
	}

	if (floor_mover == SEC_FLOOR)
	{
		DFloor *Floor = static_cast<DFloor *>(sector->floordata);

        MSG_WriteByte(netbuf, Floor->m_Type);
        MSG_WriteByte(netbuf, Floor->m_Status);
        MSG_WriteBool(netbuf, Floor->m_Crush);
        MSG_WriteByte(netbuf, Floor->m_Direction);
        MSG_WriteShort(netbuf, Floor->m_NewSpecial);
        MSG_WriteShort(netbuf, Floor->m_Texture);
        MSG_WriteShort(netbuf, Floor->m_FloorDestHeight >> FRACBITS);
        MSG_WriteShort(netbuf, Floor->m_Speed >> FRACBITS);
        MSG_WriteLong(netbuf, Floor->m_ResetCount);
        MSG_WriteShort(netbuf, Floor->m_OrgHeight >> FRACBITS);
        MSG_WriteLong(netbuf, Floor->m_Delay);
        MSG_WriteLong(netbuf, Floor->m_PauseTime);
        MSG_WriteLong(netbuf, Floor->m_StepTime);
        MSG_WriteLong(netbuf, Floor->m_PerStepTime);
        MSG_WriteShort(netbuf, Floor->m_Height >> FRACBITS);
        MSG_WriteByte(netbuf, Floor->m_Change);
		MSG_WriteLong(netbuf, Floor->m_Line ? (Floor->m_Line - lines) : -1);
	}

	if (floor_mover == SEC_PLAT)
	{
		DPlat *Plat = static_cast<DPlat *>(sector->floordata);

        MSG_WriteShort(netbuf, Plat->m_Speed >> FRACBITS);
        MSG_WriteShort(netbuf, Plat->m_Low >> FRACBITS);
        MSG_WriteShort(netbuf, Plat->m_High >> FRACBITS);
        MSG_WriteLong(netbuf, Plat->m_Wait);
        MSG_WriteLong(netbuf, Plat->m_Count);
        MSG_WriteByte(netbuf, Plat->m_Status);
        MSG_WriteByte(netbuf, Plat->m_OldStatus);
        MSG_WriteBool(netbuf, Plat->m_Crush);
        MSG_WriteShort(netbuf, Plat->m_Tag);
        MSG_WriteByte(netbuf, Plat->m_Type);
        MSG_WriteShort(netbuf, Plat->m_Height >> FRACBITS);
        MSG_WriteShort(netbuf, Plat->m_Lip >> FRACBITS);
	}
}

//
// SV_UpdateMovingSectors
// Update doors, floors, ceilings etc... that are actively moving
//
void SV_UpdateMovingSectors(player_t &player)
{
	std::list<movingsector_t>::iterator itr;
	for (itr = movingsectors.begin(); itr != movingsectors.end(); ++itr)
	{
		sector_t *sector = itr->sector;

		SV_SendMovingSectorUpdate(player, sector);
	}
}


//
// SV_SendGametic
// Sends gametic to synchronize with the client
//
// [SL] 2011-05-11 - Instead of sending the whole gametic (4 bytes),
// send only the least significant byte to save bandwidth.
void SV_SendGametic(client_t* cl)
{
	MSG_WriteMarker	(&cl->netbuf, svc_svgametic);
	MSG_WriteByte	(&cl->netbuf, (byte)(gametic & 0xFF));
}


//
// SV_ClientFullUpdate
//
void SV_ClientFullUpdate(player_t &pl)
{
	client_t *cl = &pl.client;

	// send player's info to the client
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->mo)
			SV_AwarenessUpdate(pl, it->mo);

		SV_SendUserInfo(*it, cl);

		if (cl->reliablebuf.cursize >= 600)
			if (!SV_SendPacket(pl))
				return;
	}

	// update warmup state
	SV_SendWarmupState(pl, warmup.get_status(), warmup.get_countdown());

	// update frags/points/.tate./ready
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		MSG_WriteMarker(&cl->reliablebuf, svc_updatefrags);
		MSG_WriteByte(&cl->reliablebuf, it->id);
		if(sv_gametype != GM_COOP)
			MSG_WriteShort(&cl->reliablebuf, it->fragcount);
		else
			MSG_WriteShort(&cl->reliablebuf, it->killcount);
		MSG_WriteShort(&cl->reliablebuf, it->deathcount);
		MSG_WriteShort(&cl->reliablebuf, it->points);

		MSG_WriteMarker (&cl->reliablebuf, svc_spectate);
		MSG_WriteByte (&cl->reliablebuf, it->id);
		MSG_WriteByte (&cl->reliablebuf, it->spectator);

		MSG_WriteMarker (&cl->reliablebuf, svc_readystate);
		MSG_WriteByte (&cl->reliablebuf, it->id);
		MSG_WriteByte (&cl->reliablebuf, it->ready);
	}

	// [deathz0r] send team frags/captures if teamplay is enabled
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		MSG_WriteMarker(&cl->reliablebuf, svc_teampoints);
		for (int i = 0;i < NUMTEAMS;i++)
			MSG_WriteShort(&cl->reliablebuf, TEAMpoints[i]);
	}

	SV_UpdateHiddenMobj();

	// update flags
	if (sv_gametype == GM_CTF)
		CTF_Connect(pl);

	// update sectors
	SV_UpdateSectors(cl);
	if (cl->reliablebuf.cursize >= 600)
		if(!SV_SendPacket(pl))
			return;

	// update switches
	for (int l=0; l<numlines; l++)
	{
		unsigned state = 0, time = 0;
		if(P_GetButtonInfo(&lines[l], state, time) || lines[l].wastoggled)
		{
			MSG_WriteMarker (&cl->reliablebuf, svc_switch);
			MSG_WriteLong (&cl->reliablebuf, l);
			MSG_WriteByte (&cl->reliablebuf, lines[l].wastoggled);
			MSG_WriteByte (&cl->reliablebuf, state);
			MSG_WriteLong (&cl->reliablebuf, time);
		}
	}

	MSG_WriteMarker(&cl->reliablebuf, svc_fullupdatedone);

	SV_SendPacket(pl);
}

//
//	SV_SendServerSettings
//
//	Sends server setting info
//

void SV_SendPackets(void);

void SV_SendServerSettings (player_t &pl)
{
	// GhostlyDeath <June 19, 2008> -- Loop through all CVARs and send the CVAR_SERVERINFO stuff only
	cvar_t *var = GetFirstCvar();

    client_t *cl = &pl.client;

	while (var)
	{
		if (var->flags() & CVAR_SERVERINFO)
		{
            if ((cl->reliablebuf.cursize + 1 + 1 + (strlen(var->name()) + 1) + (strlen(var->cstring()) + 1) + 1) >= 512)
                SV_SendPacket(pl);

            MSG_WriteMarker(&cl->reliablebuf, svc_serversettings);

            MSG_WriteByte(&cl->reliablebuf, 1); // TODO: REMOVE IN 0.7

			MSG_WriteString(&cl->reliablebuf, var->name());
			MSG_WriteString(&cl->reliablebuf, var->cstring());

            MSG_WriteByte(&cl->reliablebuf, 2); // TODO: REMOVE IN 0.7
		}

		var = var->GetNext();
	}
}

//
//	SV_ServerSettingChange
//
//	Sends server settings to clients when changed
//
void SV_ServerSettingChange (void)
{
	if (gamestate != GS_LEVEL)
		return;

	for (Players::iterator it = players.begin();it != players.end();++it)
		SV_SendServerSettings(*it);
}

// SV_CheckClientVersion
bool SV_CheckClientVersion(client_t *cl, Players::iterator it)
{
	int GameVer = 0;
	char VersionStr[20];
	char OurVersionStr[20];
	memset(VersionStr, 0, sizeof(VersionStr));
	memset(OurVersionStr, 0, sizeof(VersionStr));
	bool AllowConnect = true;
	int majorver = 0;
	int minorver = 0;
	int releasever = 0;
	int MAJORVER = GAMEVER / 256;
	int MINORVER = (GAMEVER % 256) / 10;
	int RELEASEVER = (GAMEVER % 256) % 10;

	if ((GAMEVER % 256) % 10)
		sprintf(OurVersionStr, "%i.%i.%i", GAMEVER / 256, (GAMEVER % 256) / 10, (GAMEVER % 256) % 10);
	else
		sprintf(OurVersionStr, "%i.%i", GAMEVER / 256, (GAMEVER % 256) / 10);

	switch (cl->version)
	{
		case 65:
			GameVer = MSG_ReadLong();
			cl->majorversion = GameVer / 256;
			cl->minorversion = GameVer % 256;
			if ((GameVer % 256) % 10)
				sprintf(VersionStr, "%i.%i.%i", cl->majorversion, cl->minorversion / 10, cl->minorversion % 10);
			else
				sprintf(VersionStr, "%i.%i", cl->majorversion, cl->minorversion / 10);

			majorver = GameVer / 256;
			minorver = (GameVer % 256) / 10;
			releasever = (GameVer % 256) % 10;

			if ((majorver == MAJORVER) &&
				(minorver == MINORVER) &&
				(releasever >= RELEASEVER))
				AllowConnect = true;
			else
				AllowConnect = false;
			break;
		case 64:
			sprintf(VersionStr, "0.2a or 0.3");
			break;
		case 63:
			sprintf(VersionStr, "Pre-0.2");
			break;
		case 62:
			sprintf(VersionStr, "0.1a");
			break;
		default:
			sprintf(VersionStr, "Unknown");
			break;

	}

	// GhostlyDeath -- removes the constant AllowConnects above
	if (cl->version != 65)
		AllowConnect = false;

	// GhostlyDeath -- boot em
	if (!AllowConnect)
	{
		std::ostringstream FormattedString;
		bool older = false;

		// GhostlyDeath -- Version Mismatch message

        FormattedString <<
            std::endl <<
            "Your version of Odamex " <<
            VersionStr <<
            " does not match the server " <<
            OurVersionStr <<
            std::endl;

		// GhostlyDeath -- Check to see if it's older or not
		if (cl->majorversion < (GAMEVER / 256))
			older = true;
		else
		{
			if (cl->majorversion > (GAMEVER / 256))
				older = false;
			else
			{
				if (cl->minorversion < (GAMEVER % 256))
					older = true;
				else if (cl->minorversion > (GAMEVER % 256))
					older = false;
			}
		}

		// GhostlyDeath -- Print message depending on older or newer
		if (older)
		{
			FormattedString <<
                "For updates, visit http://odamex.net/" <<
                std::endl;
		}
		else
        {
			FormattedString <<
                "If a new version just came out, " <<
                "give server administrators time to update their servers." <<
                std::endl;
        }

		// GhostlyDeath -- email address set?
		if (*(sv_email.cstring()))
		{
			char emailbuf[100];
			memset(emailbuf, 0, sizeof(emailbuf));
			const char* in = sv_email.cstring();
			char* out = emailbuf;

			for (int i = 0; i < 100 && *in; i++, in++, out++)
				*out = *in;

			FormattedString <<
                "If problems persist, contact the server administrator at " <<
                emailbuf <<
                std::endl;
		}

		// GhostlyDeath -- Now we tell them our built up message and boot em
		cl->displaydisconnect = false;	// Don't spam the players

		MSG_WriteMarker(&cl->reliablebuf, svc_print);
		MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString(&cl->reliablebuf,
                        (const char *)FormattedString.str().c_str());

		MSG_WriteMarker(&cl->reliablebuf, svc_disconnect);

		SV_SendPacket(*it);

		// GhostlyDeath -- And we tell the server
		Printf(PRINT_HIGH,
                "%s -- Version mismatch (%s != %s)\n",
                NET_AdrToString(net_from),
                VersionStr,
                OurVersionStr);
	}

	return AllowConnect;
}



//
//	SV_ConnectClient
//
//	Called when a client connects
//
void G_DoReborn (player_t &playernum);

void SV_ConnectClient()
{
	int challenge = MSG_ReadLong();

	// New querying system
	if (SV_QryParseEnquiry(challenge) == 0)
		return;

	if (challenge == LAUNCHER_CHALLENGE)  // for Launcher
	{
		SV_SendServerInfo();
		return;
	}

	if (challenge != CHALLENGE)
		return;

	if (!SV_IsValidToken(MSG_ReadLong()))
		return;

	Printf(PRINT_HIGH, "%s is trying to connect...\n", NET_AdrToString (net_from));

	// find an open slot
	Players::iterator it = SV_GetFreeClient();

	if (it == players.end()) // a server is full
	{
		Printf(PRINT_HIGH, "%s disconnected (server full).\n", NET_AdrToString (net_from));

		static buf_t smallbuf(16);
		MSG_WriteLong(&smallbuf, 0);
		MSG_WriteMarker(&smallbuf, svc_full);
		NET_SendPacket(smallbuf, net_from);

		return;
	}

	player_t* player = &(*it);
	client_t* cl = &(player->client);

	// clear client network info
	cl->address = net_from;
	cl->last_received = gametic;
	cl->reliable_bps = 0;
	cl->unreliable_bps = 0;
	cl->lastcmdtic = 0;
	cl->lastclientcmdtic = 0;
	cl->allow_rcon = false;
	cl->displaydisconnect = false;

	// generate a random string
	std::stringstream ss;
	ss << time(NULL) << level.time << VERSION << NET_AdrToString(net_from);
	cl->digest = MD5SUM(ss.str());

	// Set player time
	player->JoinTime = time(NULL);

	SZ_Clear(&cl->netbuf);
	SZ_Clear(&cl->reliablebuf);
	SZ_Clear(&cl->relpackets);

	memset(cl->packetseq, -1, sizeof(cl->packetseq));
	memset(cl->packetbegin, 0, sizeof(cl->packetbegin));
	memset(cl->packetsize, 0, sizeof(cl->packetsize));

	cl->sequence = 0;
	cl->last_sequence = -1;
	cl->packetnum =  0;

	cl->version = MSG_ReadShort();
	byte connection_type = MSG_ReadByte();

	// [SL] 2011-05-11 - Register the player with the reconciliation system
	// for unlagging
	Unlag::getInstance().registerPlayer(player->id);

	if (!SV_CheckClientVersion(cl, it))
	{
		SV_DropClient(*player);
		return;
	}

	// get client userinfo
	clc_t userinfo = (clc_t)MSG_ReadByte();
	if (userinfo != clc_userinfo)
	{
		SV_InvalidateClient(*player, "Client didn't send any userinfo");
		return;
	}

	if (!SV_SetupUserInfo(*player))
		return;

	// get rate value
	SV_SetClientRate(*cl, MSG_ReadLong());

	if (SV_BanCheck(cl))
	{
		cl->displaydisconnect = false;
		SV_DropClient(*player);
		return;
	}

	std::string passhash = MSG_ReadString();
	if (strlen(join_password.cstring()) && MD5SUM(join_password.cstring()) != passhash)
	{
		Printf(PRINT_HIGH, "%s disconnected (password failed).\n", NET_AdrToString(net_from));

		MSG_WriteMarker(&cl->reliablebuf, svc_print);
		MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString(&cl->reliablebuf, "Server is passworded, no password specified or bad password\n");

		SV_SendPacket(*player);
		SV_DropClient(*player);
		return;
	}

	// send consoleplayer number
	MSG_WriteMarker(&cl->reliablebuf, svc_consoleplayer);
	MSG_WriteByte(&cl->reliablebuf, player->id);
	MSG_WriteString(&cl->reliablebuf, cl->digest.c_str());
	SV_SendPacket(*player);

	// [Toke] send server settings
	SV_SendServerSettings(*player);

	cl->displaydisconnect = true;

	cl->download.name = "";
	if (connection_type == 1)
	{
		if (sv_waddownload)
		{
			player->playerstate = PST_DOWNLOAD;
			SV_BroadcastUserInfo(*player);
			SV_BroadcastPrintf(PRINT_HIGH, "%s has connected. (downloading)\n", player->userinfo.netname.c_str());

			// send the client the scores and list of other clients
			SV_ClientFullUpdate(*player);

			for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
			{
				// [SL] 2011-07-30 - clients should consider downloaders as spectators
				MSG_WriteMarker(&pit->client.reliablebuf, svc_spectate);
				MSG_WriteByte(&pit->client.reliablebuf, player->id);
				MSG_WriteByte(&pit->client.reliablebuf, true);
			}
		}
		else
		{
			// Downloading is not allowed. Just kick the client and don't
			// bother telling anyone else
			cl->displaydisconnect = false;

			Printf(PRINT_HIGH, "%s has connected. (downloading)\n", player->userinfo.netname.c_str());

			MSG_WriteMarker(&cl->reliablebuf, svc_print);
			MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
			MSG_WriteString(&cl->reliablebuf, "Server: Downloading is disabled.\n");

			SV_DropClient(*player);

			Printf(PRINT_HIGH, "%s disconnected. Downloading is disabled.\n", player->userinfo.netname.c_str());
		}

		return;
	}

	SV_BroadcastUserInfo(*player);
	player->playerstate = PST_REBORN;

	player->fragcount = 0;
	player->killcount = 0;
	player->points = 0;

	if (!step_mode)
	{
		player->spectator = true;
		for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
		{
			MSG_WriteMarker(&pit->client.reliablebuf, svc_spectate);
			MSG_WriteByte(&pit->client.reliablebuf, player->id);
			MSG_WriteByte(&pit->client.reliablebuf, true);
		}
	}

	// send a map name
	SV_SendLoadMap(wadfiles, patchfiles, level.mapname, player);

	// [SL] 2011-12-07 - Force the player to jump to intermission if not in a level
	if (gamestate == GS_INTERMISSION)
		MSG_WriteMarker(&cl->reliablebuf, svc_exitlevel);

	G_DoReborn(*player);
	SV_ClientFullUpdate(*player);

	SV_BroadcastPrintf(PRINT_HIGH, "%s has connected.\n", player->userinfo.netname.c_str());

	// tell others clients about it
	for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
	{
		MSG_WriteMarker(&pit->client.reliablebuf, svc_connectclient);
		MSG_WriteByte(&pit->client.reliablebuf, player->id);
	}

	SV_MidPrint((char*)sv_motd.cstring(), player, 6);
}

extern bool singleplayerjustdied;

//
// SV_DisconnectClient
//
void SV_DisconnectClient(player_t &who)
{
	char str[100];
	std::string disconnectmessage;

	// already gone though this procedure?
	if (who.playerstate == PST_DISCONNECT)
		return;

	// tell others clients about it
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
	   client_t &cl = it->client;
	   MSG_WriteMarker(&cl.reliablebuf, svc_disconnectclient);
	   MSG_WriteByte(&cl.reliablebuf, who.id);
	}

	Maplist_Disconnect(who);
	Vote_Disconnect(who);

	if (who.client.displaydisconnect)
	{
		// print some final stats for the disconnected player
		std::string status;
		if (who.playerstate == PST_DOWNLOAD)
			status = "downloading";
		else if (who.spectator)
			status = "SPECTATOR";
		else
		{
			if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			{
				sprintf(str, "%s TEAM, ", team_names[who.userinfo.team]);
				status += str;
			}

			// Points (CTF).
			if (sv_gametype == GM_CTF)
			{
				sprintf(str, "%d POINTS, ", who.points);
				status += str;
			}

			// Frags (DM/TDM/CTF) or Kills (Coop).
			if (sv_gametype == GM_COOP)
				sprintf(str, "%d KILLS, ", who.killcount);
			else
				sprintf(str, "%d FRAGS, ", who.fragcount);

			status += str;

			// Deaths.
			sprintf(str, "%d DEATHS", who.deathcount);
			status += str;
		}

		// Name and reason for disconnect.
		if (gametic - who.client.last_received == CLIENT_TIMEOUT*35)
			SV_BroadcastPrintf(PRINT_HIGH, "%s timed out. (%s)\n",
							who.userinfo.netname.c_str(), status.c_str());
		else
			SV_BroadcastPrintf(PRINT_HIGH, "%s disconnected. (%s)\n",
							who.userinfo.netname.c_str(), status.c_str());
	}

	who.playerstate = PST_DISCONNECT;
}


//
// SV_DropClient
// Called when the player is leaving the server unwillingly.
//
void SV_DropClient(player_t &who)
{
	client_t *cl = &who.client;

	MSG_WriteMarker(&cl->reliablebuf, svc_disconnect);

	SV_SendPacket(who);

	SV_DisconnectClient(who);
}

//
// SV_SendDisconnectSignal
//
void SV_SendDisconnectSignal()
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteMarker(&cl->reliablebuf, svc_disconnect);
		SV_SendPacket(*it);

		if (it->mo)
			it->mo->Destroy();
	}

	players.clear();
}

//
// SV_SendReconnectSignal
// All clients will reconnect. Called when the server changes a map
//
void SV_SendReconnectSignal()
{
	// tell others clients about it
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		MSG_WriteMarker(&(it->client.reliablebuf), svc_reconnect);
		SV_SendPacket(*it);

		if (it->mo)
			it->mo->Destroy();
	}

	players.clear();
}

//
// SV_ExitLevel
// Called when sv_timelimit or sv_fraglimit hit.
//
void SV_ExitLevel()
{
	for (Players::iterator it = players.begin();it != players.end();++it)
		MSG_WriteMarker(&(it->client.reliablebuf), svc_exitlevel);
}

//
// SV_SendLoadMap
//
// Sends a message to a player telling them to change to the specified WAD
// and DEH patch files and load a map.
//
void SV_SendLoadMap(const std::vector<std::string> &wadnames,
					const std::vector<std::string> &patchnames,
					const std::string &mapname, player_t *player)
{
	if (!player)
		return;

	buf_t *buf = &(player->client.reliablebuf);
	MSG_WriteMarker(buf, svc_loadmap);

	// send list of wads (skip over wadnames[0] == odamex.wad)
	MSG_WriteByte(buf, MIN<size_t>(wadnames.size() - 1, 255));
	for (size_t i = 1; i < MIN<size_t>(wadnames.size(), 256); i++)
	{
		MSG_WriteString(buf, D_CleanseFileName(wadnames[i], "wad").c_str());
		MSG_WriteString(buf, wadhashes[i].c_str());
	}

	// send list of DEH/BEX patches
	MSG_WriteByte(buf, MIN<size_t>(patchnames.size(), 255));
	for (size_t i = 0; i < MIN<size_t>(patchnames.size(), 255); i++)
	{
		MSG_WriteString(buf, D_CleanseFileName(patchnames[i]).c_str());
		MSG_WriteString(buf, patchhashes[i].c_str());
	}

	MSG_WriteString(buf, mapname.c_str());
}


//
// Comparison functors for sorting vectors of players
//
struct compare_player_frags
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->fragcount < arg1->fragcount;
	}
};

struct compare_player_kills
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->killcount < arg1->killcount;
	}
};

struct compare_player_points
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->points < arg1->points;
	}
};

struct compare_player_names
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg1->userinfo.netname < arg2->userinfo.netname;
	}
};


static float SV_CalculateKillDeathRatio(const player_t* player)
{
	if (player->killcount <= 0)	// Copied from HU_DMScores1.
		return 0.0f;
	else if (player->killcount >= 1 && player->deathcount == 0)
		return float(player->killcount);
	else
		return float(player->killcount) / float(player->deathcount);
}

static float SV_CalculateFragDeathRatio(const player_t* player)
{
	if (player->fragcount <= 0)	// Copied from HU_DMScores1.
		return 0.0f;
	else if (player->fragcount >= 1 && player->deathcount == 0)
		return float(player->fragcount);
	else
		return float(player->fragcount) / float(player->deathcount);
}

//
// SV_DrawScores
// Draws scoreboard to console. Used during level exit or a command.
//
// [AM] Commonize this with client.
//
void SV_DrawScores()
{
	char str[1024];

	typedef std::list<const player_t*> PlayerPtrList;
	PlayerPtrList sortedplayers;
	PlayerPtrList sortedspectators;

	for (Players::const_iterator it = players.begin(); it != players.end(); ++it)
		if (!it->spectator && it->ingame())
			sortedplayers.push_back(&*it);
		else
			sortedspectators.push_back(&*it);

	Printf(PRINT_HIGH, "\n");

	if (sv_gametype == GM_CTF)
	{
		compare_player_points comparison_functor;
		sortedplayers.sort(comparison_functor);

        Printf_Bold("                    CAPTURE THE FLAG");
        Printf_Bold("-----------------------------------------------------------");

		if (sv_scorelimit)
			sprintf(str, "Scorelimit: %-6d", sv_scorelimit.asInt());
		else
			sprintf(str, "Scorelimit: N/A   ");

		Printf_Bold("%s  ", str);

		if (sv_timelimit)
			sprintf(str, "Timelimit: %-7d", sv_timelimit.asInt());
		else
			sprintf(str, "Timelimit: N/A");

		Printf_Bold("%18s\n", str);

		for (int team_num = 0; team_num < NUMTEAMS; team_num++)
		{
			if (team_num == TEAM_BLUE)
                Printf_Bold("--------------------------------------------------BLUE TEAM");
			else if (team_num == TEAM_RED)
                Printf_Bold("---------------------------------------------------RED TEAM");
			else		// shouldn't happen
                Printf_Bold("-----------------------------------------------UNKNOWN TEAM");

            Printf_Bold("ID  Address          Name            Points Caps Frags Time");
            Printf_Bold("-----------------------------------------------------------");

			for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
			{
				const player_t* itplayer = *it;
				if (itplayer->userinfo.team == team_num)
				{
					Printf_Bold("%-3d %-16s %-15s %-6d N/A  %-5d %-3d",
							itplayer->id,
							NET_AdrToString(itplayer->client.address),
							itplayer->userinfo.netname.c_str(),
							itplayer->points,
							//itplayer->captures,
							itplayer->fragcount,
							itplayer->GameTime / 60);
				}
			}
		}
	}

	else if (sv_gametype == GM_TEAMDM)
	{
		compare_player_frags comparison_functor;
		sortedplayers.sort(comparison_functor);

        Printf_Bold("                     TEAM DEATHMATCH");
        Printf_Bold("-----------------------------------------------------------");

		if (sv_fraglimit)
			sprintf(str, "Fraglimit: %-7d", sv_fraglimit.asInt());
		else
			sprintf(str, "Fraglimit: N/A    ");

		Printf_Bold("%s  ", str);

		if (sv_timelimit)
			sprintf(str, "Timelimit: %-7d", sv_timelimit.asInt());
		else
			sprintf(str, "Timelimit: N/A");

		Printf_Bold("%18s\n", str);

		for (int team_num = 0; team_num < NUMTEAMS; team_num++)
		{
			if (team_num == TEAM_BLUE)
                Printf_Bold("--------------------------------------------------BLUE TEAM");
			else if (team_num == TEAM_RED)
                Printf_Bold("---------------------------------------------------RED TEAM");
			else		// shouldn't happen
                Printf_Bold("-----------------------------------------------UNKNOWN TEAM");

            Printf_Bold("ID  Address          Name            Frags Deaths  K/D Time");
            Printf_Bold("-----------------------------------------------------------");

			for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
			{
				const player_t* itplayer = *it;
				if (itplayer->userinfo.team == team_num)
				{
					Printf_Bold("%-3d %-16s %-15s %-5d %-6d %2.1f %-3d",
							itplayer->id,
							NET_AdrToString(itplayer->client.address),
							itplayer->userinfo.netname.c_str(),
							itplayer->fragcount,
							itplayer->deathcount,
							SV_CalculateFragDeathRatio(itplayer),
							itplayer->GameTime / 60);
				}
			}
		}
	}

	else if (sv_gametype == GM_DM)
	{
		compare_player_frags comparison_functor;
		sortedplayers.sort(comparison_functor);

        Printf_Bold("                        DEATHMATCH");
        Printf_Bold("-----------------------------------------------------------");

		if (sv_fraglimit)
			sprintf(str, "Fraglimit: %-7d", sv_fraglimit.asInt());
		else
			sprintf(str, "Fraglimit: N/A    ");

		Printf_Bold("%s  ", str);

		if (sv_timelimit)
			sprintf(str, "Timelimit: %-7d", sv_timelimit.asInt());
		else
			sprintf(str, "Timelimit: N/A");

		Printf_Bold("%18s\n", str);

        Printf_Bold("ID  Address          Name            Frags Deaths  K/D Time");
        Printf_Bold("-----------------------------------------------------------");

		for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
		{
			const player_t* itplayer = *it;
			Printf_Bold("%-3d %-16s %-15s %-5d %-6d %2.1f %-3d",
					itplayer->id,
					NET_AdrToString(itplayer->client.address),
					itplayer->userinfo.netname.c_str(),
					itplayer->fragcount,
					itplayer->deathcount,
					SV_CalculateFragDeathRatio(itplayer),
					itplayer->GameTime / 60);
		}

	}

	else if (sv_gametype == GM_COOP)
	{
		compare_player_kills comparison_functor;
		sortedplayers.sort(comparison_functor);

        Printf_Bold("                       COOPERATIVE");
        Printf_Bold("-----------------------------------------------------------");
        Printf_Bold("ID  Address          Name            Kills Deaths  K/D Time");
        Printf_Bold("-----------------------------------------------------------");

		for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
		{
			const player_t* itplayer = *it;
			Printf_Bold("%-3d %-16s %-15s %-5d %-6d %2.1f %-3d",
					itplayer->id,
					NET_AdrToString(itplayer->client.address),
					itplayer->userinfo.netname.c_str(),
					itplayer->killcount,
					itplayer->deathcount,
					SV_CalculateKillDeathRatio(itplayer),
					itplayer->GameTime / 60);
		}
	}

	if (!sortedspectators.empty())
	{
		compare_player_names comparison_functor;
		sortedspectators.sort(comparison_functor);

    	Printf_Bold("-------------------------------------------------SPECTATORS");

		for (PlayerPtrList::const_iterator it = sortedspectators.begin(); it != sortedspectators.end(); ++it)
		{
			const player_t* itplayer = *it;
			Printf_Bold("%-3d %-16s %-15s\n",
					itplayer->id,
					NET_AdrToString(itplayer->client.address),
					itplayer->userinfo.netname.c_str());
		}
	}

	Printf(PRINT_HIGH, "\n");
}

BEGIN_COMMAND (showscores)
{
    SV_DrawScores();
}
END_COMMAND (showscores)

//
// SV_BroadcastPrintf
// Sends text to all active clients.
//
void STACK_ARGS SV_BroadcastPrintf(int level, const char *fmt, ...)
{
	va_list argptr;
	char string[2048];
	client_t *cl;

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	Printf(level, "%s", string);  // print to the console

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		cl = &(it->client);

		if (cl->allow_rcon) // [mr.crispy -- sept 23 2013] RCON guy already got it when it printed to the console
			continue;

		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, level);
		MSG_WriteString (&cl->reliablebuf, string);
	}
}

// GhostlyDeath -- same as above but ONLY for spectators
void STACK_ARGS SV_SpectatorPrintf(int level, const char *fmt, ...)
{
	va_list argptr;
	char string[2048];
	client_t *cl;

	va_start(argptr,fmt);
	vsprintf(string, fmt,argptr);
	va_end(argptr);

	Printf(level, "%s", string);  // print to the console

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		cl = &(it->client);

		if (cl->allow_rcon) // [mr.crispy -- sept 23 2013] RCON guy already got it when it printed to the console
			continue;

		bool spectator = it->spectator || !it->ingame();
		if (spectator)
		{
			MSG_WriteMarker(&cl->reliablebuf, svc_print);
			MSG_WriteByte(&cl->reliablebuf, level);
			MSG_WriteString(&cl->reliablebuf, string);
		}
	}
}

// Print directly to a specific client.
void STACK_ARGS SV_ClientPrintf(client_t *cl, int level, const char *fmt, ...)
{
	va_list argptr;
	char string[2048];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	MSG_WriteMarker(&cl->reliablebuf, svc_print);
	MSG_WriteByte(&cl->reliablebuf, level);
	MSG_WriteString(&cl->reliablebuf, string);
}

// Print directly to a specific player.
void STACK_ARGS SV_PlayerPrintf(int level, int player_id, const char *fmt, ...)
{
	va_list argptr;
	char string[2048];

	va_start(argptr,fmt);
	vsprintf(string, fmt,argptr);
	va_end(argptr);

	client_t* cl = &idplayer(player_id).client;
	MSG_WriteMarker(&cl->reliablebuf, svc_print);
	MSG_WriteByte(&cl->reliablebuf, level);
	MSG_WriteString(&cl->reliablebuf, string);
}

void STACK_ARGS SV_TeamPrintf(int level, int who, const char *fmt, ...)
{
	if (sv_gametype != GM_TEAMDM && sv_gametype != GM_CTF)
		return;

	va_list argptr;
	char string[2048];

	va_start(argptr,fmt);
	vsprintf(string, fmt,argptr);
	va_end(argptr);

	Printf(level, "%s", string);  // print to the console

	player_t* player = &idplayer(who);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (it->userinfo.team != player->userinfo.team)
			continue;

		bool spectator = it->spectator || !it->ingame();
		if (spectator)
			continue;

		client_t* cl = &(it->client);

		if (cl->allow_rcon) // [mr.crispy -- sept 23 2013] RCON guy already got it when it printed to the console
			continue;

		MSG_WriteMarker(&cl->reliablebuf, svc_print);
		MSG_WriteByte(&cl->reliablebuf, level);
		MSG_WriteString(&cl->reliablebuf, string);
	}
}

/**
 * Send a message to teammates of a player.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
void SVC_TeamSay(player_t &player, const char* message)
{
	char team[5] = { 0 };
	if (player.userinfo.team == TEAM_BLUE)
		sprintf(team, "BLUE");
	else if (player.userinfo.team == TEAM_RED)
		sprintf(team, "RED");

	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_TEAMCHAT, "<%s TEAM> * %s %s\n", team, player.userinfo.netname.c_str(), &message[4]);
	else
		Printf(PRINT_TEAMCHAT, "<%s TEAM> %s: %s\n", team, player.userinfo.netname.c_str(), message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		bool spectator = it->spectator || !it->ingame();

		// Player needs to be on the same team
		if (spectator || it->userinfo.team != player.userinfo.team)
			continue;

		MSG_WriteMarker(&(it->client.reliablebuf), svc_say);
		MSG_WriteByte(&(it->client.reliablebuf), 1);
		MSG_WriteByte(&(it->client.reliablebuf), player.id);
		MSG_WriteString(&(it->client.reliablebuf), message);
	}
}

/**
 * Send a message to all spectators.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
void SVC_SpecSay(player_t &player, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_TEAMCHAT, "<SPEC> * %s %s\n", player.userinfo.netname.c_str(), &message[4]);
	else
		Printf(PRINT_TEAMCHAT, "<SPEC> %s: %s\n", player.userinfo.netname.c_str(), message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		bool spectator = it->spectator || !it->ingame();

		if (!spectator)
			continue;

		MSG_WriteMarker(&(it->client.reliablebuf), svc_say);
		MSG_WriteByte(&(it->client.reliablebuf), 1);
		MSG_WriteByte(&(it->client.reliablebuf), player.id);
		MSG_WriteString(&(it->client.reliablebuf), message);
	}
}

/**
 * Send a message to everybody.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
void SVC_Say(player_t &player, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_CHAT, "<CHAT> * %s %s\n", player.userinfo.netname.c_str(), &message[4]);
	else
		Printf(PRINT_CHAT, "<CHAT> %s: %s\n", player.userinfo.netname.c_str(), message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		MSG_WriteMarker(&(it->client.reliablebuf), svc_say);
		MSG_WriteByte(&(it->client.reliablebuf), 0);
		MSG_WriteByte(&(it->client.reliablebuf), player.id);
		MSG_WriteString(&(it->client.reliablebuf), message);
	}
}

/**
 * Send a message to a specific player from a specific other player.
 *
 * @param player  Sending player.
 * @param dplayer Player to send to.
 * @param message Message to send.
 */
void SVC_PrivMsg(player_t &player, player_t &dplayer, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		Printf(PRINT_CHAT, "<PRIVMSG> * %s (to %s) %s\n",
				player.userinfo.netname.c_str(), dplayer.userinfo.netname.c_str(), &message[4]);
	else
		Printf(PRINT_CHAT, "<PRIVMSG> %s (to %s): %s\n",
				player.userinfo.netname.c_str(), dplayer.userinfo.netname.c_str(), message);

	MSG_WriteMarker(&dplayer.client.reliablebuf, svc_say);
	MSG_WriteByte(&dplayer.client.reliablebuf, 1);
	MSG_WriteByte(&dplayer.client.reliablebuf, player.id);
	MSG_WriteString(&dplayer.client.reliablebuf, message);

	// [AM] Send a duplicate message to the sender, so he knows the message
	//      went through.
	if (player.id != dplayer.id)
	{
		MSG_WriteMarker(&player.client.reliablebuf, svc_say);
		MSG_WriteByte(&player.client.reliablebuf, 1);
		MSG_WriteByte(&player.client.reliablebuf, player.id);
		MSG_WriteString(&player.client.reliablebuf, message);
	}
}

//
// SV_Say
// Show a chat string and send it to others clients.
//
bool SV_Say(player_t &player)
{
	byte message_visibility = MSG_ReadByte();

	std::string message(MSG_ReadString());
	StripColorCodes(message);

	if (!ValidString(message))
	{
		SV_InvalidateClient(player, "Chatstring contains invalid characters");
		return false;
	}

	if (message.empty() || message.length() > MAX_CHATSTR_LEN)
		return true;

	// Flood protection
	if (player.LastMessage.Time)
	{
		const dtime_t min_diff = I_ConvertTimeFromMs(1000) * sv_flooddelay;
		dtime_t diff = I_GetTime() - player.LastMessage.Time;

		if (diff < min_diff)
			return true;

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
			SVC_SpecSay(player, message.c_str());
		else
			SVC_Say(player, message.c_str());
	}
	else if (message_visibility == 1)
	{
		if (spectator)
			SVC_SpecSay(player, message.c_str());
		else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			SVC_TeamSay(player, message.c_str());
		else
			SVC_Say(player, message.c_str());
	}

	return true;
}

//
// SV_PrivMsg
// Show a chat string and show it to a single other client.
//
bool SV_PrivMsg(player_t &player)
{
	player_t& dplayer = idplayer(MSG_ReadByte());

	std::string str(MSG_ReadString());
	StripColorCodes(str);

	if (!ValidString(str))
	{
		SV_InvalidateClient(player, "Private Message contains invalid characters");
		return false;
	}

	if (!validplayer(dplayer))
		return true;

	if (str.empty() || str.length() > MAX_CHATSTR_LEN)
		return true;

	// In competitive gamemodes, don't allow spectators to message players.
	if (sv_gametype != GM_COOP && player.spectator && !dplayer.spectator)
		return true;

	// Flood protection
	if (player.LastMessage.Time)
	{
		const dtime_t min_diff = I_ConvertTimeFromMs(1000) * sv_flooddelay;
		dtime_t diff = I_GetTime() - player.LastMessage.Time;

		if (diff < min_diff)
			return true;

		player.LastMessage.Time = 0;
	}

	if (player.LastMessage.Time == 0)
	{
		player.LastMessage.Time = I_GetTime();
		player.LastMessage.Message = str;
	}

	SVC_PrivMsg(player, dplayer, str.c_str());

	return true;
}

//
// SV_UpdateMissiles
// Updates missiles position sometimes.
//
void SV_UpdateMissiles(player_t &pl)
{
    AActor *mo;

    TThinkerIterator<AActor> iterator;
    while ( (mo = iterator.Next() ) )
    {
        if (!(mo->flags & MF_MISSILE || mo->flags & MF_SKULLFLY))
			continue;

		if (mo->type == MT_PLASMA)
			continue;

		// update missile position every 30 tics
		if (((gametic+mo->netid) % 30) && (mo->type != MT_TRACER) && (mo->type != MT_FATSHOT))
			continue;
		// Revenant tracers and Mancubus fireballs need to be  updated more often
		else if (((gametic+mo->netid) % 5) && (mo->type == MT_TRACER || mo->type == MT_FATSHOT))
			continue;

		if(SV_IsPlayerAllowedToSee(pl, mo))
		{
			client_t *cl = &pl.client;

			MSG_WriteMarker (&cl->netbuf, svc_movemobj);
			MSG_WriteShort (&cl->netbuf, mo->netid);
			MSG_WriteByte (&cl->netbuf, mo->rndindex);
			MSG_WriteLong (&cl->netbuf, mo->x);
			MSG_WriteLong (&cl->netbuf, mo->y);
			MSG_WriteLong (&cl->netbuf, mo->z);

			MSG_WriteMarker (&cl->netbuf, svc_mobjspeedangle);
			MSG_WriteShort(&cl->netbuf, mo->netid);
			MSG_WriteLong (&cl->netbuf, mo->angle);
			MSG_WriteLong (&cl->netbuf, mo->momx);
			MSG_WriteLong (&cl->netbuf, mo->momy);
			MSG_WriteLong (&cl->netbuf, mo->momz);

			if (mo->tracer)
			{
				MSG_WriteMarker (&cl->netbuf, svc_actor_tracer);
				MSG_WriteShort(&cl->netbuf, mo->netid);
				MSG_WriteShort (&cl->netbuf, mo->tracer->netid);
			}

            if (cl->netbuf.cursize >= 1024)
                if(!SV_SendPacket(pl))
                    return;
		}
    }
}

// Update the given actors state immediately.
void SV_UpdateMobjState(AActor *mo)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		if (SV_IsPlayerAllowedToSee(*it, mo))
		{
			client_t *cl = &(it->client);
			statenum_t mostate = (statenum_t)(mo->state - states);

			MSG_WriteMarker(&cl->reliablebuf, svc_mobjstate);
			MSG_WriteShort(&cl->reliablebuf, mo->netid);
			MSG_WriteShort(&cl->reliablebuf, (short)mostate);
		}
	}
}

// Keep tabs on monster positions and angles.
void SV_UpdateMonsters(player_t &pl)
{
	AActor *mo;

	TThinkerIterator<AActor> iterator;
	while ((mo = iterator.Next()))
	{
		// Ignore corpses.
		if (mo->flags & MF_CORPSE)
			continue;

		// We don't handle updating non-monsters here.
		if (!(mo->flags & MF_COUNTKILL || mo->type == MT_SKULL))
			continue;

		// update monster position every 7 tics
		if ((gametic+mo->netid) % 7)
			continue;

		if (SV_IsPlayerAllowedToSee(pl, mo) && mo->target)
		{
			client_t *cl = &pl.client;

			MSG_WriteMarker(&cl->netbuf, svc_movemobj);
			MSG_WriteShort(&cl->netbuf, mo->netid);
			MSG_WriteByte(&cl->netbuf, mo->rndindex);
			MSG_WriteLong(&cl->netbuf, mo->x);
			MSG_WriteLong(&cl->netbuf, mo->y);
			MSG_WriteLong(&cl->netbuf, mo->z);

			MSG_WriteMarker(&cl->netbuf, svc_mobjspeedangle);
			MSG_WriteShort(&cl->netbuf, mo->netid);
			MSG_WriteLong(&cl->netbuf, mo->angle);
			MSG_WriteLong(&cl->netbuf, mo->momx);
			MSG_WriteLong(&cl->netbuf, mo->momy);
			MSG_WriteLong(&cl->netbuf, mo->momz);

			MSG_WriteMarker(&cl->netbuf, svc_actor_movedir);
			MSG_WriteShort(&cl->netbuf, mo->netid);
			MSG_WriteByte(&cl->netbuf, mo->movedir);
			MSG_WriteLong(&cl->netbuf, mo->movecount);

			MSG_WriteMarker(&cl->netbuf, svc_actor_target);
			MSG_WriteShort(&cl->netbuf, mo->netid);
			MSG_WriteShort(&cl->netbuf, mo->target->netid);

			if (cl->netbuf.cursize >= 1024)
			{
				if (!SV_SendPacket(pl))
					return;
			}
		}
	}
}

//
// SV_ActorTarget
//
void SV_ActorTarget(AActor *actor)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		client_t *cl = &(it->client);

		if(!SV_IsPlayerAllowedToSee(*it, actor))
			continue;

		MSG_WriteMarker (&cl->reliablebuf, svc_actor_target);
		MSG_WriteShort (&cl->reliablebuf, actor->netid);
		MSG_WriteShort (&cl->reliablebuf, actor->target ? actor->target->netid : 0);
	}
}

//
// SV_ActorTracer
//
void SV_ActorTracer(AActor *actor)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		client_t *cl = &(it->client);

		MSG_WriteMarker (&cl->reliablebuf, svc_actor_tracer);
		MSG_WriteShort (&cl->reliablebuf, actor->netid);
		MSG_WriteShort (&cl->reliablebuf, actor->tracer ? actor->tracer->netid : 0);
	}
}

//
// SV_RemoveCorpses
// Removes some corpses
//
void SV_RemoveCorpses (void)
{
	AActor *mo;
	int     corpses = 0;

	// joek - Number of corpses infinite
	if(sv_maxcorpses <= 0)
		return;

	if (!P_AtInterval(TICRATE))
		return;
	else
	{
		TThinkerIterator<AActor> iterator;
		while ( (mo = iterator.Next() ) )
		{
			if (mo->type == MT_PLAYER && (!mo->player || mo->health <=0) )
				corpses++;
		}
	}

	TThinkerIterator<AActor> iterator;
	while (corpses > sv_maxcorpses && (mo = iterator.Next() ) )
	{
		if (mo->type == MT_PLAYER && !mo->player)
		{
			mo->Destroy();
			corpses--;
		}
	}
}

//
// SV_SendPingRequest
// Pings the client and requests a reply
//
// [SL] 2011-05-11 - Changed from SV_SendGametic to SV_SendPingRequest
//
void SV_SendPingRequest(client_t* cl)
{
	if (!P_AtInterval(100))
		return;

	MSG_WriteMarker (&cl->reliablebuf, svc_pingrequest);
	MSG_WriteLong (&cl->reliablebuf, I_MSTime());
}

// calculates ping using gametic which was sent by SV_SendGametic and
// current gametic
void SV_CalcPing(player_t &player)
{
	unsigned int ping = I_MSTime() - MSG_ReadLong();

	if(ping > 999)
		ping = 999;

	player.ping = ping;
}

//
// SV_UpdatePing
// send pings to a client
//
void SV_UpdatePing(client_t* cl)
{
	if (!P_AtInterval(101))
		return;

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (!(it->ingame()))
			continue;

		MSG_WriteMarker(&cl->reliablebuf, svc_updateping);
		MSG_WriteByte(&cl->reliablebuf, it->id);  // player
		MSG_WriteLong(&cl->reliablebuf, it->ping);
	}
}


//
// SV_UpdateDeadPlayers
// Update player's frame while he's dying.
//
void SV_UpdateDeadPlayers()
{
 /*   AActor *mo;

    TThinkerIterator<AActor> iterator;
    while ( (mo = iterator.Next() ) )
    {
        if (mo->type != MT_PLAYER || mo->player)
			continue;

		if (mo->oldframe != mo->frame)
			for (size_t i = 0; i < players.size(); i++)
			{
				client_t *cl = &clients[i];

				MSG_WriteMarker (&cl->reliablebuf, svc_mobjframe);
				MSG_WriteShort (&cl->reliablebuf, mo->netid);
				MSG_WriteByte (&cl->reliablebuf, mo->frame);
			}

		mo->oldframe = mo->frame;
    }
*/
}


//
// SV_ClearClientsBPS
//
void SV_ClearClientsBPS(void)
{
	if (!P_AtInterval(TICRATE))
		return;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		cl->reliable_bps = 0;
		cl->unreliable_bps = 0;
	}
}

//
// SV_SendPackets
//
void SV_SendPackets()
{
	if (players.empty())
		return;

	static size_t fair_send = 0;
	size_t num_players = players.size();

	// Wrap the starting index around if necessary.
	if (fair_send >= num_players)
		fair_send = 0;

	// Shift the starting point.
	Players::iterator begin = players.begin();
	for (size_t i = 0;i < fair_send;i++)
		++begin;

	// Loop through all players in a staggered fashion.
	Players::iterator it = begin;
	do
	{
		SV_SendPacket(*it);

		++it;
		if (it == players.end())
			it = players.begin();
	}
	while (it != begin);

	// Advance the send index.
	fair_send++;
}

void SV_SendPlayerStateUpdate(client_t *client, player_t *player)
{
	if (!client || !player || !player->mo)
		return;

	buf_t *buf = &client->netbuf;

	MSG_WriteMarker(buf, svc_playerstate);
	MSG_WriteByte(buf, player->id);
	MSG_WriteShort(buf, player->health);
	MSG_WriteByte(buf, player->armortype);
	MSG_WriteShort(buf, player->armorpoints);

	MSG_WriteByte(buf, player->readyweapon);

	for (int i = 0; i < NUMAMMO; i++)
		MSG_WriteShort(buf, player->ammo[i]);

	for (int i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t *psp = &player->psprites[i];
		if (psp->state)
			MSG_WriteByte(buf, psp->state - states);
		else
			MSG_WriteByte(buf, 0xFF);
	}
}

void SV_SpyPlayer(player_t &viewer)
{
	byte id = MSG_ReadByte();

	player_t &other = idplayer(id);
	if (!validplayer(other) || !P_CanSpy(viewer, other))
		return;

	viewer.spying = id;
	SV_SendPlayerStateUpdate(&viewer.client, &other);
}

//
// SV_WriteCommands
//
void SV_WriteCommands(void)
{
	// [SL] 2011-05-11 - Save player positions and moving sector heights so
	// they can be reconciled later for unlagging
	Unlag::getInstance().recordPlayerPositions();
	Unlag::getInstance().recordSectorPositions();

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		client_t *cl = &(it->client);

		// Don't need to update origin every tic.
		// The server sends origin and velocity of a
		// player and the client always knows origin on
		// on the next tic.
		// HOWEVER, update as often as the player requests
		if (P_AtInterval(it->userinfo.update_rate))
		{
			// [SL] 2011-05-11 - Send the client the server's gametic
			// this gametic is returned to the server with the client's
			// next cmd
			if (it->ingame())
				SV_SendGametic(cl);

			for (Players::iterator pit = players.begin();pit != players.end();++pit)
			{
				if (!(pit->ingame()) || !(pit->mo))
					continue;

				// a player is updated about their own position elsewhere
				if (&*it == &*pit)
					continue;

				// GhostlyDeath -- Screw spectators
				if (pit->spectator)
					continue;

				if(!SV_IsPlayerAllowedToSee(*it, pit->mo))
					continue;

				MSG_WriteMarker(&cl->netbuf, svc_moveplayer);
				MSG_WriteByte(&cl->netbuf, pit->id); // player number

				// [SL] 2011-09-14 - the most recently processed ticcmd from the
				// client we're sending this message to.
				MSG_WriteLong(&cl->netbuf, it->tic);

				MSG_WriteLong(&cl->netbuf, pit->mo->x);
				MSG_WriteLong(&cl->netbuf, pit->mo->y);
				MSG_WriteLong(&cl->netbuf, pit->mo->z);

				if (GAMEVER > 60)
				{
					MSG_WriteShort(&cl->netbuf, pit->mo->angle >> FRACBITS);
					MSG_WriteShort(&cl->netbuf, pit->mo->pitch >> FRACBITS);
				}
				else
				{
					MSG_WriteLong(&cl->netbuf, pit->mo->angle);
				}

				if (pit->mo->frame == 32773)
					MSG_WriteByte(&cl->netbuf, PLAYER_FULLBRIGHTFRAME);
				else
					MSG_WriteByte(&cl->netbuf, pit->mo->frame);

				// write velocity
				MSG_WriteLong(&cl->netbuf, pit->mo->momx);
				MSG_WriteLong(&cl->netbuf, pit->mo->momy);
				MSG_WriteLong(&cl->netbuf, pit->mo->momz);

				// [Russell] - hack, tell the client about the partial
				// invisibility power of another player.. (cheaters can disable
				// this but its all we have for now)
				if (GAMEVER > 60)
					MSG_WriteByte(&cl->netbuf, pit->powers[pw_invisibility]);
				else
					MSG_WriteLong(&cl->netbuf, pit->powers[pw_invisibility]);
			}
		}

		// [SL] Send client info about player he is spying on
		player_t *target = &idplayer(it->spying);
		if (validplayer(*target) && &(*it) != target && P_CanSpy(*it, *target))
			SV_SendPlayerStateUpdate(&(it->client), target);

		SV_UpdateHiddenMobj();

		SV_UpdateConsolePlayer(*it);

		SV_UpdateMissiles(*it);

		SV_UpdateMonsters(*it);

		SV_SendPingRequest(cl);     // request ping reply

		SV_UpdatePing(cl);          // send the ping value of all cients to this client
	}

	SV_UpdateDeadPlayers(); // Update dying players.
}

void SV_PlayerTriedToCheat(player_t &player)
{
	SV_BroadcastPrintf(PRINT_HIGH, "%s tried to cheat!\n", player.userinfo.netname.c_str());
	SV_DropClient(player);
}

//
// SV_FlushPlayerCmds
//
// Clears a player's queue of ticcmds, ignoring and discarding them
//
void SV_FlushPlayerCmds(player_t &player)
{
	while (!player.cmdqueue.empty())
		player.cmdqueue.pop();
}

//
// SV_CalculateNumTiccmds
//
// [SL] 2011-09-16 - Calculate how many ticcmds should be processed.  Under
// most circumstances, it should be 1 per gametic to have the smoothest
// player movement possible.
//
int SV_CalculateNumTiccmds(player_t &player)
{
	if (!player.mo || player.cmdqueue.empty())
		return 0;

	static const size_t maximum_queue_size = TICRATE / 4;

	if (!sv_ticbuffer || player.spectator || player.playerstate == PST_DEAD)
	{
		// Process all queued ticcmds.
		return maximum_queue_size;
	}
	if (player.mo->momx == 0 && player.mo->momy == 0 && player.mo->momz == 0)
	{
		// Player is not moving
		return 2;
	}
	if (player.cmdqueue.size() > 2 && gametic % 2*TICRATE == player.id % 2*TICRATE)
	{
		// Process an extra ticcmd once every 2 seconds to reduce the
		// queue size. Use player id to stagger the timing to prevent everyone
		// from running an extra ticcmd at the same time.
		return 2;
	}
	if (player.cmdqueue.size() > maximum_queue_size)
	{
		// The player experienced a large latency spike so try to catch up by
		// processing more than one ticcmd at the expense of appearing perfectly
		//  smooth
		return 2;
	}

	// always run at least 1 ticcmd if possible
	return 1;
}

//
// SV_ProcessPlayerCmd
//
// Decides how many of a player's queued ticcmds should be processed and
// prepares the player.cmd structure for P_PlayerThink().  Also has the
// responsibility of ensuring the Unlag class has the appropriate latency
// for the player whose ticcmd we're processing.
//
void SV_ProcessPlayerCmd(player_t &player)
{
	static const int maxcmdmove = 12800;

	if (!validplayer(player) || !player.mo)
		return;

	#ifdef _TICCMD_QUEUE_DEBUG_
	DPrintf("Cmd queue size for %s: %d\n",
				player.userinfo.netname, player.cmdqueue.size());
	#endif	// _TICCMD_QUEUE_DEBUG_

	int num_cmds = SV_CalculateNumTiccmds(player);

	for (int i = 0; i < num_cmds && !player.cmdqueue.empty(); i++)
	{
		NetCommand *netcmd = &(player.cmdqueue.front());
		memset(&player.cmd, 0, sizeof(ticcmd_t));

		player.tic = netcmd->getTic();
		// Set the latency amount for Unlagging
		Unlag::getInstance().setRoundtripDelay(player.id, netcmd->getWorldIndex() & 0xFF);

		if ((netcmd->hasForwardMove() && abs(netcmd->getForwardMove()) > maxcmdmove) ||
			(netcmd->hasSideMove() && abs(netcmd->getSideMove()) > maxcmdmove))
		{
			SV_PlayerTriedToCheat(player);
			return;
		}

		netcmd->toPlayer(&player);

		if (!sv_freelook)
			player.mo->pitch = 0;

		// Apply this ticcmd using the game logic
		if (gamestate == GS_LEVEL)
		{
			P_PlayerThink(&player);
			player.mo->RunThink();
		}

		player.cmdqueue.pop();		// remove this tic from the queue after being processed
	}
}

//
// SV_GetPlayerCmd
//
// Extracts a player's ticcmd message from their network buffer and queues
// the ticcmd for later processing.  The client always sends its previous
// ticcmd followed by its current ticcmd just in case there is a dropped
// packet.

void SV_GetPlayerCmd(player_t &player)
{
	client_t *cl = &player.client;

	// The client-tic at the time this message was sent.  The server stores
	// this and sends it back the next time it tells the client
	int tic = MSG_ReadLong();

	// Read the last 10 ticcmds from the client and add any new ones
	// to the cmdqueue
	for (int i = 9; i >= 0; i--)
	{
		NetCommand netcmd;
		netcmd.read(&net_message);
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

void SV_UpdateConsolePlayer(player_t &player)
{
	AActor *mo = player.mo;
	client_t *cl = &player.client;

	// Send updates about a player's position as often as the player wishes
	if (!mo || !P_AtInterval(player.userinfo.update_rate))
		return;

	// GhostlyDeath -- Spectators are on their own really
	if (player.spectator)
	{
        SV_UpdateMovingSectors(player);
		return;
	}

	// client player will update his position if packets were missed
	MSG_WriteMarker (&cl->netbuf, svc_updatelocalplayer);

	// client-tic of the most recently processed ticcmd for this client
	MSG_WriteLong (&cl->netbuf, player.tic);

	MSG_WriteLong (&cl->netbuf, mo->x);
	MSG_WriteLong (&cl->netbuf, mo->y);
	MSG_WriteLong (&cl->netbuf, mo->z);

	MSG_WriteLong (&cl->netbuf, mo->momx);
	MSG_WriteLong (&cl->netbuf, mo->momy);
	MSG_WriteLong (&cl->netbuf, mo->momz);

    MSG_WriteByte (&cl->netbuf, mo->waterlevel);

    SV_UpdateMovingSectors(player);
}

//
//	SV_ChangeTeam
//																							[Toke - CTF]
//	Allows players to change teams properly in teamplay and CTF
//
void SV_ChangeTeam (player_t &player)  // [Toke - Teams]
{
	team_t team = (team_t)MSG_ReadByte();

	if ((team >= NUMTEAMS && team != TEAM_NONE) || team < 0)
		return;

	if(sv_gametype == GM_CTF && team >= 2)
		return;

	if(sv_gametype != GM_CTF && team >= sv_teamsinplay)
		return;

	team_t old_team = player.userinfo.team;
	player.userinfo.team = team;

	SV_BroadcastPrintf (PRINT_HIGH, "%s has joined the %s team.\n", player.userinfo.netname.c_str(), team_names[team]);

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
		if (player.mo && player.userinfo.team != old_team)
			P_DamageMobj (player.mo, 0, 0, 1000, 0);
}

//
// SV_Spectate
//
void SV_Spectate(player_t &player)
{
	// [AM] Code has three possible values; true, false and 5.  True specs the
	//      player, false unspecs him and 5 updates the server with the spec's
	//      new position.
	byte Code = MSG_ReadByte();

	if (!player.ingame())
		return;

	if (Code == 5)
	{
		// GhostlyDeath -- Prevent Cheaters
		if (!player.spectator || !player.mo)
		{
			for (int i = 0; i < 3; i++)
				MSG_ReadLong();
			return;
		}

		// GhostlyDeath -- Code 5! Anyway, this just updates the player for "antiwallhack" fun
		player.mo->x = MSG_ReadLong();
		player.mo->y = MSG_ReadLong();
		player.mo->z = MSG_ReadLong();
	}
	else
	{
		SV_SetPlayerSpec(player, Code);
	}
}

// Change a player into a spectator or vice-versa.  Pass 'true' for silent
// param to spec or unspec the player without a broadcasted message.
void P_SetSpectatorFlags(player_t &player);

void SV_SetPlayerSpec(player_t &player, bool setting, bool silent)
{
	// We don't care about spectators during intermission
	if (gamestate == GS_INTERMISSION)
		return;

	if (player.ingame() == false)
		return;

	if (!setting && player.spectator)
	{
		// We want to unspectate the player.
		if ((level.time > player.joinafterspectatortime + TICRATE * 3) ||
			level.time > player.joinafterspectatortime + TICRATE * 5)
		{

			// Check to see if there is an empty spot on the server
			int NumPlayers = 0;
			for (Players::iterator it = players.begin(); it != players.end(); ++it)
				if (it->ingame() && !it->spectator)
					NumPlayers++;

			// Too many players.
			if (!(NumPlayers < sv_maxplayers))
				return;

			// Check to make sure we're not exceeding sv_maxplayersperteam.
			if (sv_maxplayersperteam && (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF))
			{
				if (P_NumPlayersOnTeam(player.userinfo.team) >= sv_maxplayersperteam)
				{
					if (SV_GoodTeam() == TEAM_NONE)
						return;

					SV_ForceSetTeam(player, SV_GoodTeam());
					SV_CheckTeam(player);
				}
			}

			// [SL] 2011-09-01 - Clear any previous SV_MidPrint (sv_motd for example)
			SV_MidPrint("", &player, 0);

			player.spectator = false;
			for (Players::iterator it = players.begin(); it != players.end(); ++it)
			{
				MSG_WriteMarker(&it->client.reliablebuf, svc_spectate);
				MSG_WriteByte(&it->client.reliablebuf, player.id);
				MSG_WriteByte(&it->client.reliablebuf, false);
			}

			if (player.mo)
				P_KillMobj(NULL, player.mo, NULL, true);

			player.playerstate = PST_REBORN;

			if (!silent)
			{
				if (sv_gametype != GM_TEAMDM && sv_gametype != GM_CTF)
					SV_BroadcastPrintf(PRINT_HIGH, "%s joined the game.\n", player.userinfo.netname.c_str());
				else
					SV_BroadcastPrintf(PRINT_HIGH, "%s joined the game on the %s team.\n",
									   player.userinfo.netname.c_str(), team_names[player.userinfo.team]);
			}

			// GhostlyDeath -- Reset Frags, Deaths and Kills
			player.fragcount = 0;
			player.deathcount = 0;
			player.killcount = 0;
			SV_UpdateFrags(player);

			// [AM] Set player unready if we're in warmup mode.
			if (sv_warmup)
			{
				SV_SetReady(player, false, true);
				player.timeout_ready = 0;
			}
		}
	}
	else if (setting && !player.spectator)
	{
		// We want to spectate the player
		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			MSG_WriteMarker(&(it->client.reliablebuf), svc_spectate);
			MSG_WriteByte(&(it->client.reliablebuf), player.id);
			MSG_WriteByte(&(it->client.reliablebuf), true);
		}

		// call CTF_CheckFlags _before_ the player becomes a spectator.
		// Otherwise a flag carrier will drop his flag at (0,0), which
		// is often right next to one of the bases...
		if (sv_gametype == GM_CTF)
			CTF_CheckFlags(player);

		// [tm512 2014/04/18] Avoid setting spectator flags on a dead player
		// Instead we respawn the player, move him back, and immediately spectate him afterwards
		if (player.playerstate == PST_DEAD)
		{
//			player.deadspectator = true; // prevent teleport fog
			G_DoReborn (player);
//			player.deadspectator = false;
		}

		player.spectator = true;

		// [AM] Set player unready if we're in warmup mode.
		if (sv_warmup)
		{
			SV_SetReady(player, false, true);
			player.timeout_ready = 0;
		}

		player.playerstate = PST_LIVE;
		player.joinafterspectatortime = level.time;

		P_SetSpectatorFlags(player);

		if (!silent)
			SV_BroadcastPrintf(PRINT_HIGH, "%s became a spectator.\n", player.userinfo.netname.c_str());
	}
}

bool CMD_ForcespecCheck(const std::vector<std::string> &arguments,
						std::string &error, size_t &pid) {
	if (arguments.empty()) {
		error = "need a player id (try 'players').";
		return false;
	}

	std::istringstream buffer(arguments[0]);
	buffer >> pid;

	if (!buffer) {
		error = "player id needs to be a numeric value.";
		return false;
	}

	// Verify the player actually exists.
	player_t &player = idplayer(pid);
	if (!validplayer(player)) {
		std::ostringstream error_ss;
		error_ss << "could not find client " << pid << ".";
		error = error_ss.str();
		return false;
	}

	// Verify that the player is actually in a spectatable state.
	if (!player.ingame()) {
		std::ostringstream error_ss;
		error_ss << "cannot spectate client " << pid << ".";
		error = error_ss.str();
		return false;
	}

	// Verify that the player isn't already spectating.
	if (player.spectator) {
		std::ostringstream error_ss;
		error_ss << "client " << pid << " is already a spectator.";
		error = error_ss.str();
		return false;
	}

	return true;
}

BEGIN_COMMAND (forcespec) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::string error;

	size_t pid;

	if (!CMD_ForcespecCheck(arguments, error, pid)) {
		Printf(PRINT_HIGH, "forcespec: %s\n", error.c_str());
		return;
	}

	// Actually spec the given player
	player_t &player = idplayer(pid);
	SV_SetPlayerSpec(player, true);
} END_COMMAND (forcespec)

// Change the player's ready state and broadcast it to all connected players.
void SV_SetReady(player_t &player, bool setting, bool silent)
{
	if (!validplayer(player) || !player.ingame())
		return;

	// Change the player's ready state only if the new state is different from
	// the current state.
	bool changed = true;
	if (player.ready && !setting) {
		player.ready = false;
		if (!silent) {
			if (player.spectator)
				SV_PlayerPrintf(PRINT_HIGH, player.id, "You are no longer willing to play.\n");
			else
				SV_PlayerPrintf(PRINT_HIGH, player.id, "You are no longer ready to play.\n");
		}
	} else if (!player.ready && setting) {
		player.ready = true;
		if (!silent) {
			if (player.spectator)
				SV_PlayerPrintf(PRINT_HIGH, player.id, "You are now willing to play.\n");
			else
				SV_PlayerPrintf(PRINT_HIGH, player.id, "You are now ready to play.\n");
		}
	} else {
		changed = false;
	}

	if (changed) {
		// Broadcast the new ready state to all connected players.
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			MSG_WriteMarker(&(it->client.reliablebuf), svc_readystate);
			MSG_WriteByte(&(it->client.reliablebuf), player.id);
			MSG_WriteBool(&(it->client.reliablebuf), setting);
		}
	}

	warmup.readytoggle();
}

void SV_Ready(player_t &player)
{

	if (warmup.get_status() != ::Warmup::WARMUP)
		return;

	// If the player is not ingame, he shouldn't be sending us ready packets.
	if (!player.ingame()) {
		return;
	}

	if (player.timeout_ready > level.time) {
		// We must be on a new map.  Reset the timeout.
		player.timeout_ready = 0;
	}

	// Check to see if warmup will allow us to toggle our ready state.
	if (!warmup.checkreadytoggle())
	{
		SV_PlayerPrintf(PRINT_HIGH, player.id, "You can't ready in the middle of a match!\n");
		return;
	}

	// Check to see if the player's timeout has expired.
	if (player.timeout_ready > 0) {
		int timeout = level.time - player.timeout_ready;

		// Players can only toggle their ready state every 3 seconds.
		int timeout_check = 3 * TICRATE;
		int timeout_waitsec = 3 - (timeout / TICRATE);

		if (timeout < timeout_check) {
			SV_PlayerPrintf(PRINT_HIGH, player.id, "Please wait another %d second%s to change your ready state.\n",
			                timeout_waitsec, timeout_waitsec != 1 ? "s" : "");
			return;
		}
	}

	// Set the timeout.
	player.timeout_ready = level.time;

	// Toggle ready state
	SV_SetReady(player, !player.ready);
}

//
// SV_RConLogout
//
//   Removes rcon privileges
//
void SV_RConLogout (player_t &player)
{
	client_t *cl = &player.client;

	// read and ignore the password field since rcon_logout doesn't use a password
	MSG_ReadString();

	if (cl->allow_rcon)
	{
		Printf(PRINT_HIGH, "rcon logout from %s - %s", player.userinfo.netname.c_str(), NET_AdrToString(cl->address));
		cl->allow_rcon = false;
	}
}


//
// SV_RConPassword
// denis
//
void SV_RConPassword (player_t &player)
{
	client_t *cl = &player.client;

	std::string challenge = MSG_ReadString();
	std::string password = rcon_password.cstring();

	// Don't display login messages again if the client is already logged in
	if (cl->allow_rcon)
		return;

	if (!password.empty() && MD5SUM(password + cl->digest) == challenge)
	{
		cl->allow_rcon = true;
		Printf(PRINT_HIGH, "rcon login from %s - %s", player.userinfo.netname.c_str(), NET_AdrToString(cl->address));
	}
	else
	{
		Printf(PRINT_HIGH, "rcon login failure from %s - %s", player.userinfo.netname.c_str(), NET_AdrToString(cl->address));
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString (&cl->reliablebuf, "Bad password\n");
	}
}

//
// SV_Suicide
//
void SV_Suicide(player_t &player)
{
	if (!player.mo)
		return;

	// merry suicide!
	P_DamageMobj (player.mo, NULL, NULL, 10000, MOD_SUICIDE);
	//player.mo->player = NULL;
	//player.mo = NULL;
}

//
// SV_Cheat
//
void SV_Cheat(player_t &player)
{
	byte cheats = MSG_ReadByte();

	if(!sv_allowcheats)
		return;

	player.cheats = cheats;
}

BOOL P_GiveWeapon(player_s*, weapontype_t, BOOL);
BOOL P_GivePower(player_s*, int);

void SV_CheatPulse(player_t &player)
{
    byte cheats = MSG_ReadByte();
    int i;

    if (!sv_allowcheats)
    {
        if (cheats == 3)
            MSG_ReadByte();

        return;
    }

    if (cheats == 1)
    {
        player.armorpoints = deh.FAArmor;
        player.armortype = deh.FAAC;

        weapontype_t pendweap = player.pendingweapon;

        for (i = 0; i<NUMWEAPONS; i++)
            P_GiveWeapon (&player, (weapontype_t)i, false);

        player.pendingweapon = pendweap;

        for (i=0; i<NUMAMMO; i++)
            player.ammo[i] = player.maxammo[i];

        return;
    }

    if (cheats == 2)
    {
        player.armorpoints = deh.KFAArmor;
        player.armortype = deh.KFAAC;

        weapontype_t pendweap = player.pendingweapon;

        for (i = 0; i<NUMWEAPONS; i++)
            P_GiveWeapon (&player, (weapontype_t)i, false);

        player.pendingweapon = pendweap;

        for (i=0; i<NUMAMMO; i++)
            player.ammo[i] = player.maxammo[i];

        for (i=0; i<NUMCARDS; i++)
            player.cards[i] = true;

        return;
    }

    if (cheats == 3)
    {
        byte power = MSG_ReadByte();

        if (!player.powers[power])
            P_GivePower(&player, power);
        else if (power != pw_strength)
            player.powers[power] = 1;
        else
            player.powers[power] = 0;

        return;
    }

    if (cheats == 4)
    {
        player.weaponowned[wp_chainsaw] = true;

        return;
    }
}

void SV_WantWad(player_t &player)
{
	client_t *cl = &player.client;

	if(!sv_waddownload)
	{
		// read and ignore the rest of the wad request
		MSG_ReadString();
		MSG_ReadString();
		MSG_ReadLong();

		MSG_WriteMarker(&cl->reliablebuf, svc_print);
		MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString(&cl->reliablebuf, "Server: Downloading is disabled\n");

		SV_DropClient(player);
		return;
	}

	if(player.ingame())
		SV_Suicide(player);

	std::string request = MSG_ReadString();
	std::string md5 = MSG_ReadString();
	size_t next_offset = MSG_ReadLong();

	std::transform(md5.begin(), md5.end(), md5.begin(), toupper);

	size_t i;
	std::string filename;
	for (i = 0; i < wadfiles.size(); i++)
	{
		filename = D_CleanseFileName(wadfiles[i]);
		if (filename == request && (md5.empty() || wadhashes[i] == md5))
			break;
	}

	if (i == wadfiles.size())
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString (&cl->reliablebuf, "Server: Bad wad request\n");
		SV_DropClient(player);
		return;
	}

	// denis - do not download commercial wads
	if (W_IsIWAD(wadfiles[i]))
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		char message[256];	
		sprintf(message, "Server: %s is a commercial wad and will not be downloaded\n",
				D_CleanseFileName(wadfiles[i]).c_str());
		MSG_WriteString(&cl->reliablebuf, message);

		SV_DropClient(player);
		return;
	}

	if (player.playerstate != PST_DOWNLOAD || cl->download.name != wadfiles[i])
		Printf(PRINT_HIGH, "> client %d is downloading %s\n", player.id, filename.c_str());

	cl->download.name = wadfiles[i];
	cl->download.next_offset = next_offset;
	player.playerstate = PST_DOWNLOAD;
}

//
// SV_ParseCommands
//

void SV_SendPlayerInfo(player_t &player);

void SV_ParseCommands(player_t &player)
{
	 while(validplayer(player))
	 {
		clc_t cmd = (clc_t)MSG_ReadByte();

		if(cmd == (clc_t)-1)
			break;

		switch(cmd)
		{
		case clc_disconnect:
			SV_DisconnectClient(player);
			return;

		case clc_userinfo:
			if (!SV_SetupUserInfo(player))
				return;
			SV_BroadcastUserInfo(player);
			break;

		case clc_getplayerinfo:
			SV_SendPlayerInfo (player);
			break;

		case clc_say:
			if (!SV_Say(player))
				return;
			break;

		case clc_privmsg:
			if (!SV_PrivMsg(player))
				return;
			break;

		case clc_move:
			SV_GetPlayerCmd(player);
			break;

		case clc_pingreply:  // [SL] 2011-05-11 - Changed to clc_pingreply
			SV_CalcPing(player);
			break;

		case clc_rate:
			{
				// denis - prevent problems by locking rate within a range
				SV_SetClientRate(player.client, MSG_ReadLong());
			}
			break;

		case clc_ack:
			SV_AcknowledgePacket(player);
			break;

		case clc_rcon:
			{
				std::string str(MSG_ReadString());
				StripColorCodes(str);

				if (player.client.allow_rcon)
				{
					Printf(PRINT_HIGH, "rcon command from %s - %s -> %s",
							player.userinfo.netname.c_str(), NET_AdrToString(net_from), str.c_str());
					AddCommandString(str);
				}
			}
			break;

		case clc_rcon_password:
			{
				bool login = MSG_ReadByte();

				if (login)
					SV_RConPassword(player);
				else
					SV_RConLogout(player);

				break;
			}

		case clc_changeteam:
			SV_ChangeTeam(player);
			break;

		case clc_spectate:
            {
                SV_Spectate (player);
            }
			break;

		case clc_ready:
			SV_Ready(player);
			break;

		case clc_kill:
			if(player.mo &&
               level.time > player.death_time + TICRATE*10 &&
               (sv_allowcheats || sv_gametype == GM_COOP || warmup.get_status() == warmup.WARMUP))
            {
				SV_Suicide (player);
            }
			break;

		case clc_launcher_challenge:
			MSG_ReadByte();
			MSG_ReadByte();
			MSG_ReadByte();
			SV_SendServerInfo();
			break;

		case clc_challenge:
			{
				size_t len = MSG_BytesLeft();
				MSG_ReadChunk(len);
			}
			break;

		case clc_wantwad:
			SV_WantWad(player);
			break;

		case clc_cheat:
			SV_Cheat(player);
			break;

        case clc_cheatpulse:
            SV_CheatPulse(player);
            break;

		case clc_abort:
			Printf(PRINT_HIGH, "Client abort.\n");
			SV_DropClient(player);
			return;

		case clc_spy:
			SV_SpyPlayer(player);
			break;

		// [AM] Vote
		case clc_callvote:
			SV_Callvote(player);
			break;
		case clc_vote:
			SV_Vote(player);
			break;

		// [AM] Maplist
		case clc_maplist:
			SV_Maplist(player);
			break;
		case clc_maplist_update:
			SV_MaplistUpdate(player);
			break;

		default:
			Printf(PRINT_HIGH, "SV_ParseCommands: Unknown client message %d.\n", (int)cmd);
			SV_DropClient(player);
			return;
		}

		if (net_message.overflowed)
		{
			Printf (PRINT_HIGH, "SV_ReadClientMessage: badread %d(%s)\n",
					    (int)cmd,
					    clc_info[cmd].getName());
			SV_DropClient(player);
			return;
		}
	 }
}

//
// SV_WinCheck - Checks to see if a game has been won
//
void SV_WinCheck (void)
{
	if (shotclock)
	{
		shotclock--;

		if (!shotclock)
			G_ExitLevel (0, 1);
	}
}

EXTERN_CVAR (sv_waddownloadcap)

//
// SV_WadDownloads
//
void SV_WadDownloads (void)
{
	// nobody around?
	if(players.empty())
		return;

	// wad downloading
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->playerstate != PST_DOWNLOAD)
			continue;

		client_t *cl = &(it->client);

		if(!cl->download.name.length())
			continue;

		static char buff[1024];
		// Smaller buffer for slower clients
		int chunk_size = (sizeof(buff) > (unsigned)cl->rate*1000/TICRATE) ? cl->rate*1000/TICRATE : sizeof(buff);

		// maximum rate client can download at (in bytes per second)
		int download_rate = (sv_waddownloadcap > cl->rate) ? cl->rate*1000 : sv_waddownloadcap*1000;

		do
		{
			// read next bit of wad
			unsigned int read;
			unsigned int filelen = 0;
			read = W_ReadChunk(cl->download.name.c_str(), cl->download.next_offset, chunk_size, buff, filelen);

			if (!read)
				break;

			// [SL] 2011-08-09 - Always send the data in netbuf and reliablebuf prior
			// to writing a wadchunk to netbuf to keep packet sizes below the MTU.
			// This prevents packets from getting dropped due to size on some networks.
			if (cl->netbuf.size() + cl->reliablebuf.size())
				SV_SendPacket(*it);

			if(!cl->download.next_offset)
			{
				MSG_WriteMarker (&cl->netbuf, svc_wadinfo);
				MSG_WriteLong (&cl->netbuf, filelen);
			}

			MSG_WriteMarker (&cl->netbuf, svc_wadchunk);
			MSG_WriteLong (&cl->netbuf, cl->download.next_offset);
			MSG_WriteShort (&cl->netbuf, read);
			MSG_WriteChunk (&cl->netbuf, buff, read);

			// Make double-sure the wadchunk is sent in its own packet
			if (cl->netbuf.size() + cl->reliablebuf.size())
				SV_SendPacket(*it);

			cl->download.next_offset += read;
		} while (
			(double)(cl->reliable_bps + cl->unreliable_bps) * TICRATE
				/ (double)(gametic % TICRATE)	// bps already used
				+ (double)chunk_size / TICRATE 	// bps this chunk will use
			< (double)download_rate);
	}
}

//
//	SV_WinningTeam					[Toke - teams]
//
//	Determines the winning team, if there is one
//
team_t SV_WinningTeam (void)
{
	team_t team = (team_t)0;
	bool isdraw = false;

	for(size_t i = 1; i < NUMTEAMS; i++)
	{
		if(TEAMpoints[i] > TEAMpoints[(int)team]) {
			team = (team_t)i;
			isdraw = false;
		} else if(TEAMpoints[i] == TEAMpoints[(int)team]) {
			isdraw = true;
		}
	}

	if (isdraw)
		team = TEAM_NONE;

	return team;
}

//
//	SV_TimelimitCheck
//
void SV_TimelimitCheck()
{
	if(!sv_timelimit)
		return;

	if (warmup.get_status() == warmup.INGAME && sv_warmup_overtime_enable)
		level.timeleft = (int)(sv_timelimit * TICRATE * 60)+(warmup.get_overtime() *sv_warmup_overtime* TICRATE *60);
	else
		level.timeleft = (int)(sv_timelimit * TICRATE * 60);

	// Don't substract the proper amount of time unless we're actually ingame.
	if (warmup.checktimeleftadvance())
		level.timeleft -= level.time;	// in tics

	// [SL] 2011-10-25 - Send the clients the remaining time (measured in seconds)
	if (P_AtInterval(TICRATE))
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			MSG_WriteMarker(&(it->client.netbuf), svc_timeleft);
			MSG_WriteShort(&(it->client.netbuf), level.timeleft / TICRATE);
		}
	}

	if (level.timeleft > 0 || shotclock || gamestate == GS_INTERMISSION)
		return;

	// LEVEL TIMER
	if (!players.empty()) {
		if (sv_gametype == GM_DM) {
			player_t *winplayer = &*(players.begin());
			bool drawgame = false;

			if (players.size() > 1) {
				for (Players::iterator it = players.begin();it != players.end();++it)
				{
					if (it->fragcount > winplayer->fragcount)
					{
						drawgame = false;
						winplayer = &*it;
					}
					else if (it->id != winplayer->id && it->fragcount == winplayer->fragcount)
					{
						drawgame = true;
					}
				}
			}

			if (drawgame)
			{
				if (sv_warmup_overtime_enable && warmup.get_status() == warmup.INGAME)
				{
					warmup.add_overtime();
					SV_BroadcastPrintf(PRINT_HIGH, "Overtime \#%d! Adding %d minute%s.\n", warmup.get_overtime(), sv_warmup_overtime.asInt(), (sv_warmup_overtime.asInt()>1 ? "s" : ""));
					return;
				}
				else
					SV_BroadcastPrintf (PRINT_HIGH, "Time limit hit. Game is a draw!\n");
			}
			else
				SV_BroadcastPrintf (PRINT_HIGH, "Time limit hit. Game won by %s!\n", winplayer->userinfo.netname.c_str());
		} else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
			team_t winteam = SV_WinningTeam ();

			if (winteam == TEAM_NONE)
			{
				if (sv_warmup_overtime_enable && warmup.get_status() == warmup.INGAME)
				{
					warmup.add_overtime();
					SV_BroadcastPrintf(PRINT_HIGH, "Overtime \#%d! Adding %d minute%s.\n", warmup.get_overtime(), sv_warmup_overtime.asInt(), (sv_warmup_overtime.asInt()>1?"s":""));

					if (sv_gametype == GM_CTF)
						SV_BroadcastPrintf(PRINT_HIGH, "Respawning penalty time: %d seconds.\n", warmup.get_ctf_overtime_penalty());

					return;
				}
				else
					SV_BroadcastPrintf(PRINT_HIGH, "Time limit hit. Game is a draw!\n");
			}
			else
				SV_BroadcastPrintf (PRINT_HIGH, "Time limit hit. %s team wins!\n", team_names[winteam]);
		}
	}

	shotclock = TICRATE*2;
}

void SV_IntermissionTimeCheck()
{
	level.inttimeleft = mapchange/TICRATE;

	// [SL] 2011-10-25 - Send the clients the remaining time (measured in seconds)
	// [ML] 2012-2-1 - Copy it for intermission fun
	if (P_AtInterval(1 * TICRATE)) // every second
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			MSG_WriteMarker(&(it->client.netbuf), svc_inttimeleft);
			MSG_WriteShort(&(it->client.netbuf), level.inttimeleft);
		}
	}
}

//
// SV_GameTics
//
//	Runs once gametic (35 per second gametic)
//
void SV_GameTics (void)
{
	if (sv_gametype == GM_CTF)
		CTF_RunTics();

	switch (gamestate)
	{
		case GS_LEVEL:
			SV_RemoveCorpses();
			warmup.tic();
			SV_WinCheck();
			SV_TimelimitCheck();
			Vote_Runtic();
		break;
		case GS_INTERMISSION:
			SV_IntermissionTimeCheck();
		break;

		default:
		break;
	}

	for (Players::iterator it = players.begin();it != players.end();++it)
		SV_ProcessPlayerCmd(*it);

	SV_WadDownloads();
}

void SV_TouchSpecial(AActor *special, player_t *player)
{
    client_t *cl = &player->client;

    if (cl == NULL || special == NULL)
        return;

    MSG_WriteMarker(&cl->reliablebuf, svc_touchspecial);
    MSG_WriteShort(&cl->reliablebuf, special->netid);
}

void SV_PlayerTimes (void)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame())
			(it->GameTime) += 1;
	}
}


//
// SV_Frozen
//
// Returns true if the game state should be frozen (not advance).
//
bool SV_Frozen()
{
	return sv_emptyfreeze && players.empty() && gamestate == GS_LEVEL;
}


//
// SV_StepTics
//
void SV_StepTics(QWORD count)
{
	DObject::BeginFrame();

	// run the newtime tics
	while (count--)
	{
		SV_GameTics();

		G_Ticker();

		SV_WriteCommands();
		SV_SendPackets();
		SV_ClearClientsBPS();
		SV_CheckTimeouts();

		// Since clients are only sent sector updates every 3rd tic, don't destroy
		// the finished moving sectors until we've sent the clients the update
		if (P_AtInterval(3))
			SV_DestroyFinishedMovingSectors();

		// increment player_t::GameTime for all players once a second
		static int TicCount = 0;
		// Only do this once a second.
		if (TicCount++ >= 35)
		{
			SV_PlayerTimes();
			TicCount = 0;
		}

		gametic++;
	}

	DObject::EndFrame();
}

//
// SV_DisplayTics
//
// Nothing to display...
//
void SV_DisplayTics()
{
}

//
// SV_RunTics
//
// Checks for incoming packets, processes console usage, and calls SV_StepTics.
//
void SV_RunTics()
{
	SV_GetPackets();

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

	SV_BanlistTics();
	SV_UpdateMaster();

	C_Ticker();

	// only run game-related tickers if the server isn't frozen
	// (sv_emptyfreeze enabled and no clients)
	if (!step_mode && !SV_Frozen())
		SV_StepTics(1);

	// Remove any recently disconnected clients
	for (Players::iterator it = players.begin(); it != players.end();)
	{
		if (it->playerstate == PST_DISCONNECT)
			it = SV_RemoveDisconnectedPlayer(it);
		else
			++it;
	}

	// [SL] 2011-05-18 - Handle sv_emptyreset
	static size_t last_player_count = players.size();
	if (gamestate == GS_LEVEL && sv_emptyreset && players.empty() &&
			last_player_count > 0)
	{
		// The last player just disconnected so reset the level.
		// [SL] Ordinarily we should call G_DeferedInitNew but this is called
		// at the end of a gametic and the level reset should take place now
		// rather than at the start of the next gametic.

		// [SL] create a copy of level.mapname because G_InitNew uses strncpy
		// to copy the mapname parameter to level.mapname, which is undefined
		// behavior.
		char mapname[9];
		strncpy(mapname, level.mapname, 8);
		mapname[8] = 0;

		G_InitNew(mapname);
	}
	last_player_count = players.size();
}


BEGIN_COMMAND(step)
{
        QWORD newtics = argc > 1 ? atoi(argv[1]) : 1;

	extern unsigned char prndindex;

	SV_StepTics(newtics);

	// debugging output
	if (players.size() && players.begin() != players.end())
		Printf(PRINT_HIGH, "level.time %d, prndindex %d, %d %d %d\n", level.time, prndindex, players.begin()->mo->x, players.begin()->mo->y, players.begin()->mo->z);
	else
		Printf(PRINT_HIGH, "level.time %d, prndindex %d\n", level.time, prndindex);
}
END_COMMAND(step)


// For Debugging
BEGIN_COMMAND (playerinfo)
{
	player_t *player = &consoleplayer();

	if (argc > 1)
	{
		player_t &p = idplayer(atoi(argv[1]));

		if (!validplayer(p))
		{
			Printf (PRINT_HIGH, "Bad player number\n");
			return;
		}
		else
			player = &p;
	}

	if (!validplayer(*player))
	{
		Printf(PRINT_HIGH, "Not a valid player\n");
		return;
	}

	char ip[16];
	sprintf(ip, "%d.%d.%d.%d",
			player->client.address.ip[0], player->client.address.ip[1],
			player->client.address.ip[2], player->client.address.ip[3]);

	char color[8];
	sprintf(color, "#%02X%02X%02X",
			player->userinfo.color[1], player->userinfo.color[2], player->userinfo.color[3]);

	char team[5] = { 0 };
	if (player->userinfo.team == TEAM_BLUE)
		sprintf(team, "BLUE");
	else if (player->userinfo.team == TEAM_RED)
		sprintf(team, "RED");

	Printf(PRINT_HIGH, "---------------[player info]----------- \n");
	Printf(PRINT_HIGH, " IP Address       - %s \n",		ip);
	Printf(PRINT_HIGH, " userinfo.netname - %s \n",		player->userinfo.netname.c_str());
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
		Printf(PRINT_HIGH, " userinfo.team    - %s \n",		team);
	Printf(PRINT_HIGH, " userinfo.aimdist - %d \n",		player->userinfo.aimdist >> FRACBITS);
	Printf(PRINT_HIGH, " userinfo.unlag   - %d \n",		player->userinfo.unlag);
	Printf(PRINT_HIGH, " userinfo.color   - %s \n",		color);
	Printf(PRINT_HIGH, " userinfo.gender  - %d \n",		player->userinfo.gender);
	Printf(PRINT_HIGH, " time             - %d \n",		player->GameTime);
	Printf(PRINT_HIGH, " spectator        - %d \n",		player->spectator);
	Printf(PRINT_HIGH, " downloader       - %d \n",		player->playerstate == PST_DOWNLOAD);
	Printf(PRINT_HIGH, "--------------------------------------- \n");
}
END_COMMAND (playerinfo)

BEGIN_COMMAND (playerlist)
{
	bool anybody = false;

	for (Players::reverse_iterator it = players.rbegin();it != players.rend();++it)
	{
		Printf(PRINT_HIGH, "(%02d): %s - %s - frags:%d ping:%d\n",
		       it->id, it->userinfo.netname.c_str(), NET_AdrToString(it->client.address), it->fragcount, it->ping);
		anybody = true;
	}

	if (!anybody)
	{
		Printf(PRINT_HIGH, "There are no players on the server\n");
		return;
	}
}
END_COMMAND (playerlist)

BEGIN_COMMAND (players)
{
	AddCommandString("playerlist");
}
END_COMMAND (players)

void OnChangedSwitchTexture (line_t *line, int useAgain)
{
	int l;

	for (l=0; l<numlines; l++)
	{
		if (line == &lines[l])
			break;
	}

	line->wastoggled = 1;

	unsigned state = 0, time = 0;
	P_GetButtonInfo(line, state, time);

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteMarker (&cl->reliablebuf, svc_switch);
		MSG_WriteLong (&cl->reliablebuf, l);
		MSG_WriteByte (&cl->reliablebuf, line->wastoggled);
		MSG_WriteByte (&cl->reliablebuf, state);
		MSG_WriteLong (&cl->reliablebuf, time);
	}
}

void OnActivatedLine (line_t *line, AActor *mo, int side, int activationType)
{
	int l = 0;
	for (l = 0; l < numlines; l++)
		if(&lines[l] == line)
			break;

	line->wastoggled = 1;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		client_t *cl = &(it->client);

		MSG_WriteMarker (&cl->reliablebuf, svc_activateline);
		MSG_WriteLong (&cl->reliablebuf, l);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);
		MSG_WriteByte (&cl->reliablebuf, side);
		MSG_WriteByte (&cl->reliablebuf, activationType);
	}
}

// [RH]
// ClientObituary: Show a message when a player dies
//
void ClientObituary(AActor* self, AActor* inflictor, AActor* attacker)
{
	char gendermessage[1024];

	if (!self || !self->player)
		return;

	// Don't print obituaries after the end of a round
	if (shotclock || gamestate != GS_LEVEL)
		return;

	int gender = self->player->userinfo.gender;

	// Treat voodoo dolls as unknown deaths
	if (inflictor && inflictor->player == self->player)
		MeansOfDeath = MOD_UNKNOWN;

	if (sv_gametype == GM_COOP)
		MeansOfDeath |= MOD_FRIENDLY_FIRE;

	if ((sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) &&
		attacker && attacker->player &&
		self->player->userinfo.team == attacker->player->userinfo.team)
		MeansOfDeath |= MOD_FRIENDLY_FIRE;

	bool friendly = MeansOfDeath & MOD_FRIENDLY_FIRE;
	int mod = MeansOfDeath & ~MOD_FRIENDLY_FIRE;
	const char* message = NULL;
	int messagenum = 0;

	switch (mod)
	{
		case MOD_SUICIDE:		messagenum = OB_SUICIDE;	break;
		case MOD_FALLING:		messagenum = OB_FALLING;	break;
		case MOD_CRUSH:			messagenum = OB_CRUSH;		break;
		case MOD_EXIT:			messagenum = OB_EXIT;		break;
		case MOD_WATER:			messagenum = OB_WATER;		break;
		case MOD_SLIME:			messagenum = OB_SLIME;		break;
		case MOD_LAVA:			messagenum = OB_LAVA;		break;
		case MOD_BARREL:		messagenum = OB_BARREL;		break;
		case MOD_SPLASH:		messagenum = OB_SPLASH;		break;
	}

	if (messagenum)
		message = GStrings(messagenum);

	if (attacker && message == NULL)
	{
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_R_SPLASH:	messagenum = OB_R_SPLASH;		break;
			case MOD_ROCKET:	messagenum = OB_ROCKET;			break;
			default:			messagenum = OB_KILLEDSELF;		break;
			}
			message = GStrings(messagenum);
		}
		else if (!attacker->player)
		{
			if (mod == MOD_HIT)
			{
				switch (attacker->type)
				{
					case MT_UNDEAD:
						messagenum = OB_UNDEADHIT;
						break;
					case MT_TROOP:
						messagenum = OB_IMPHIT;
						break;
					case MT_HEAD:
						messagenum = OB_CACOHIT;
						break;
					case MT_SERGEANT:
						messagenum = OB_DEMONHIT;
						break;
					case MT_SHADOWS:
						messagenum = OB_SPECTREHIT;
						break;
					case MT_BRUISER:
						messagenum = OB_BARONHIT;
						break;
					case MT_KNIGHT:
						messagenum = OB_KNIGHTHIT;
						break;
					default:
						break;
				}
			}
			else
			{
				switch (attacker->type)
				{
					case MT_POSSESSED:	messagenum = OB_ZOMBIE;		break;
					case MT_SHOTGUY:	messagenum = OB_SHOTGUY;	break;
					case MT_VILE:		messagenum = OB_VILE;		break;
					case MT_UNDEAD:		messagenum = OB_UNDEAD;		break;
					case MT_FATSO:		messagenum = OB_FATSO;		break;
					case MT_CHAINGUY:	messagenum = OB_CHAINGUY;	break;
					case MT_SKULL:		messagenum = OB_SKULL;		break;
					case MT_TROOP:		messagenum = OB_IMP;		break;
					case MT_HEAD:		messagenum = OB_CACO;		break;
					case MT_BRUISER:	messagenum = OB_BARON;		break;
					case MT_KNIGHT:		messagenum = OB_KNIGHT;		break;
					case MT_SPIDER:		messagenum = OB_SPIDER;		break;
					case MT_BABY:		messagenum = OB_BABY;		break;
					case MT_CYBORG:		messagenum = OB_CYBORG;		break;
					case MT_WOLFSS:		messagenum = OB_WOLFSS;		break;
					default:break;
				}
			}

			if (messagenum)
				message = GStrings(messagenum);
		}
	}

	if (message)
	{
		SexMessage(message, gendermessage, gender,
				self->player->userinfo.netname.c_str(), self->player->userinfo.netname.c_str());
		SV_BroadcastPrintf(PRINT_MEDIUM, "%s\n", gendermessage);
		return;
	}

	if (attacker && attacker->player)
	{
		if (friendly)
		{
			gender = attacker->player->userinfo.gender;
			messagenum = OB_FRIENDLY1 + (P_Random() & 3);
		}
		else
		{
			switch (mod)
			{
				case MOD_FIST:			messagenum = OB_MPFIST;			break;
				case MOD_CHAINSAW:		messagenum = OB_MPCHAINSAW;		break;
				case MOD_PISTOL:		messagenum = OB_MPPISTOL;		break;
				case MOD_SHOTGUN:		messagenum = OB_MPSHOTGUN;		break;
				case MOD_SSHOTGUN:		messagenum = OB_MPSSHOTGUN;		break;
				case MOD_CHAINGUN:		messagenum = OB_MPCHAINGUN;		break;
				case MOD_ROCKET:		messagenum = OB_MPROCKET;		break;
				case MOD_R_SPLASH:		messagenum = OB_MPR_SPLASH;		break;
				case MOD_PLASMARIFLE:	messagenum = OB_MPPLASMARIFLE;	break;
				case MOD_BFG_BOOM:		messagenum = OB_MPBFG_BOOM;		break;
				case MOD_BFG_SPLASH:	messagenum = OB_MPBFG_SPLASH;	break;
				case MOD_TELEFRAG:		messagenum = OB_MPTELEFRAG;		break;
				case MOD_RAILGUN:		messagenum = OB_RAILGUN;		break;
			}
		}
		if (messagenum)
			message = GStrings(messagenum);
	}

	if (message && attacker && attacker->player)
	{
		SexMessage(message, gendermessage, gender,
				self->player->userinfo.netname.c_str(), attacker->player->userinfo.netname.c_str());
		SV_BroadcastPrintf(PRINT_MEDIUM, "%s\n", gendermessage);
		return;
	}

	SexMessage(GStrings(OB_DEFAULT), gendermessage, gender,
			self->player->userinfo.netname.c_str(), self->player->userinfo.netname.c_str());
	SV_BroadcastPrintf(PRINT_MEDIUM, "%s\n", gendermessage);
}

void SV_SendDamagePlayer(player_t *player, int damage)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteMarker(&cl->reliablebuf, svc_damageplayer);
		MSG_WriteByte(&cl->reliablebuf, player->id);
		MSG_WriteByte(&cl->reliablebuf, player->armorpoints);
		MSG_WriteShort(&cl->reliablebuf, damage);
	}
}

void SV_SendDamageMobj(AActor *target, int pain)
{
	if (!target)
		return;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteMarker(&cl->reliablebuf, svc_damagemobj);
		MSG_WriteShort(&cl->reliablebuf, target->netid);
		MSG_WriteShort(&cl->reliablebuf, target->health);
		MSG_WriteByte(&cl->reliablebuf, pain);

		MSG_WriteMarker (&cl->netbuf, svc_movemobj);
		MSG_WriteShort (&cl->netbuf, target->netid);
		MSG_WriteByte (&cl->netbuf, target->rndindex);
		MSG_WriteLong (&cl->netbuf, target->x);
		MSG_WriteLong (&cl->netbuf, target->y);
		MSG_WriteLong (&cl->netbuf, target->z);

		MSG_WriteMarker (&cl->netbuf, svc_mobjspeedangle);
		MSG_WriteShort(&cl->netbuf, target->netid);
		MSG_WriteLong (&cl->netbuf, target->angle);
		MSG_WriteLong (&cl->netbuf, target->momx);
		MSG_WriteLong (&cl->netbuf, target->momy);
		MSG_WriteLong (&cl->netbuf, target->momz);
	}
}

void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor,
				     bool joinkill)
{
	if (!target)
		return;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		if (!SV_IsPlayerAllowedToSee(*it, target))
			continue;

		// send death location first
		MSG_WriteMarker(&cl->reliablebuf, svc_movemobj);
		MSG_WriteShort(&cl->reliablebuf, target->netid);
		MSG_WriteByte(&cl->reliablebuf, target->rndindex);

		// [SL] 2012-12-26 - Get real position since this actor is at
		// a reconciled position with sv_unlag 1
		fixed_t xoffs = 0, yoffs = 0, zoffs = 0;
		if (target->player)
		{
			Unlag::getInstance().getReconciliationOffset(
					target->player->id, xoffs, yoffs, zoffs);
		}

		MSG_WriteLong(&cl->reliablebuf, target->x + xoffs);
		MSG_WriteLong(&cl->reliablebuf, target->y + yoffs);
		MSG_WriteLong(&cl->reliablebuf, target->z + zoffs);

		MSG_WriteMarker (&cl->reliablebuf, svc_mobjspeedangle);
		MSG_WriteShort(&cl->reliablebuf, target->netid);
		MSG_WriteLong (&cl->reliablebuf, target->angle);
		MSG_WriteLong (&cl->reliablebuf, target->momx);
		MSG_WriteLong (&cl->reliablebuf, target->momy);
		MSG_WriteLong (&cl->reliablebuf, target->momz);

		MSG_WriteMarker(&cl->reliablebuf, svc_killmobj);
		if (source)
			MSG_WriteShort(&cl->reliablebuf, source->netid);
		else
			MSG_WriteShort(&cl->reliablebuf, 0);

		MSG_WriteShort (&cl->reliablebuf, target->netid);
		MSG_WriteShort (&cl->reliablebuf, inflictor ? inflictor->netid : 0);
		MSG_WriteShort (&cl->reliablebuf, target->health);
		MSG_WriteLong (&cl->reliablebuf, MeansOfDeath);
		MSG_WriteByte (&cl->reliablebuf, joinkill);
	}
}

// Tells clients to remove an actor from the world as it doesn't exist anymore
void SV_SendDestroyActor(AActor *mo)
{
	if (mo->netid && mo->type != MT_PUFF)
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (mo->players_aware.get(it->id))
			{
				client_t *cl = &(it->client);

            	// denis - todo - need a queue for destroyed (lost awareness)
            	// objects, as a flood of destroyed things could easily overflow a
            	// buffer
				MSG_WriteMarker(&cl->reliablebuf, svc_removemobj);
				MSG_WriteShort(&cl->reliablebuf, mo->netid);
			}
		}
	}

	// AActor no longer active. NetID released.
	if (mo->netid)
		ServerNetID.ReleaseNetID( mo->netid );
}

// Missile exploded so tell clients about it
void SV_ExplodeMissile(AActor *mo)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		if (!SV_IsPlayerAllowedToSee(*it, mo))
			continue;

		MSG_WriteMarker (&cl->reliablebuf, svc_movemobj);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);
		MSG_WriteByte (&cl->reliablebuf, mo->rndindex);
		MSG_WriteLong (&cl->reliablebuf, mo->x);
		MSG_WriteLong (&cl->reliablebuf, mo->y);
		MSG_WriteLong (&cl->reliablebuf, mo->z);

		MSG_WriteMarker(&cl->reliablebuf, svc_explodemissile);
		MSG_WriteShort(&cl->reliablebuf, mo->netid);
	}
}

//
// SV_SendPlayerInfo
//
// Sends a player their current inventory
//
void SV_SendPlayerInfo(player_t &player)
{
	client_t *cl = &player.client;

	MSG_WriteMarker (&cl->reliablebuf, svc_playerinfo);

	// [AM] 9 weapons, 6 cards, 1 backpack = 16 bits
	uint16_t booleans = 0;
	for (int i = 0; i < NUMWEAPONS; i++)
	{
		if (player.weaponowned[i])
			booleans |= (1 << i);
	}
	for (int i = 0; i < NUMCARDS; i++)
	{
		if (player.cards[i])
			booleans |= (1 << (i + NUMWEAPONS));
	}
	if (player.backpack)
		booleans |= (1 << (NUMWEAPONS + NUMCARDS));
	MSG_WriteShort(&cl->reliablebuf, booleans);

	for (int i = 0; i < NUMAMMO; i++)
	{
		MSG_WriteShort (&cl->reliablebuf, player.maxammo[i]);
		MSG_WriteShort (&cl->reliablebuf, player.ammo[i]);
	}

	MSG_WriteByte (&cl->reliablebuf, player.health);
	MSG_WriteByte (&cl->reliablebuf, player.armorpoints);
	MSG_WriteByte (&cl->reliablebuf, player.armortype);
	MSG_WriteByte (&cl->reliablebuf, player.readyweapon);

	for (int i = 0; i < NUMPOWERS; i++)
		MSG_WriteShort(&cl->reliablebuf, player.powers[i]);
}

//
// SV_PreservePlayer
//
void SV_PreservePlayer(player_t &player)
{
	if (!serverside || sv_gametype != GM_COOP || !validplayer(player) || !player.ingame())
		return;

	if(!unnatural_level_progression)
		player.playerstate = PST_LIVE; // denis - carry weapons and keys over to next level

	G_DoReborn(player);

	SV_SendPlayerInfo(player);
}

VERSION_CONTROL (sv_main_cpp, "$Id$")
