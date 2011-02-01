// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2010 by The Odamex Team.
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


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <time.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#endif

#include "doomtype.h"
#include "doomstat.h"
#include "dstrings.h"
#include "d_player.h"
#include "s_sound.h"
#include "gi.h"
#include "d_net.h"
#include "g_game.h"
#include "p_local.h"
#include "sv_main.h"
#include "sv_sqp.h"
#include "sv_sqpold.h"
#include "sv_master.h"
#include "i_system.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_random.h"
#include "vectors.h"
#include "p_ctf.h"
#include "w_wad.h"
#include "md5.h"
#include "p_mobj.h"

#include <algorithm>
#include <sstream>
#include <vector>

extern void G_DeferedInitNew (char *mapname);
extern level_locals_t level;

// denis - game manipulation, but no fancy gfx
bool clientside = false, serverside = true;
bool predicting = false;
baseapp_t baseapp = server;

extern bool HasBehavior;

bool step_mode = false;

#define IPADDRSIZE 4	// GhostlyDeath -- Someone might want to do IPv6 junk

typedef struct
{
	short ip[IPADDRSIZE];
	std::string Reason;
} BanEntry_t;

const short RANGEBAN = -1;	// ban everyone! =)

std::vector<BanEntry_t> BanList;		// People who are banned
std::vector<BanEntry_t> WhiteList;		// people who are [accidently] banned but can get inside

// General server settings
EXTERN_CVAR(sv_motd)
EXTERN_CVAR(sv_hostname)
EXTERN_CVAR(sv_email)
EXTERN_CVAR(sv_website)
EXTERN_CVAR(sv_waddownload)
EXTERN_CVAR(sv_emptyreset)
EXTERN_CVAR(sv_clientcount)
EXTERN_CVAR(sv_globalspectatorchat)
EXTERN_CVAR(sv_allowtargetnames)
EXTERN_CVAR(sv_flooddelay)
EXTERN_CVAR(sv_maxrate)

void SexMessage (const char *from, char *to, int gender);

CVAR_FUNC_IMPL (sv_maxclients)	// Describes the max number of clients that are allowed to connect. - does not work yet
{
	if(var > MAXPLAYERS)
		var.Set(MAXPLAYERS);

	if(var < 0)
		var.Set((float)0);

	while(players.size() > sv_maxclients)
	{
		int last = players.size() - 1;
		SV_DropClient(players[last]);
		players.erase(players.begin() + last);
	}

	// repair mo after player pointers are reset
	for(size_t i = 0; i < players.size(); i++)
	{
		if(players[i].mo)
			players[i].mo->player = &players[i];
	}

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());
	//R_InitTranslationTables();
}

CVAR_FUNC_IMPL (sv_maxplayers)
{
	// [Nes] - Force extras to become spectators.
	int normalcount = 0;

	if (var < 0)
		var.Set((float)0);

	if (var > MAXPLAYERS)
		var.Set(MAXPLAYERS);

	for (size_t i = 0; i < players.size(); i++)
	{
		if (!players[i].spectator)
		{
			normalcount++;

			if (normalcount > var)
			{
				for (size_t j = 0; j < players.size(); j++) {
					MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
					MSG_WriteByte (&(players[j].client.reliablebuf), players[i].id);
					MSG_WriteByte (&(players[j].client.reliablebuf), true);
				}
				SV_BroadcastPrintf (PRINT_HIGH, "%s became a spectator.\n", players[i].userinfo.netname);
				MSG_WriteMarker (&players[i].client.reliablebuf, svc_print);
				MSG_WriteByte (&players[i].client.reliablebuf, PRINT_CHAT);
				MSG_WriteString (&players[i].client.reliablebuf, "Active player limit reduced. You are now a spectator!\n");
				players[i].spectator = true;
				players[i].playerstate = PST_LIVE;
				players[i].joinafterspectatortime = level.time;
			}
		}
	}
}

EXTERN_CVAR (sv_allowcheats)
EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_timelimit)
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

// Teamplay/CTF
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_friendlyfire)

// Private server settings
CVAR_FUNC_IMPL (join_password)
{
	if(strlen(var.cstring()))
		Printf(PRINT_HIGH, "join password set");
}

CVAR_FUNC_IMPL (spectate_password)
{
	if(strlen(var.cstring()))
		Printf(PRINT_HIGH, "spectate password set");
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

EXTERN_CVAR (sv_antiwallhack)
EXTERN_CVAR (sv_speedhackfix)

client_c clients;


#define CLIENT_TIMEOUT 65 // 65 seconds

QWORD gametime;

void SV_UpdateConsolePlayer(player_t &player);

void SV_CheckTeam (player_t & playernum);
team_t SV_GoodTeam (void);
void SV_ForceSetTeam (player_t &who, int team);

void SV_SendServerSettings (client_t *cl);
void SV_ServerSettingChange (void);

// some doom functions
void P_KillMobj (AActor *source, AActor *target, AActor *inflictor, bool joinkill);
bool P_CheckSightEdges (const AActor* t1, const AActor* t2, float radius_boost = 0.0);
bool P_CheckSightEdges2 (const AActor* t1, const AActor* t2, float radius_boost = 0.0);

void SV_WinCheck (void);

// denis - kick player
BEGIN_COMMAND (kick)
{
	if (argc < 2)
		return;

	player_t &player = idplayer(atoi(argv[1]));

	if(!validplayer(player))
	{
		Printf(PRINT_HIGH, "bad client number: %d\n", atoi(argv[1]));
		return;
	}

	if(!player.ingame())
	{
		Printf(PRINT_HIGH, "client %d not in game\n", atoi(argv[1]));
		return;
	}

	if (argc > 2)
	{
		std::string reason = BuildString(argc - 2, (const char **)(argv + 2));
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked from the server! (Reason: %s)\n", player.userinfo.netname, reason.c_str());
	}
	else
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked from the server!\n", player.userinfo.netname);

	player.client.displaydisconnect = false;
	SV_DropClient(player);
}
END_COMMAND (kick)

//
// Nes - IP Lists: bans(BanList), exceptions(WhiteList)
//
void SV_BanStringify(std::string *ToStr = NULL, short *ip = NULL)
{
	if (!ToStr || !ip)
		return;

	for (int i = 0; i < IPADDRSIZE; i++)
	{
		if (ip[i] == RANGEBAN)
			*ToStr += '*';
		else
		{
			char bleh[5];
			sprintf(bleh, "%i", ip[i]);
			*ToStr += bleh;
		}

		if (i < (IPADDRSIZE - 1))
			*ToStr += '.';
	}
}

// Nes - Make IP for the temporary BanEntry.
void SV_IPListMakeIP (BanEntry_t *tBan, std::string IPtoBan)
{
	std::string Oct;//, Oct2, Oct3, Oct4;

	// There is garbage in IPtoBan
	IPtoBan = IPtoBan.substr(0, IPtoBan.find(' '));

	for (int i = 0; i < IPADDRSIZE; i++)
	{
		int loc = 0;
		char *seek;

		Oct = IPtoBan.substr(0, Oct.find("."));
		IPtoBan = IPtoBan.substr(IPtoBan.find(".") + 1);

		seek = const_cast<char*>(Oct.c_str());

		while (*seek)
		{
			if (*seek == '.')
				break;
			loc++;
			seek++;
		}

		Oct = Oct.substr(0, loc);

		if ((*(Oct.c_str()) == '*') || (*(Oct.c_str()) == 0))
			(*tBan).ip[i] = RANGEBAN;
		else
			(*tBan).ip[i] = atoi(Oct.c_str());
	}
}

// Nes - Add IP to a certain IP list.
void SV_IPListAdd (std::vector<BanEntry_t> *list, std::string listname, std::string IPtoBan, std::string reason)
{
	BanEntry_t tBan;	// GhostlyDeath -- Temporary Ban Holder
	std::string IP;

	SV_IPListMakeIP(&tBan, IPtoBan);

	for (size_t i = 0; i < (*list).size(); i++)
	{
		bool match = false;

		for (int j = 0; j < IPADDRSIZE; j++)
			if ((tBan.ip[j] == (*list)[i].ip[j]) || ((*list)[i].ip[j] == RANGEBAN) &&
				((j > 0 && match) || (j == 0 && !match)))
				match = true;
			else
			{
				match = false;
				break;
			}

		if (match)
		{
			SV_BanStringify(&IP, tBan.ip);
			Printf(PRINT_HIGH, "%s on %s already exists!\n", listname.c_str(), IP.c_str());
			return;
		}
	}

	tBan.Reason = reason;

	(*list).push_back(tBan);
	SV_BanStringify(&IP, tBan.ip);
	Printf(PRINT_HIGH, "%s on %s added.\n", listname.c_str(), IP.c_str());
}

// Nes - Delete IP from a certain IP list.
void SV_IPListDelete (std::vector<BanEntry_t> *list, std::string listname, std::string IPtoBan)
{
	if (!(*list).size())
		Printf(PRINT_HIGH, "%s list is empty.\n", listname.c_str());
	else {
		BanEntry_t tBan;	// GhostlyDeath -- Temporary Ban Holder
		std::string IP;
		int RemovalCount = 0;
		size_t i;

		SV_IPListMakeIP(&tBan, IPtoBan);

		for (i = 0; i < (*list).size(); i++)
		{
			bool match = false;

			for (int j = 0; j < IPADDRSIZE; j++)
				if (tBan.ip[j] == (*list)[i].ip[j])
					match = true;
				else
				{
					match = false;
					break;
				}

			if (match)
			{
				(*list)[i].ip[0] = 32000;
				RemovalCount++;
			}
		}

		i = 0;

		while (i < (*list).size())
		{
			if ((*list)[i].ip[0] == 32000)
				(*list).erase((*list).begin() + i);
			else
				i++;
		}

		if (RemovalCount == 0)
			Printf(PRINT_HIGH, "%s entry not found.\n", listname.c_str());
		else
			Printf(PRINT_HIGH, "%i %ss removed.\n", RemovalCount, listname.c_str());
	}
}

// Nes - List a certain IP list.
void SV_IPListDisplay (std::vector<BanEntry_t> *list, std::string listname)
{
	if (!(*list).size())
		Printf(PRINT_HIGH, "%s list is empty.\n", listname.c_str());
	else {
		for (size_t i = 0; i < (*list).size(); i++)
		{
			std::string IP;
			SV_BanStringify(&IP, (*list)[i].ip);
			Printf(PRINT_HIGH, "%s #%i: %s (Reason: %s)\n", listname.c_str(), i + 1, IP.c_str(), (*list)[i].Reason.c_str());
		}

		Printf(PRINT_HIGH, "%s list has %i entries.\n", listname.c_str(), (*list).size());
	}
}

// Nes - Clears a certain IP list.
void SV_IPListClear (std::vector<BanEntry_t> *list, std::string listname)
{
	if (!(*list).size())
		Printf(PRINT_HIGH, "%s list is already empty!\n", listname.c_str());
	else {
		Printf(PRINT_HIGH, "All %i %ss removed.\n", (*list).size(), listname.c_str());
		(*list).clear();
	}
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

BEGIN_COMMAND(addban)
{
	std::string reason;

	if (argc < 2)
		return;

	if (argc >= 3)
		reason = BuildString(argc - 2, (const char **)(argv + 2));
	else
		reason = "none given";

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListAdd (&BanList, "Ban", IPtoBan, reason);
}
END_COMMAND(addban)

BEGIN_COMMAND(delban)
{
	if (argc < 2)
		return;

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListDelete (&BanList, "Ban", IPtoBan);
}
END_COMMAND(delban)

BEGIN_COMMAND(banlist)
{
	SV_IPListDisplay (&BanList, "Ban");
}
END_COMMAND(banlist)

BEGIN_COMMAND(clearbans)
{
	SV_IPListClear (&BanList, "Ban");
}
END_COMMAND(clearbans)

BEGIN_COMMAND(addexception)
{
	std::string reason;

	if (argc < 2)
		return;

	if (argc >= 3)
		reason = BuildString(argc - 2, (const char **)(argv + 2));
	else
		reason = "none given";

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListAdd (&WhiteList, "Exception", IPtoBan, reason);
}
END_COMMAND(addexception)

BEGIN_COMMAND(delexception)
{
	if (argc < 2)
		return;

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListDelete (&WhiteList, "Exception", IPtoBan);
}
END_COMMAND(delexception)

BEGIN_COMMAND(exceptionlist)
{
	SV_IPListDisplay (&WhiteList, "Exception");
}
END_COMMAND(exceptionlist)

BEGIN_COMMAND(clearexceptions)
{
	SV_IPListClear (&WhiteList, "Exception");
}
END_COMMAND(clearexceptions)

// Nes - Same as kick, only add ban.
BEGIN_COMMAND(kickban)
{
	if (argc < 2)
		return;

	player_t &player = idplayer(atoi(argv[1]));
	std::string command, tempipstring;
	short tempip[IPADDRSIZE];

	// Check for validity...
	if(!validplayer(player))
	{
		Printf(PRINT_HIGH, "bad client number: %d\n", atoi(argv[1]));
		return;
	}

	if(!player.ingame())
	{
		Printf(PRINT_HIGH, "client %d not in game\n", atoi(argv[1]));
		return;
	}

	// Generate IP for the ban...
	for (int i = 0; i < IPADDRSIZE; i++)
		tempip[i] = (short)player.client.address.ip[i];

	SV_BanStringify(&tempipstring, tempip);

	// The kick...
	if (argc > 2)
	{
		std::string reason = BuildString(argc - 2, (const char **)(argv + 2));
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kickbanned from the server! (Reason: %s)\n", player.userinfo.netname, reason.c_str());
	}
	else
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kickbanned from the server!\n", player.userinfo.netname);

	player.client.displaydisconnect = false;
	SV_DropClient(player);

	// ... and the ban!
	command = "addban ";
	command += tempipstring;
	if (argc > 2) {
		command += " ";
		command += BuildString(argc - 2, (const char **)(argv + 2));
	}
	AddCommandString(command);
}
END_COMMAND(kickban)

BEGIN_COMMAND (say)
{
	if (argc > 1)
	{
		std::string chat = BuildString (argc - 1, (const char **)(argv + 1));
		SV_BroadcastPrintf (PRINT_CHAT, "[console]: %s\n", chat.c_str());
	}

}
END_COMMAND (say)


BEGIN_COMMAND (rquit)
{
	SV_SendReconnectSignal();
	exit (0);
}
END_COMMAND (rquit)


BEGIN_COMMAND (quit)
{
	exit (0);
}
END_COMMAND (quit)


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

	gametime = I_GetTime ();
	
	// Nes - Connect with the master servers. (If valid)
	SV_InitMasters();
}


int SV_GetFreeClient(void)
{
	if(players.size() >= sv_maxclients)
		return -1;

	players.push_back(player_t());

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());

	// repair mo after player pointers are reset
	for(size_t i = 0; i < players.size() - 1; i++)
	{
		if(players[i].mo)
			players[i].mo->player = &players[i];
	}

	// generate player id
	size_t max = 0;
	for (size_t c = 0; c < players.size() - 1; c++)
	{
		max = (max>players[c].id) ? max : players[c].id;
	}
	players[players.size() - 1].id = max + 1;

	return players.size() - 1;
}

player_t &SV_FindPlayerByAddr(void)
{
	size_t i;

	for (i = 0; i < players.size(); i++)
	{
		if (NET_CompareAdr(players[i].client.address, net_from))
		   return players[i];
	}

	return idplayer(0);
}

//
// SV_CheckTimeouts
// If a packet has not been received from a client in CLIENT_TIMEOUT
// seconds, drop the conneciton.
//
void SV_CheckTimeouts (void)
{
	for (size_t i = 0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		if (gametic - cl->last_received == CLIENT_TIMEOUT*35)
		    SV_DropClient(players[i]);
	}
}

//
// SV_GetPackets
//
void SV_GetPackets (void)
{
	while (NET_GetPacket())
	{
		player_t &player = SV_FindPlayerByAddr();

		if (!validplayer(player)) // no client with net_from address
		{
			// apparently, someone is trying to connect
			if (gamestate == GS_LEVEL)
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

	size_t i = 0;
    BOOL resetlevel = false;

	// remove disconnected players
	while (i < players.size())
	{
        if(players[i].playerstate == PST_DISCONNECT)
		{
            players.erase(players.begin() + i);

			// update tracking cvar
			sv_clientcount.ForceSet(players.size());

            if (sv_emptyreset && players.size() == 0)
                resetlevel = true;
        }
		else
        {
            ++i;
        }
	}

	// repair mo after player pointers are reset
	for(i = 0; i < players.size(); ++i)
	{
		if(players[i].mo)
			players[i].mo->player = &players[i];
	}

	if (resetlevel)
    {
        resetlevel = false;

        G_DeferedInitNew(level.mapname);
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
	int        sfx_id;
	client_t  *cl;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	for (size_t i = 0; i < players.size(); i++)
	{
		cl = &clients[i];

		int x = 0, y = 0;
		byte vol = 0;

		if(mo)
		{
			x = mo->x;
			y = mo->y;

			vol = SV_PlayerHearingLoss(players[i], x, y);
		}

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
		MSG_WriteByte (&cl->netbuf, vol);
	}

}


void SV_Sound (player_t &pl, AActor *mo, byte channel, const char *name, byte attenuation)
{
	int sfx_id;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	int x = 0, y = 0;
	byte vol = 0;

	if(mo)
	{
		x = mo->x;
		y = mo->y;

		vol = SV_PlayerHearingLoss(pl, x, y);
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
	MSG_WriteByte (&cl->netbuf, vol);
}

//
// UV_SoundAvoidPlayer
// Sends a sound to clients, but doesn't send it to client 'player'.
//
void UV_SoundAvoidPlayer (AActor *mo, byte channel, const char *name, byte attenuation)
{
	int        sfx_id;
	client_t  *cl;

	if(!mo->player)
		return;

	player_t &pl = *mo->player;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

    for (size_t i = 0; i < players.size(); i++)
    {
		if(&pl == &players[i])
			continue;

        cl = &clients[i];

		int x = 0, y = 0;
		byte vol = 0;

		if(mo)
		{
			x = mo->x;
			y = mo->y;

			vol = SV_PlayerHearingLoss(players[i], x, y);
		}

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
		MSG_WriteByte (&cl->netbuf, vol);
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

	for (size_t i = 0; i < players.size(); i++ )
	{
		if (players[i].ingame())
		{
			if (players[i].userinfo.team == team)	// [Toke - Teams]
			{
				cl = &clients[i];

				MSG_WriteMarker  (&cl->netbuf, svc_startsound );
                if (players[i].mo == NULL)
                    MSG_WriteShort (&cl->netbuf, 0);
                else
                    MSG_WriteShort (&cl->netbuf, players[i].mo->netid);
				MSG_WriteLong (&cl->netbuf, 0);
				MSG_WriteLong (&cl->netbuf, 0);
				MSG_WriteByte  (&cl->netbuf, channel );
				MSG_WriteByte  (&cl->netbuf, sfx_id );
			    MSG_WriteByte  (&cl->netbuf, attenuation );
			    MSG_WriteByte  (&cl->netbuf, 255 );
			}
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

	for (size_t i = 0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		cl = &clients[i];

		byte vol;

		if((vol = SV_PlayerHearingLoss(players[i], x, y)))
		{
			MSG_WriteMarker (&cl->netbuf, svc_soundorigin);
			MSG_WriteLong (&cl->netbuf, x);
			MSG_WriteLong (&cl->netbuf, y);
			MSG_WriteByte (&cl->netbuf, channel);
			MSG_WriteByte (&cl->netbuf, sfx_id);
			MSG_WriteByte (&cl->netbuf, attenuation);
			MSG_WriteByte (&cl->netbuf, vol);
		}
	}
}

//
// SV_UpdateFrags
//
void SV_UpdateFrags (player_t &player)
{
    for (size_t i=0; i < players.size(); i++)
    {
        client_t *cl = &clients[i];

        MSG_WriteMarker (&cl->reliablebuf, svc_updatefrags);
        MSG_WriteByte (&cl->reliablebuf, player.id);
        if(sv_gametype != GM_COOP)
            MSG_WriteShort (&cl->reliablebuf, player.fragcount);
        else
            MSG_WriteShort (&cl->reliablebuf, player.killcount);
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
	MSG_WriteString (&cl->reliablebuf, p->userinfo.netname);
	MSG_WriteByte	(&cl->reliablebuf, p->userinfo.team);
	MSG_WriteLong	(&cl->reliablebuf, p->userinfo.gender);
	MSG_WriteLong	(&cl->reliablebuf, p->userinfo.color);
	MSG_WriteString	(&cl->reliablebuf, skins[p->userinfo.skin].name);  // [Toke - skins]
	MSG_WriteShort	(&cl->reliablebuf, time(NULL) - p->JoinTime);
}

//
//	SV_SetupUserInfo
//
//	Stores a players userinfo
//
void SV_SetupUserInfo (player_t &player)
{
	int		old_team;
	const char   *skin;
	char	old_netname[MAXPLAYERNAME+1];
	std::string		gendermessage;

	player_t* p;
	p = &player;

	// store players team number
	old_team = p->userinfo.team;

	// Store player's old name for comparison.
	strncpy (old_netname, p->userinfo.netname, sizeof(old_netname));

	// Read user info
	strncpy (p->userinfo.netname, MSG_ReadString(), sizeof(p->userinfo.netname));
	p->userinfo.team	= (team_t)MSG_ReadByte();	// [Toke - Teams]
	p->userinfo.gender	= (gender_t)MSG_ReadLong();
	p->prefcolor		= MSG_ReadLong();
	p->userinfo.color	= p->prefcolor;

	skin = MSG_ReadString();	// [Toke - Skins] Player skin

	// Make sure the skin is valid
	p->userinfo.skin = R_FindSkin(skin);

	p->userinfo.aimdist = MSG_ReadLong();

	// Make sure the aimdist is valid
	if (p->userinfo.aimdist < 0)
		p->userinfo.aimdist = 0;
	if (p->userinfo.aimdist > 5000)
		p->userinfo.aimdist = 5000;

	// Make sure the gender is valid
	if(p->userinfo.gender >= NUMGENDER)
		p->userinfo.gender = GENDER_NEUTER;

	// Compare names and broadcast if different.
	if (strncmp(old_netname, "", sizeof(old_netname)) && strncmp(p->userinfo.netname, old_netname, sizeof(old_netname))) {
		switch (p->userinfo.gender) {
			case 0: gendermessage = "his";  break;
			case 1: gendermessage = "her";  break;
			default: gendermessage = "its";  break;
		}
		SV_BroadcastPrintf (PRINT_HIGH, "%s changed %s name to %s.\n", old_netname, gendermessage.c_str(), p->userinfo.netname);
	}

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
		SV_CheckTeam (player);

	// kill player if team is changed
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
		if (p->mo && p->userinfo.team != old_team)
			P_DamageMobj (p->mo, 0, 0, 1000, 0);


	// inform all players of new player info
	for (size_t i = 0; i < players.size(); i++ )
	{
		SV_SendUserInfo (player, &clients[i]);
	}
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
	Printf (PRINT_HIGH, "Forcing %s to %s team\n", who.userinfo.netname, team == TEAM_NONE ? "NONE" : team_names[team]);
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
	switch (player.userinfo.team)
	{
		case TEAM_BLUE:
		case TEAM_RED:

		if(sv_gametype == GM_CTF && player.userinfo.team < 2)
			break;

		if(sv_gametype != GM_CTF && player.userinfo.team < sv_teamsinplay)
			break;

		default:
			SV_ForceSetTeam (player, SV_GoodTeam ());
			break;
	}

	// Force colors
	switch (player.userinfo.team)
	{
		case TEAM_BLUE:
			player.userinfo.color = (0x000000FF);
			break;
		case TEAM_RED:
			player.userinfo.color = (0x00FF0000);
			break;
		default:
			break;
	}
}

//
//	SV_GoodTeam
//
//	Finds a working team
//
team_t SV_GoodTeam (void)
{
	for(size_t i = 0; i < NUMTEAMS; i++)
		if((sv_gametype == GM_CTF && i < 2) || (sv_gametype != GM_CTF && i < sv_teamsinplay))
			return (team_t)i;

	I_Error ("Teamplay is set and no teams are enabled!\n");

	return TEAM_NONE;
}


//
// [denis] SV_ClientHearingLoss
// determine if an actor should be able to hear a sound
//
fixed_t P_AproxDistance2 (AActor *listener, fixed_t x, fixed_t y);
byte SV_PlayerHearingLoss(player_t &pl, fixed_t &x, fixed_t &y)
{
	AActor *ear = pl.camera;

	if(!ear)
		return 0;

	float dist = (FIXED2FLOAT(P_AproxDistance2 (ear, x, y)));

	if(S_CLIPPING_DIST - dist < (float)S_CLIPPING_DIST/4)
	{
		// at this range, you shouldn't be able to tell the direction
		x = -1;
		y = -1;
	}
	else if(dist)
	{
		// randomly scramble this distance
		float a = rand()%(int)(1 + dist*dist/(float)(S_CLIPPING_DIST));
		float b = rand()%(int)(1 + dist*dist/(float)(S_CLIPPING_DIST));
		float c = rand()%(int)(1 + dist*dist/(float)(S_CLIPPING_DIST));
		float d = rand()%(int)(1 + dist*dist/(float)(S_CLIPPING_DIST));

		x += FLOAT2FIXED(a) - FLOAT2FIXED(b);
		y += FLOAT2FIXED(c) - FLOAT2FIXED(d);
	}

	if (dist >= S_CLIPPING_DIST)
	{
		if (!(level.flags & LEVEL_NOSOUNDCLIPPING))
			return 0; // sound is beyond the hearing range...
	}

	byte vol = (byte)((1 - dist/(float)S_CLIPPING_DIST) * 255);

	return vol;
}

//
// SV_SendMobjToClient
//
void SV_SendMobjToClient(AActor *mo, client_t *cl)
{
	MSG_WriteMarker(&cl->reliablebuf, svc_spawnmobj);
	MSG_WriteLong(&cl->reliablebuf, mo->x);
	MSG_WriteLong(&cl->reliablebuf, mo->y);
	MSG_WriteLong(&cl->reliablebuf, mo->z);
	MSG_WriteLong(&cl->reliablebuf, mo->angle);

	MSG_WriteShort(&cl->reliablebuf, mo->type);
	MSG_WriteShort(&cl->reliablebuf, mo->netid);
	MSG_WriteByte(&cl->reliablebuf, mo->rndindex);
	MSG_WriteShort(&cl->reliablebuf, (mo->state - states)); // denis - sending state fixes monster ghosts appearing under doors

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
	else if(player.mo && mo->player && !sv_antiwallhack)
		ok = true;
	else if (	player.mo && mo->player && sv_antiwallhack &&
				player.spectator)	// GhostlyDeath -- Spectators MUST see players to F12 properly
		ok = true;
	else if (	player.mo && mo->player && sv_antiwallhack &&
				player.spectator)	// GhostlyDeath -- Spectators MUST see players to F12 properly
		ok = true;

	else if(player.mo && mo->player && sv_antiwallhack && 
         ((HasBehavior && P_CheckSightEdges2(player.mo, mo, 5)) || (!HasBehavior && P_CheckSightEdges(player.mo, mo, 5)))/*player.awaresector[sectors - mo->subsector->sector]*/)
		ok = true;

	std::vector<size_t>::iterator a = std::find(mo->players_aware.begin(), mo->players_aware.end(), player.id);
	bool previously_ok = (a != mo->players_aware.end());

	client_t *cl = &player.client;

	if(!ok && previously_ok)
	{
		mo->players_aware.erase(a);

		MSG_WriteMarker (&cl->reliablebuf, svc_removemobj);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);

		return true;
	}
	else if(!previously_ok && ok)
	{
		mo->players_aware.push_back(player.id);

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
	if(!mo)
		return;

	for (size_t i = 0; i < players.size(); i++)
	{
		if(mo->player)
			SV_AwarenessUpdate(players[i], mo);
		else
			players[i].to_spawn.push(mo->ptr());
	}
}

//
// [denis] SV_IsPlayerAllowedToSee
// determine if a client should be able to see an actor
//
bool SV_IsPlayerAllowedToSee(player_t &p, AActor *mo)
{
	if (mo->flags & MF_SPECTATOR)
		return false; // GhostlyDeath -- always false, as usual!
	else
		return std::find(mo->players_aware.begin(), mo->players_aware.end(), p.id) != mo->players_aware.end();
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
	size_t i, e = players.size();

	for(i = 0; i != e; i++)
	{
		player_t &pl = players[i];

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
	for (int s=0; s<numsectors; s++)
	{
		sector_t* sec = &sectors[s];

		if (sec->moveable)
		{
			MSG_WriteMarker (&cl->reliablebuf, svc_sector);
			MSG_WriteShort (&cl->reliablebuf, s);
			MSG_WriteShort (&cl->reliablebuf, sec->floorheight>>FRACBITS);
			MSG_WriteShort (&cl->reliablebuf, sec->ceilingheight>>FRACBITS);
			MSG_WriteShort (&cl->reliablebuf, sec->floorpic);
			MSG_WriteShort (&cl->reliablebuf, sec->ceilingpic);

			/*if(sec->floordata->IsKindOf(RUNTIME_CLASS(DMover)))
			{
				DMover *d = sec->floordata;
				MSG_WriteByte(&cl->netbuf, 1);
				MSG_WriteByte(&cl->netbuf, d->get_speed());
				MSG_WriteByte(&cl->netbuf, d->get_dest());
				MSG_WriteByte(&cl->netbuf, d->get_crush());
				MSG_WriteByte(&cl->netbuf, d->get_floorOrCeiling());
				MSG_WriteByte(&cl->netbuf, d->get_direction());
			}
			else
				MSG_WriteByte(&cl->netbuf, 0);

			if(sec->ceilingdata->IsKindOf(RUNTIME_CLASS(DMover)))
			{
				MSG_WriteByte(&cl->netbuf, 1);
				MSG_WriteByte(&cl->netbuf, d->get_speed());
				MSG_WriteByte(&cl->netbuf, d->get_dest());
				MSG_WriteByte(&cl->netbuf, d->get_crush());
				MSG_WriteByte(&cl->netbuf, d->get_floorOrCeiling());
				MSG_WriteByte(&cl->netbuf, d->get_direction());
			}
			else
				MSG_WriteByte(&cl->netbuf, 0);*/
		}
	}
}

//
// SV_UpdateMovingSectors
// Update doors, floors, ceilings etc... that are actively moving
//
void SV_UpdateMovingSectors(client_t* cl)
{
	for (int s=0; s<numsectors; s++)
	{
		sector_t* sec = &sectors[s];

		if(sec->floordata)
		{
			if(sec->floordata->IsKindOf(RUNTIME_CLASS(DPlat)))
			{
				MSG_WriteMarker (&cl->netbuf, svc_movingsector);
				MSG_WriteLong (&cl->netbuf, cl->lastclientcmdtic);
				MSG_WriteShort (&cl->netbuf, s);
				MSG_WriteLong (&cl->netbuf, sec->floorheight);
				MSG_WriteLong (&cl->netbuf, sec->ceilingheight);

				DPlat *plat = (DPlat *)sec->floordata;
				byte state; int count;
				plat->GetState(state, count);

				MSG_WriteByte (&cl->netbuf, state);
				MSG_WriteLong (&cl->netbuf, count);
			}
			else if(sec->floordata->IsKindOf(RUNTIME_CLASS(DMovingFloor)))
			{
				MSG_WriteMarker (&cl->netbuf, svc_movingsector);
				MSG_WriteLong (&cl->netbuf, cl->lastclientcmdtic);
				MSG_WriteShort (&cl->netbuf, s);
				MSG_WriteLong (&cl->netbuf, sec->floorheight);
				MSG_WriteLong (&cl->netbuf, sec->ceilingheight);

				MSG_WriteByte (&cl->netbuf, 0);
				MSG_WriteLong (&cl->netbuf, 0);
			}
		}
	}
}

//
// SV_ClientFullUpdate
//
void SV_ClientFullUpdate (player_t &pl)
{
	size_t	i;
	client_t *cl = &pl.client;

	// send player's info to the client
	for (i=0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		if(players[i].mo)
			SV_AwarenessUpdate(pl, players[i].mo);

		SV_SendUserInfo(players[i], cl);

		if (cl->reliablebuf.cursize >= 600)
			if(!SV_SendPacket(pl))
				return;
	}

	// update frags/points/spectate
	for (i = 0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		MSG_WriteMarker(&cl->reliablebuf, svc_updatefrags);
		MSG_WriteByte(&cl->reliablebuf, players[i].id);
		if(sv_gametype != GM_COOP)
			MSG_WriteShort(&cl->reliablebuf, players[i].fragcount);
		else
			MSG_WriteShort(&cl->reliablebuf, players[i].killcount);
		MSG_WriteShort(&cl->reliablebuf, players[i].deathcount);
		MSG_WriteShort(&cl->reliablebuf, players[i].points);

		MSG_WriteMarker (&cl->reliablebuf, svc_spectate);
		MSG_WriteByte (&cl->reliablebuf, players[i].id);
		MSG_WriteByte (&cl->reliablebuf, players[i].spectator);
	}

	// [deathz0r] send team frags/captures if teamplay is enabled
	if(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_teampoints);
		for (i = 0; i < NUMTEAMS; i++)
			MSG_WriteShort (&cl->reliablebuf, TEAMpoints[i]);
	}

	SV_UpdateHiddenMobj();

	// update flags
	if(sv_gametype == GM_CTF)
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

	SV_SendPacket(pl);
}

//
//	SV_SendServerSettings
//
//	Sends server setting info
//

void SV_SendServerSettings (client_t *cl)
{
	// GhostlyDeath <June 19, 2008> -- Loop through all CVARs and send the CVAR_SERVERINFO stuff only
	cvar_t *var = GetFirstCvar();

	MSG_WriteMarker(&cl->reliablebuf, svc_serversettings);

	while (var)
	{
		if (var->flags() & CVAR_SERVERINFO)
		{
			MSG_WriteByte(&cl->reliablebuf, 1);
			MSG_WriteString(&cl->reliablebuf, var->name());
			MSG_WriteString(&cl->reliablebuf, var->cstring());
		}

		var = var->GetNext();
	}

	MSG_WriteByte(&cl->reliablebuf, 2);
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

	for (size_t i = 0; i < players.size(); i++)
		SV_SendServerSettings (&clients[i]);
}

//
//  SV_BanCheck
//
//  Checks a connecting player against a banlist
//
bool SV_BanCheck (client_t *cl, int n)
{
	for (size_t i = 0; i < BanList.size(); i++)
	{
		bool match = false;
		bool exception = false;

		for (int j = 0; j < IPADDRSIZE; j++)
		{
			if ((cl->address.ip[j] == BanList[i].ip[j]) || (BanList[i].ip[j] == RANGEBAN) &&
				((j > 0 && match) || (j == 0 && !match)))
				match = true;
			else
			{
				match = false;
				break;
			}
		}

		// Now see if there is an exception on our ban...
		if (WhiteList.empty() == false)
		{
			for (size_t k = 0; k < WhiteList.size(); k++)
			{
				exception = false;

				for (int j = 0; j < IPADDRSIZE; j++)
				{
					if ((cl->address.ip[j] == WhiteList[k].ip[j]) || (WhiteList[k].ip[j] == RANGEBAN) &&
						((j > 0 && exception) || (j == 0 && !exception)))
						exception = true;
					else
					{
						exception = false;
						break;
					}
				}

				if (exception)
					break;	// we already know they are allowed in
			}
		}

		if (match && !exception)
		{
			std::string BanStr;
			BanStr += "\nYou are banned! (reason: ";
			//if (*(BanList[i].Reason.c_str()))
				BanStr += BanList[i].Reason;
			//else
			//	BanStr += "none given";
			BanStr += ")\n";

			MSG_WriteMarker   (&cl->reliablebuf, svc_print);
			MSG_WriteByte   (&cl->reliablebuf, PRINT_HIGH);
			MSG_WriteString (&cl->reliablebuf, BanStr.c_str());

			// GhostlyDeath -- Do we include the e-mail or no?
			if (*(sv_email.cstring()) == 0)
			{
				MSG_WriteMarker(&cl->reliablebuf, svc_print);
				MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
				MSG_WriteString(&cl->reliablebuf, "If you feel there has been an error, contact the server host. (No e-mail given)\n");
			}
			else
			{
				std::string ErrorStr;
				ErrorStr += "If you feel there has been an error, contact the server host at ";
				ErrorStr += sv_email.cstring();
				ErrorStr += "\n\n";
				MSG_WriteMarker(&cl->reliablebuf, svc_print);
				MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
				MSG_WriteString(&cl->reliablebuf, ErrorStr.c_str());
			}

			Printf(PRINT_HIGH, "%s is banned and unable to join! (reason: %s)\n", NET_AdrToString (net_from), BanList[i].Reason.c_str());

			SV_SendPacket (players[n]);
			cl->displaydisconnect = false;
			SV_DropClient(players[n]);
			return true;
		}
		else if (exception)	// don't bother because they'll be allowed multiple times
		{
			std::string BanStr;
			BanStr += "\nBan Exception (reason: ";
			//if (*(WhiteList[i].Reason.c_str()))
				BanStr += WhiteList[i].Reason;
			//else
			//	BanStr += "none given";
			//BanStr += ")\n\n";

			MSG_WriteMarker(&cl->reliablebuf, svc_print);
			MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
			MSG_WriteString(&cl->reliablebuf, BanStr.c_str());

			return false;
		}
	}

	return false;
}

// SV_CheckClientVersion
bool SV_CheckClientVersion(client_t *cl, int n)
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

		SV_SendPacket (players[n]);

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

void SV_ConnectClient (void)
{
	int n;
	size_t i;
	int challenge = MSG_ReadLong();
	client_t  *cl;

    // New querying system
    if (SV_QryParseEnquiry(challenge) == 0)
        return;
    
	if (challenge == LAUNCHER_CHALLENGE)  // for Launcher
	{
		SV_SendServerInfo ();

		return;
	}

	if (challenge != CHALLENGE)
		return;

	if(!SV_IsValidToken(MSG_ReadLong()))
		return;

	// find an open slot
	n = SV_GetFreeClient ();

	if (n == -1)  // a server is full
	{
		static buf_t smallbuf(16);

		MSG_WriteLong (&smallbuf, 0);
		MSG_WriteMarker (&smallbuf, svc_full);

		NET_SendPacket (smallbuf, net_from);

		return;
	}

	Printf (PRINT_HIGH, "%s is trying to connect...\n", NET_AdrToString (net_from));

	cl = &clients[n];

	// clear client network info
	cl->address          = net_from;
	cl->last_received    = gametic;
	cl->reliable_bps     = 0;
	cl->unreliable_bps   = 0;
	cl->lastcmdtic       = 0;
	cl->lastclientcmdtic = 0;
	cl->allow_rcon       = false;

	// generate a random string
	std::stringstream ss;
	ss << time(NULL) << level.time << VERSION << n << NET_AdrToString (net_from);
	cl->digest			 = MD5SUM(ss.str());

	// Set player time
	players[n].JoinTime = time(NULL);

	SZ_Clear (&cl->netbuf);
	SZ_Clear (&cl->reliablebuf);
	SZ_Clear (&cl->relpackets);

	memset (cl->packetseq,  -1, sizeof(cl->packetseq)  );
	memset (cl->packetbegin, 0, sizeof(cl->packetbegin));
	memset (cl->packetsize,  0, sizeof(cl->packetsize) );

	cl->sequence      =  0;
	cl->last_sequence = -1;
	cl->packetnum     =  0;

	cl->version = MSG_ReadShort();
	byte connection_type = MSG_ReadByte();

	if (!SV_CheckClientVersion(cl, n))
	{
		SV_DropClient(players[n]);
		return;
	}

	if (SV_BanCheck(cl, n))
	{
		SV_DropClient(players[n]);
		return;
	}

	// send consoleplayer number
	MSG_WriteMarker (&cl->reliablebuf, svc_consoleplayer);
	MSG_WriteByte (&cl->reliablebuf, players[n].id);
	MSG_WriteString (&cl->reliablebuf, cl->digest.c_str());

    SV_SendPacket(players[n]);

	// get client userinfo
	MSG_ReadByte ();  // clc_userinfo
	SV_SetupUserInfo (players[n]); // send it to other players

	// get rate value
	cl->rate				= MSG_ReadLong();

    std::string passhash = MSG_ReadString();

    if (strlen(join_password.cstring()) && MD5SUM(join_password.cstring()) != passhash)
    {
        MSG_WriteMarker(&cl->reliablebuf, svc_print);
        MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
        MSG_WriteString (&cl->reliablebuf,
                         "Server is passworded, no password specified or bad password\n");

        SV_SendPacket(players[n]);

        SV_DropClient(players[n]);

        return;
    }

	// [Toke] send server settings
	SV_SendServerSettings (cl);

	cl->download.name = "";
	if(connection_type == 1)
	{
		players[n].playerstate = PST_DOWNLOAD;
		return;
	}
	else if(connection_type == 2)
	{
		players[n].playerstate = PST_SPECTATE;
		return;
	}
	else
		players[n].playerstate = PST_REBORN;

	players[n].fragcount	= 0;
	players[n].killcount	= 0;
	players[n].points		= 0;

	if(!step_mode) {
		players[n].spectator	= true;
		for (size_t j = 0; j < players.size(); j++)
		{
			MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
			MSG_WriteByte (&(players[j].client.reliablebuf), players[n].id);
			MSG_WriteByte (&(players[j].client.reliablebuf), true);
		}
	}

	// send a map name
	MSG_WriteMarker   (&cl->reliablebuf, svc_loadmap);
	MSG_WriteString (&cl->reliablebuf, level.mapname);
	G_DoReborn (players[n]);
	SV_ClientFullUpdate (players[n]);
	SV_SendPacket (players[n]);

	SV_BroadcastPrintf (PRINT_HIGH, "%s has connected.\n", players[n].userinfo.netname);

	// tell others clients about it
	for (i = 0; i < players.size(); i++)
	{
		client_t &cl = clients[i];

		MSG_WriteMarker(&cl.reliablebuf, svc_connectclient);
		MSG_WriteByte(&cl.reliablebuf, players[n].id);
	}
	
	SV_MidPrint((char *)sv_motd.cstring(),(player_t *) &players[n], 6);
}

extern bool singleplayerjustdied;

//
// SV_DisconnectClient
//
void SV_DisconnectClient(player_t &who)
{
	size_t i;
	char str[100];
	std::string disconnectmessage;

	// already gone though this procedure?
	if(who.playerstate == PST_DISCONNECT)
		return;

	// tell others clients about it
	for (i = 0; i < players.size(); i++)
	{
	   client_t &cl = clients[i];

	   MSG_WriteMarker(&cl.reliablebuf, svc_disconnectclient);
	   MSG_WriteByte(&cl.reliablebuf, who.id);
	}

	// remove player awareness from all actors
	AActor *mo;
    TThinkerIterator<AActor> iterator;
    while ( (mo = iterator.Next() ) )
    {
		mo->players_aware.erase(
					std::remove(mo->players_aware.begin(), mo->players_aware.end(), who.id),
					mo->players_aware.end());
	}

	if(who.mo)
	{
		if(sv_gametype == GM_CTF) // [Toke - CTF]
			CTF_CheckFlags (who);

		who.mo->Destroy();
		who.mo = AActor::AActorPtr();
	}

	if (who.client.displaydisconnect) {
		// Name and reason for disconnect.
		if (gametic - who.client.last_received == CLIENT_TIMEOUT*35)
			sprintf(str, "%s timed out. (", who.userinfo.netname);
		else
			sprintf(str, "%s disconnected. (", who.userinfo.netname);

		disconnectmessage = str;

		// Spectator status or team name (TDM/CTF).
		if (who.spectator)
			sprintf(str, "SPECTATOR, ");
		else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			sprintf(str, "%s TEAM, ", team_names[who.userinfo.team]);
		else
			*str = '\0';

		disconnectmessage += str;

		// Points (CTF).
		if (sv_gametype == GM_CTF) {
			sprintf(str, "%d POINTS, ", who.points);
			disconnectmessage += str;
		}

		// Frags (DM/TDM/CTF) or Kills (Coop).
		if (sv_gametype != GM_COOP)
			sprintf(str, "%d FRAGS, ", who.fragcount);
		else
			sprintf(str, "%d KILLS, ", who.killcount);

		disconnectmessage += str;

		// Deaths.
		sprintf(str, "%d DEATHS)", who.deathcount);

		disconnectmessage += str;

		SV_BroadcastPrintf(PRINT_HIGH, "%s\n", disconnectmessage.c_str());
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
    for (size_t i=0; i < players.size(); i++)
    {
		client_t *cl = &players[i].client;

		MSG_WriteMarker(&cl->reliablebuf, svc_disconnect);
		SV_SendPacket(players[i]);

		if(players[i].mo)
			players[i].mo->Destroy();
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
	for (size_t i=0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		MSG_WriteMarker(&cl->reliablebuf, svc_reconnect);
		SV_SendPacket(players[i]);

		if(players[i].mo)
			players[i].mo->Destroy();
	}

	players.clear();
}

//
// SV_ExitLevel
// Called when sv_timelimit or sv_fraglimit hit.
//
void SV_ExitLevel()
{
	for (size_t i=0; i < players.size(); i++)
	{
	   client_t *cl = &clients[i];

	   MSG_WriteMarker(&cl->netbuf, svc_exitlevel);
	}
}

static bool STACK_ARGS compare_player_frags (const player_t *arg1, const player_t *arg2)
{
	return arg2->fragcount < arg1->fragcount;
}

static bool STACK_ARGS compare_player_kills (const player_t *arg1, const player_t *arg2)
{
	return arg2->killcount < arg1->killcount;
}

static bool STACK_ARGS compare_player_points (const player_t *arg1, const player_t *arg2)
{
	return arg2->points < arg1->points;
}

//
// SV_DrawScores
// Draws scoreboard to console. Used during level exit or a command.
//
void SV_DrawScores()
{
    char str[80], str2[80], ip[16];
    std::vector<player_t *> sortedplayers(players.size());
    size_t i, j;

    // Player list sorting
	for (i = 0; i < sortedplayers.size(); i++)
		sortedplayers[i] = &players[i];

    if (sv_gametype == GM_CTF) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_points);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("                    CAPTURE THE FLAG");
        Printf_Bold("-----------------------------------------------------------");        

        if (sv_scorelimit)
            sprintf (str, "Scorelimit: %-6d", (int)sv_scorelimit);
        else
            sprintf (str, "Scorelimit: N/A    ");

        if (sv_timelimit)
            sprintf (str2, "Timelimit: %-7d", (int)sv_timelimit);
        else
            sprintf (str2, "Timelimit: N/A");

        Printf_Bold("%s  %35s", str, str2);

        for (j = 0; j < 2; j++) {
            if (j == 0) {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("--------------------------------------------------BLUE TEAM");
            } else {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("---------------------------------------------------RED TEAM");
            }
            Printf_Bold("ID  Address          Name            Points Caps Frags Time");
            Printf_Bold("-----------------------------------------------------------");

            for (i = 0; i < sortedplayers.size(); i++) {
                if ((unsigned)sortedplayers[i]->userinfo.team == j && !sortedplayers[i]->spectator) {
						
					sprintf(ip,"%u.%u.%u.%u",
						(int)sortedplayers[i]->client.address.ip[0],
						(int)sortedplayers[i]->client.address.ip[1],
						(int)sortedplayers[i]->client.address.ip[2],
						(int)sortedplayers[i]->client.address.ip[3]);
						                	
                    Printf(PRINT_HIGH, "%-3d %-16s %-15s %-6d N/A  %-5d  N/A",
						i+1,
						ip,
                        sortedplayers[i]->userinfo.netname,
                        sortedplayers[i]->points,
                        //sortedplayers[i]->captures,
                        sortedplayers[i]->fragcount);
                        //sortedplayers[i]->GameTime / 60);
                }
            }
        }

    } else if (sv_gametype == GM_TEAMDM) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("                     TEAM DEATHMATCH");
        Printf_Bold("-----------------------------------------------------------");        

        if (sv_fraglimit)
            sprintf (str, "Fraglimit: %-7d", (int)sv_fraglimit);
        else
            sprintf (str, "Fraglimit: N/A    ");

        if (sv_timelimit)
            sprintf (str2, "Timelimit: %-7d", (int)sv_timelimit);
        else
            sprintf (str2, "Timelimit: N/A");

        Printf_Bold("%s  %35s", str, str2);

        for (j = 0; j < 2; j++) {
            if (j == 0) {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("--------------------------------------------------BLUE TEAM");
            } else {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("---------------------------------------------------RED TEAM");
            }
            Printf_Bold("ID  Address          Name            Frags Deaths  K/D Time");
            Printf_Bold("-----------------------------------------------------------");

            for (i = 0; i < sortedplayers.size(); i++) {
                if ((unsigned)sortedplayers[i]->userinfo.team == j && !sortedplayers[i]->spectator) {
                    if (sortedplayers[i]->fragcount <= 0) // Copied from HU_DMScores1.
                        sprintf (str, "0.0");
                    else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0)
                        sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount);
                    else
                        sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);
						
					sprintf(ip,"%u.%u.%u.%u",
						(int)sortedplayers[i]->client.address.ip[0],
						(int)sortedplayers[i]->client.address.ip[1],
						(int)sortedplayers[i]->client.address.ip[2],
						(int)sortedplayers[i]->client.address.ip[3]);
					
					Printf(PRINT_HIGH, "%-3d %-16s %-15s %-5d %-6d %4s  N/A",
						i+1,
						ip,
						sortedplayers[i]->userinfo.netname,
						sortedplayers[i]->fragcount,
						sortedplayers[i]->deathcount,
						str);
						//sortedplayers[i]->GameTime / 60);
                }
            }
        }

		Printf (PRINT_HIGH, "\n");

    } else if (sv_gametype != GM_COOP) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("                        DEATHMATCH");
        Printf_Bold("-----------------------------------------------------------");        

        if (sv_fraglimit)
            sprintf (str, "Fraglimit: %-7d", (int)sv_fraglimit);
        else
            sprintf (str, "Fraglimit: N/A    ");

        if (sv_timelimit)
            sprintf (str2, "Timelimit: %-7d", (int)sv_timelimit);
        else
            sprintf (str2, "Timelimit: N/A   ");

        Printf_Bold("%s  %35s", str, str2);

        Printf_Bold("ID  Address          Name            Frags Deaths  K/D Time");
        Printf_Bold("-----------------------------------------------------------");

        for (i = 0; i < sortedplayers.size(); i++) {
        	if (!sortedplayers[i]->spectator) {
				if (sortedplayers[i]->fragcount <= 0) // Copied from HU_DMScores1.
					sprintf (str, "0.0");
				else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0)
					sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount);
				else
					sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);
					
				sprintf(ip,"%u.%u.%u.%u",
					(int)sortedplayers[i]->client.address.ip[0],
					(int)sortedplayers[i]->client.address.ip[1],
					(int)sortedplayers[i]->client.address.ip[2],
					(int)sortedplayers[i]->client.address.ip[3]);
					
				Printf(PRINT_HIGH, "%-3d %-16s %-15s %-5d %-6d %4s  N/A",
					i+1,
					ip,
					sortedplayers[i]->userinfo.netname,
					sortedplayers[i]->fragcount,
					sortedplayers[i]->deathcount,
					str);
					//sortedplayers[i]->GameTime / 60);
			}
        }

		Printf (PRINT_HIGH, "\n");
    } else {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("                       COOPERATIVE");
        Printf_Bold("-----------------------------------------------------------");        
        Printf_Bold("ID  Address          Name            Kills Deaths  K/D Time");
        Printf_Bold("-----------------------------------------------------------");

        for (i = 0; i < sortedplayers.size(); i++) {
        	if (!sortedplayers[i]->spectator) {
				if (sortedplayers[i]->killcount <= 0) // Copied from HU_DMScores1.
					sprintf (str, "0.0");
				else if (sortedplayers[i]->killcount >= 1 && sortedplayers[i]->deathcount == 0)
					sprintf (str, "%2.1f", (float)sortedplayers[i]->killcount);
				else
					sprintf (str, "%2.1f", (float)sortedplayers[i]->killcount / (float)sortedplayers[i]->deathcount);
					
				sprintf(ip,"%u.%u.%u.%u",
					(int)sortedplayers[i]->client.address.ip[0],
					(int)sortedplayers[i]->client.address.ip[1],
					(int)sortedplayers[i]->client.address.ip[2],
					(int)sortedplayers[i]->client.address.ip[3]);

				Printf(PRINT_HIGH, "%-3d %-16s %-15s %-5d %-6d %4s  N/A",
					i+1,
					ip,
					sortedplayers[i]->userinfo.netname,
					sortedplayers[i]->killcount,
					sortedplayers[i]->deathcount,
					str);
					//sortedplayers[i]->GameTime / 60);
        	}
        }

		Printf (PRINT_HIGH, "\n");
    }

    Printf_Bold("-------------------------------------------------SPECTATORS");
        for (i = 0; i < sortedplayers.size(); i++) {
            if (sortedplayers[i]->spectator) {
					
				sprintf(ip,"%u.%u.%u.%u",
					(int)sortedplayers[i]->client.address.ip[0],
					(int)sortedplayers[i]->client.address.ip[1],
					(int)sortedplayers[i]->client.address.ip[2],
					(int)sortedplayers[i]->client.address.ip[3]);
					
				Printf(PRINT_HIGH, "%-3d %-16s %-15s\n", i+1,ip,sortedplayers[i]->userinfo.netname);
            }
        }

    Printf (PRINT_HIGH, "\n");
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
void STACK_ARGS SV_BroadcastPrintf (int level, const char *fmt, ...)
{
    va_list       argptr;
    char          string[2048];
    client_t     *cl;

    va_start (argptr,fmt);
    vsprintf (string, fmt,argptr);
    va_end (argptr);

    Printf (level, "%s", string);  // print to the console

    for (size_t i=0; i < players.size(); i++)
    {
		cl = &clients[i];

		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, level);
		MSG_WriteString (&cl->reliablebuf, string);
    }
}

// GhostlyDeath -- same as above but ONLY for spectators
void STACK_ARGS SV_SpectatorPrintf (int level, const char *fmt, ...)
{
    va_list       argptr;
    char          string[2048];
    client_t     *cl;

    va_start (argptr,fmt);
    vsprintf (string, fmt,argptr);
    va_end (argptr);

    Printf (level, "%s", string);  // print to the console

    for (size_t i=0; i < players.size(); i++)
    {
		cl = &clients[i];

		if (players[i].spectator)
		{
			MSG_WriteMarker (&cl->reliablebuf, svc_print);
			MSG_WriteByte (&cl->reliablebuf, level);
			MSG_WriteString (&cl->reliablebuf, string);
		}
    }
}

void STACK_ARGS SV_TeamPrintf (int level, int who, const char *fmt, ...)
{
    va_list       argptr;
    char          string[2048];
    client_t     *cl;

    va_start (argptr,fmt);
    vsprintf (string, fmt,argptr);
    va_end (argptr);

    Printf (level, "%s", string);  // print to the console

    for (size_t i=0; i < players.size(); i++)
    {
		if(!(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) || players[i].userinfo.team != idplayer(who).userinfo.team)
			continue;

		if (players[i].spectator)
			continue;

		cl = &clients[i];

		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, level);
		MSG_WriteString (&cl->reliablebuf, string);
    }
}

//
// SV_Say
// Show a chat string and send it to others clients.
//
void SV_Say(player_t &player)
{
	byte team = MSG_ReadByte();
    const char *s = MSG_ReadString();

	if(!strlen(s) || strlen(s) > 128)
		return;

    // Flood protection
    if (player.LastMessage.Time)
    {
#if defined(_MSC_VER) && _MSC_VER <= 1200
		// GhostlyDeath <October 28, 2008> -- VC6 can't cast unsigned __int64 to a double!
		signed __int64 Difference = (I_GetTime() - player.LastMessage.Time);
#else
        QWORD Difference = (I_GetTime() - player.LastMessage.Time);
#endif
        float Delay = (float)(sv_flooddelay * TICRATE);

        if (Difference <= Delay)
            return;

        player.LastMessage.Time = 0;
    }

    if (!player.LastMessage.Time)
    {
        player.LastMessage.Time = I_GetTime();
        player.LastMessage.Message = s;
    }

	if (strnicmp(s, "/me ", 4) == 0)
	{
		if (player.spectator && (!sv_globalspectatorchat || team))
			SV_SpectatorPrintf(PRINT_CHAT, "<SPECTATORS> * %s %s\n", player.userinfo.netname, &s[4]);
		else if(!team)
			SV_BroadcastPrintf (PRINT_CHAT, "* %s %s\n", player.userinfo.netname, &s[4]);
		else if(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			SV_TeamPrintf(PRINT_CHAT, player.id, "<TEAM> * %s %s\n", player.userinfo.netname, &s[4]);
	}
	else
	{
		if (player.spectator && (!sv_globalspectatorchat || team))
			SV_SpectatorPrintf (PRINT_CHAT, "<%s to SPECTATORS> %s\n", player.userinfo.netname, s);
		else if(!team)
			SV_BroadcastPrintf (PRINT_CHAT, "%s: %s\n", player.userinfo.netname, s);
		else if(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			SV_TeamPrintf (PRINT_TEAMCHAT, player.id, "<%s to TEAM> %s\n", player.userinfo.netname, s);
	}
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
		if (((gametic+mo->netid) % 30) && mo->type != MT_TRACER)
			continue;
        // this is a hack for revenant tracers, so they get updated frequently
        // in coop, this will need to be changed later for a more "smoother"
        // tracer
        else if (((gametic+mo->netid) % 10) && mo->type == MT_TRACER)
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

			MSG_WriteMarker (&cl->netbuf, svc_actor_movedir);
			MSG_WriteShort(&cl->netbuf, mo->netid);
			MSG_WriteByte (&cl->netbuf, mo->movedir);
			MSG_WriteLong (&cl->netbuf, mo->movecount);


            if (mo->target)
            {
                MSG_WriteMarker (&cl->netbuf, svc_actor_target);
                MSG_WriteShort(&cl->netbuf, mo->netid);
                MSG_WriteShort (&cl->netbuf, mo->target->netid);
            }
            
            if (mo->tracer)
            {
                MSG_WriteMarker (&cl->netbuf, svc_actor_tracer);
                MSG_WriteShort(&cl->netbuf, mo->netid);
                MSG_WriteShort (&cl->netbuf, mo->tracer->netid);
            }
            
			if ((mobjinfo[mo->type].spawnstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].seestate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].painstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].meleestate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].missilestate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].deathstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].xdeathstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].raisestate == (statenum_t)(mo->state - states)))
            {
                MSG_WriteMarker (&cl->netbuf, svc_mobjstate);
                MSG_WriteShort (&cl->netbuf, mo->netid);
                MSG_WriteShort (&cl->netbuf, (mo->state - states));
            }

            if (cl->netbuf.cursize >= 1024)
                if(!SV_SendPacket(pl))
                    return;
		}
    }
}

//
// SV_UpdateMonsters
// Updates monster position/angle sometimes.
//
void SV_UpdateMonsters(player_t &pl)
{
    AActor *mo;

    TThinkerIterator<AActor> iterator;
    while ( (mo = iterator.Next() ) )
    {
        if (mo->flags & MF_CORPSE)
			continue;
        if (!(mo->flags & MF_COUNTKILL || mo->type == MT_SKULL))
			continue;

		// update monster position every 10 tics
		if ((gametic+mo->netid) % 10)
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

			MSG_WriteMarker (&cl->netbuf, svc_actor_movedir);
			MSG_WriteShort (&cl->netbuf, mo->netid);
			MSG_WriteByte (&cl->netbuf, mo->movedir);
			MSG_WriteLong (&cl->netbuf, mo->movecount);

            if (mo->target)
            {
                MSG_WriteMarker (&cl->netbuf, svc_actor_target);
                MSG_WriteShort(&cl->netbuf, mo->netid);
                MSG_WriteShort (&cl->netbuf, mo->target->netid);
            }

			if ((mobjinfo[mo->type].spawnstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].seestate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].painstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].meleestate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].missilestate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].deathstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].xdeathstate == (statenum_t)(mo->state - states)) ||
                (mobjinfo[mo->type].raisestate == (statenum_t)(mo->state - states)))
            {
                MSG_WriteMarker (&cl->netbuf, svc_mobjstate);
                MSG_WriteShort (&cl->netbuf, mo->netid);
                MSG_WriteShort (&cl->netbuf, (mo->state - states));
            }

            if (cl->netbuf.cursize >= 1024)
                if(!SV_SendPacket(pl))
                    return;
		}
    }
}

//
// SV_ActorTarget
//
void SV_ActorTarget(AActor *actor)
{
	for (size_t i=0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		client_t *cl = &clients[i];

		if(!SV_IsPlayerAllowedToSee(players[i], actor))
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
	for (size_t i=0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		client_t *cl = &clients[i];

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

	if (gametic%35)
	{
		return;
	}
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
// SV_SendGametic
// Sends gametic to calculate ping
//
void SV_SendGametic(client_t* cl)
{
	if ((gametic%100) != 0)
		return;

	MSG_WriteMarker (&cl->reliablebuf, svc_svgametic);
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
	if ((gametic%101) != 0)
		return;

	for (size_t j=0; j < players.size(); j++)
	{
		if (!players[j].ingame())
			continue;

	    MSG_WriteMarker(&cl->reliablebuf, svc_updateping);
	    MSG_WriteByte(&cl->reliablebuf, players[j].id);  // player
	    MSG_WriteLong(&cl->reliablebuf, players[j].ping);
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
	if (gametic % TICRATE)
		return;

	for (size_t i = 0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		cl->reliable_bps = 0;
		cl->unreliable_bps = 0;
	}
}

//
// SV_SendPackets
//
void SV_SendPackets(void)
{
	// send packets, rotating the send order
	// so that players[0] does not always get an advantage
	{
		static size_t fair_send = 0;
		size_t num_players = players.size();

		for (size_t i = 0; i < num_players; i++)
		{
			SV_SendPacket(players[(i+fair_send)%num_players]);
		}

		if(++fair_send >= num_players)
			fair_send = 0;
	}
}

//
// SV_WriteCommands
//
void SV_WriteCommands(void)
{
	size_t		i, j;
	client_t	*cl;

	for (i=0; i < players.size(); i++)
	{
		cl = &clients[i];

		// Don't need to update origin every tic.
		// The server sends origin and velocity of a
		// player and the client always knows origin on
		// on the next tic.
		if (gametic % 2)
			for (j=0; j < players.size(); j++)
			{
				if (!players[j].ingame() || !players[j].mo)
					continue;

				if (j == i)
					continue;

				// GhostlyDeath -- Screw spectators
				if (players[j].spectator)
					continue;

				if(!SV_IsPlayerAllowedToSee(players[i], players[j].mo))
					continue;

				MSG_WriteMarker(&cl->netbuf, svc_moveplayer);
				MSG_WriteByte(&cl->netbuf, players[j].id);     // player number
				MSG_WriteLong(&cl->netbuf, cl->lastclientcmdtic);
				MSG_WriteLong(&cl->netbuf, players[j].mo->x);
				MSG_WriteLong(&cl->netbuf, players[j].mo->y);
				MSG_WriteLong(&cl->netbuf, players[j].mo->z);
				MSG_WriteLong(&cl->netbuf, players[j].mo->angle);
				if (players[j].mo->frame == 32773)
					MSG_WriteByte(&cl->netbuf, PLAYER_FULLBRIGHTFRAME);
				else
					MSG_WriteByte(&cl->netbuf, players[j].mo->frame);

				// write velocity
				MSG_WriteLong(&cl->netbuf, players[j].mo->momx);
				MSG_WriteLong(&cl->netbuf, players[j].mo->momy);
				MSG_WriteLong(&cl->netbuf, players[j].mo->momz);
				
				// [Russell] - hack, tell the client about the partial
				// invisibility power of another player.. (cheaters can disable
                // this but its all we have for now)
                MSG_WriteLong(&cl->netbuf, players[j].powers[pw_invisibility]);
			}

		SV_UpdateHiddenMobj();

		SV_UpdateConsolePlayer(players[i]);

		SV_UpdateMissiles(players[i]);

		SV_UpdateMonsters(players[i]);

	 	SV_SendGametic(cl); // to calculate a ping value, when
		                    // a client returns it.
		SV_UpdatePing(cl);
	}

	SV_UpdateDeadPlayers(); // Update dying players.
}

void SV_PlayerTriedToCheat(player_t &player)
{
	SV_BroadcastPrintf(PRINT_HIGH, "%s tried to cheat!\n", player.userinfo.netname);
	SV_DropClient(player);
}

void SV_GetPlayerCmd(player_t &player)
{
	client_t *cl = &player.client;
	ticcmd_t *cmd = &player.cmd;
	int tic = MSG_ReadLong(); // get client gametic

	// denis - todo - (out of order packet)
	if(!player.mo || cl->lastclientcmdtic > tic)
	{
		MSG_ReadLong();
		MSG_ReadLong();
		MSG_ReadShort();
		MSG_ReadLong();
		MSG_ReadLong();
		MSG_ReadShort();
		return;
	}

	// get the previous cmds and angle
	if (  cl->lastclientcmdtic && (tic - cl->lastclientcmdtic >= 2)
		     && gamestate != GS_INTERMISSION)
	{
		cmd->ucmd.buttons = MSG_ReadByte();

		if(player.playerstate != PST_DEAD)
		{
			player.mo->angle = MSG_ReadShort() << 16;

			if (!sv_freelook)
			{
				player.mo->pitch = 0;
				MSG_ReadShort();
			}
			else
				player.mo->pitch = MSG_ReadShort() << 16;
		}
		else
			MSG_ReadLong();

		cmd->ucmd.forwardmove = MSG_ReadShort();
		cmd->ucmd.sidemove = MSG_ReadShort();

		if ( abs(cmd->ucmd.forwardmove) > 12800
			|| abs(cmd->ucmd.sidemove) > 12800)
		{
			SV_PlayerTriedToCheat(player);
			return;
		}

		cmd->ucmd.impulse = MSG_ReadByte();

		if(!sv_speedhackfix && gamestate == GS_LEVEL)
		{
			P_PlayerThink(&player);
			player.mo->RunThink();
		}
	}
	else
	{
		// get 10 bytes
		MSG_ReadLong();
		MSG_ReadLong();
		MSG_ReadShort();
	}

	// get current cmds and angle
	cmd->ucmd.buttons = MSG_ReadByte();
	if (gamestate != GS_INTERMISSION && player.playerstate != PST_DEAD)
	{
		if(step_mode)cmd->ucmd.yaw = MSG_ReadShort();
		else player.mo->angle = MSG_ReadShort() << 16;

		if (!sv_freelook)
		{
			player.mo->pitch = 0;
			MSG_ReadShort();
		}
		else
			player.mo->pitch = MSG_ReadShort() << 16;
	}
	else
		MSG_ReadLong();

	cmd->ucmd.forwardmove = MSG_ReadShort();
	cmd->ucmd.sidemove = MSG_ReadShort();

	if ( abs(cmd->ucmd.forwardmove) > 12800
		|| abs(cmd->ucmd.sidemove) > 12800)
	{
		SV_PlayerTriedToCheat(player);
		return;
	}

	cmd->ucmd.impulse = MSG_ReadByte();

	if(!sv_speedhackfix && gamestate == GS_LEVEL)
	{
		P_PlayerThink(&player);
		player.mo->RunThink();
	}

	cl->lastclientcmdtic = tic;
	cl->lastcmdtic = gametic;
}


void SV_UpdateConsolePlayer(player_t &player)
{
	// GhostlyDeath -- Spectators are on their own really
	if (player.spectator)
		return;

	// It's not a good idea to send 33 bytes every tic.
	if (gametic % 3)
		return;

	AActor *mo = player.mo;

	if(!mo)
		return;

	client_t *cl = &player.client;

	// client player will update his position if packets were missed
	MSG_WriteMarker (&cl->netbuf, svc_updatelocalplayer);
	MSG_WriteLong (&cl->netbuf, cl->lastclientcmdtic);

	MSG_WriteLong (&cl->netbuf, mo->x);
	MSG_WriteLong (&cl->netbuf, mo->y);
	MSG_WriteLong (&cl->netbuf, mo->z);

	MSG_WriteLong (&cl->netbuf, mo->momx);
	MSG_WriteLong (&cl->netbuf, mo->momy);
	MSG_WriteLong (&cl->netbuf, mo->momz);

    MSG_WriteByte (&cl->netbuf, mo->waterlevel);

	SV_UpdateMovingSectors(cl); // denis - fixme - todo - only info about the sector player is standing on info should be sent. note that this is not player->mo->subsector->sector

//	MSG_WriteShort (&cl->netbuf, mo->momx >> FRACBITS);
//	MSG_WriteShort (&cl->netbuf, mo->momy >> FRACBITS);
//	MSG_WriteShort (&cl->netbuf, mo->momz >> FRACBITS);
}

//
//	SV_ChangeTeam
//																							[Toke - CTF]
//	Allows players to change teams properly in teamplay and CTF
//
void SV_ChangeTeam (player_t &player)  // [Toke - Teams]
{
	team_t team = (team_t)MSG_ReadByte();

	if(team >= NUMTEAMS)
		return;

	if(sv_gametype == GM_CTF && team >= 2)
		return;

	if(sv_gametype != GM_CTF && team >= sv_teamsinplay)
		return;

	team_t old_team = player.userinfo.team;
	player.userinfo.team = team;

	SV_BroadcastPrintf (PRINT_HIGH, "%s has joined the %s team.\n", team_names[team]);

	switch (player.userinfo.team)
	{
		case TEAM_BLUE:
			player.userinfo.skin = R_FindSkin ("BlueTeam");
			break;

		case TEAM_RED:
			player.userinfo.skin = R_FindSkin ("RedTeam");
			break;

		default:
			break;
	}

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
		if (player.mo && player.userinfo.team != old_team)
			P_DamageMobj (player.mo, 0, 0, 1000, 0);
}

//
// SV_Spectate
//
void SV_Spectate (player_t &player)
{
	byte Code = MSG_ReadByte();

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
	else if (!(BOOL)Code) {
		if (gamestate == GS_INTERMISSION)
			return;

		if (player.spectator){
			int NumPlayers = 0;
			// Check to see if there are enough "activeplayers"
			for (size_t i = 0; i < players.size(); i++)
			{
				if (!players[i].spectator && players[i].playerstate != PST_CONTACT && players[i].playerstate != PST_DOWNLOAD)
					NumPlayers++;
			}

			if (NumPlayers < sv_maxplayers)
			{
				if ((multiplayer && level.time > player.joinafterspectatortime + TICRATE*3) ||
					level.time > player.joinafterspectatortime + TICRATE*5) {
					player.spectator = false;
					for (size_t j = 0; j < players.size(); j++) {
						MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
						MSG_WriteByte (&(players[j].client.reliablebuf), player.id);
						MSG_WriteByte (&(players[j].client.reliablebuf), false);
					}
					P_KillMobj(NULL, player.mo, NULL, true);
					player.playerstate = PST_REBORN;
					if (sv_gametype != GM_TEAMDM && sv_gametype != GM_CTF)
						SV_BroadcastPrintf (PRINT_HIGH, "%s joined the game.\n", player.userinfo.netname);
					else
						SV_BroadcastPrintf (PRINT_HIGH, "%s joined the game on the %s team.\n",
							player.userinfo.netname, team_names[player.userinfo.team]);
					// GhostlyDeath -- Reset Frags, Deaths and Kills
					player.fragcount = 0;
					player.deathcount = 0;
					player.killcount = 0;
					SV_UpdateFrags(player);
				}
			}
		}
	} else if (gamestate != GS_INTERMISSION) {
		if (!player.spectator) {
			for (size_t j = 0; j < players.size(); j++) {
				MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
				MSG_WriteByte (&(players[j].client.reliablebuf), player.id);
				MSG_WriteByte (&(players[j].client.reliablebuf), true);
			}
			player.spectator = true;
			player.playerstate = PST_LIVE;
			player.joinafterspectatortime = level.time;

			if (sv_gametype == GM_CTF)
				CTF_CheckFlags (player);

			SV_BroadcastPrintf(PRINT_HIGH, "%s became a spectator.\n", player.userinfo.netname);
		}
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

	if (!password.empty() && MD5SUM(password + cl->digest) == challenge)
	{
		cl->allow_rcon = true;
		Printf(PRINT_HIGH, "rcon login from %s", NET_AdrToString(cl->address));
	}
	else
	{
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
	// merry suicide!
	P_DamageMobj (player.mo, NULL, NULL, 10000, MOD_SUICIDE);
	player.mo->player = NULL;
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
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString (&cl->reliablebuf, "Server: Downloading is disabled\n");
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
	for(i = 0; i < wadnames.size(); i++)
	{
		if(wadnames[i] == request && (wadhashes[i] == md5 || md5 == ""))
			break;
	}

	if(i == wadnames.size())
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString (&cl->reliablebuf, "Server: Bad wad request\n");
		SV_DropClient(player);
		return;
	}

	// denis - do not download commercial wads
	if(W_IsIWAD(wadnames[i], wadhashes[i]))
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString (&cl->reliablebuf, "Server: This is a commercial wad and will not be downloaded\n");

		SV_DropClient(player);
		return;
	}

	if(player.playerstate != PST_DOWNLOAD || cl->download.name != wadfiles[i])
		Printf(PRINT_HIGH, "> client %d is downloading %s\n", player.id, wadnames[i].c_str());

	cl->download.name = wadfiles[i];
	cl->download.next_offset = next_offset;
	player.playerstate = PST_DOWNLOAD;
}

//
// SV_ParseCommands
//
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
			SV_SetupUserInfo(player);
			break;

		case clc_say:
			SV_Say(player);
			break;

		case clc_move:
			SV_GetPlayerCmd(player);
			break;

		case clc_svgametic:
			SV_CalcPing(player);
			break;

		case clc_rate:
			player.client.rate = MSG_ReadLong();
			// denis - prevent problems by locking rate within a range
            if(player.client.rate < 500)player.client.rate = 500;
			if(player.client.rate > sv_maxrate)player.client.rate = sv_maxrate;
			break;

		case clc_ack:
			SV_AcknowledgePacket(player);
			break;

		case clc_rcon:
			{
				const char *str = MSG_ReadString();

				if (player.client.allow_rcon)
					AddCommandString (str);
			}
			break;

		case clc_rcon_password:
			SV_RConPassword(player);
			break;

		case clc_changeteam:
			SV_ChangeTeam(player);
			break;

		case clc_spectate:
            {
                SV_Spectate (player);
            }
			break;

		case clc_kill:
			if(player.mo &&
               level.time > player.respawn_time + TICRATE*10 &&
               sv_allowcheats)
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

	// don't send too much
	// [ML] 8/27/10: ...unless the server admin allows it
	if(gametic%(2 * (players.size()+1)) && sv_waddownloadcap)
        return;

	// wad downloading
	for(size_t i = 0; i < players.size(); i++)
	{
		if(players[i].playerstate != PST_DOWNLOAD)
			continue;

		client_t *cl = &clients[i];

		if(!cl->download.name.length())
			continue;

		// read next bit of wad
		static char buff[1024];

		unsigned int filelen = 0;
		unsigned int read;

		read = W_ReadChunk(cl->download.name.c_str(), cl->download.next_offset, sizeof(buff), buff, filelen);

        // [Russell] - We take into account the maximum packet limit so we don't
        // go over it and end up with a mangled package, the numeric values here
        // represent the size of each written data type that follows
        if ((cl->netbuf.cursize + 1 + 4 + 1 + 4 + 2 + read) >= 8192)
            SV_SendPacket(players[i]);

		if(read)
		{
			if(!cl->download.next_offset)
			{
				MSG_WriteMarker (&cl->netbuf, svc_wadinfo);
				MSG_WriteLong (&cl->netbuf, filelen);
			}

			MSG_WriteMarker (&cl->netbuf, svc_wadchunk);
			MSG_WriteLong (&cl->netbuf, cl->download.next_offset);
			MSG_WriteShort (&cl->netbuf, read);
			MSG_WriteChunk (&cl->netbuf, buff, read);

			cl->download.next_offset += read;
		}
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
	if(!sv_timelimit || level.time < (int)(sv_timelimit * TICRATE * 60) || shotclock || gamestate == GS_INTERMISSION)
		return;

	// LEVEL TIMER
	if (players.size()) {
		if (sv_gametype == GM_DM) {
			player_t *winplayer = &players[0];
			bool drawgame = false;

			if (players.size() > 1) {
				for (size_t i = 1; i < players.size(); i++) {
					if (players[i].fragcount > winplayer->fragcount) {
						drawgame = false;
						winplayer = &players[i];
					} else if (players[i].fragcount == winplayer->fragcount)
						drawgame = true;
				}
			}

			if (drawgame)
				SV_BroadcastPrintf (PRINT_HIGH, "Time limit hit. Game is a draw!\n");
			else
				SV_BroadcastPrintf (PRINT_HIGH, "Time limit hit. Game won by %s!\n", winplayer->userinfo.netname);
		} else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
			team_t winteam = SV_WinningTeam ();

			if(winteam == TEAM_NONE)
				SV_BroadcastPrintf(PRINT_HIGH, "Time limit hit. Game is a draw!\n");
			else
				SV_BroadcastPrintf (PRINT_HIGH, "Time limit hit. %s team wins!\n", team_names[winteam]);
		}
	}

	shotclock = TICRATE*2;
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

	if(gamestate == GS_LEVEL)
	{
		SV_RemoveCorpses();
		SV_WinCheck();
		SV_TimelimitCheck();
	}

	SV_WadDownloads();
	SV_UpdateMaster();
}

//
// SV_SetMoveableSectors
//
void SV_SetMoveableSectors()
{
	// [csDoom] if the floor or the ceiling of a sector is moving,
	// mark it as moveable
	for (int i=0; i<numsectors; i++)
	{
		sector_t* sec = &sectors[i];

		if ((sec->ceilingdata && sec->ceilingdata->IsKindOf (RUNTIME_CLASS(DMover)))
		|| (sec->floordata && sec->floordata->IsKindOf (RUNTIME_CLASS(DMover))))
			sec->moveable = true;
	}
}

void SV_TouchSpecial(AActor *special, player_t *player)
{
    client_t *cl = &player->client;
    
    if (cl == NULL || special == NULL)
        return;
        
    MSG_WriteMarker(&cl->reliablebuf, svc_touchspecial);
    MSG_WriteShort(&cl->reliablebuf, special->netid);
}

//
// SV_StepTics
//
void SV_StepTics (QWORD tics)
{
	DObject::BeginFrame ();

	// run the newtime tics
	while (tics--)
	{
		SV_SetMoveableSectors();
		C_Ticker ();

		SV_GameTics ();

		G_Ticker ();

		SV_WriteCommands();
		SV_SendPackets();
		SV_ClearClientsBPS();
		SV_CheckTimeouts();

		gametic++;
	}

	DObject::EndFrame ();
}

//
// SV_RunTics
//
void SV_RunTics (void)
{
	QWORD nowtime = I_GetTime ();
	QWORD newtics = nowtime - gametime;

	SV_GetPackets();

	std::string cmd = I_ConsoleInput();
	if (cmd.length())
	{
		AddCommandString (cmd.c_str());
	}

	if(CON.is_open())
	{
		CON.clear();
		if(!CON.eof())
		{
			std::getline(CON, cmd);
			AddCommandString (cmd.c_str());
		}
	}

	if(newtics > 0 && !step_mode)
	{
		SV_StepTics(newtics);
		gametime = nowtime;
	}

	// wait until a network message arrives or next tick starts
	if(nowtime == I_GetTime())
		NetWaitOrTimeout((I_MSTime()%TICRATE)+1);
}

BEGIN_COMMAND(step)
{
        QWORD newtics = argc > 1 ? atoi(argv[1]) : 1;

	extern unsigned char prndindex;

	SV_StepTics(newtics);

	// debugging output
	if(players.size() && players[0].mo)
		Printf(PRINT_HIGH, "level.time %d, prndindex %d, %d %d %d\n", level.time, prndindex, players[0].mo->x, players[0].mo->y, players[0].mo->z);
	else
		Printf(PRINT_HIGH, "level.time %d, prndindex %d\n", level.time, prndindex);
}
END_COMMAND(step)


//	For Debugging
BEGIN_COMMAND (playerinfo)
{
	player_t *player = &consoleplayer();
	char ip[16];

	if(argc > 1)
	{
		size_t who = atoi(argv[1]);
		player_t &p = idplayer(who);

		if(!validplayer(p) || !p.ingame())
		{
			Printf (PRINT_HIGH, "Bad player number\n");
			return;
		}
		else
			player = &p;
	}

	if(!validplayer(*player))
	{
		Printf (PRINT_HIGH, "Not a valid player\n");
		return;
	}
	
	sprintf(ip,"%u.%u.%u.%u",
			(int)player->client.address.ip[0],
			(int)player->client.address.ip[1],
			(int)player->client.address.ip[2],
			(int)player->client.address.ip[3]);

	Printf (PRINT_HIGH, "---------------[player info]----------- \n");
	Printf (PRINT_HIGH, " IP Address       - %s \n",		  ip);
	Printf (PRINT_HIGH, " userinfo.netname - %s \n",		  player->userinfo.netname);
	Printf (PRINT_HIGH, " userinfo.team    - %d \n",		  player->userinfo.team);
	Printf (PRINT_HIGH, " userinfo.aimdist - %d \n",		  player->userinfo.aimdist);
	Printf (PRINT_HIGH, " userinfo.color   - %d \n",		  player->userinfo.color);
	Printf (PRINT_HIGH, " userinfo.skin    - %s \n",		  skins[player->userinfo.skin].name);
	Printf (PRINT_HIGH, " userinfo.gender  - %d \n",		  player->userinfo.gender);
//	Printf (PRINT_HIGH, " time             - %d \n",		  player->GameTime);
	Printf (PRINT_HIGH, "--------------------------------------- \n");
}
END_COMMAND (playerinfo)

BEGIN_COMMAND (playerlist)
{
	bool anybody = false;

	for(int i = players.size()-1; i >= 0 ; i--)
	{
		Printf(PRINT_HIGH, "(%02d): %s - %s - frags:%d ping:%d\n", players[i].id, players[i].userinfo.netname, NET_AdrToString(clients[i].address), players[i].fragcount, players[i].ping);
		anybody = true;
	}

	if(!anybody)
	{
		Printf(PRINT_HIGH, "There are no players on the server\n");
		return;
	}
}
END_COMMAND (playerlist)

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

	for (size_t i=0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

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

	for (size_t i = 0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		client_t *cl = &clients[i];

		MSG_WriteMarker (&cl->reliablebuf, svc_activateline);
		MSG_WriteLong (&cl->reliablebuf, l);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);
		MSG_WriteByte (&cl->reliablebuf, side);
		MSG_WriteByte (&cl->reliablebuf, activationType);
	}
}

//
// MSG_WriteMarker
//
// denis - use this function to mark the start of your server message
// as it allows for better debugging and optimization of network code
//
// Spleen - moved to sv_main.cpp to allow SV_SendPackets call

void MSG_WriteMarker (buf_t *b, svc_t c)
{
    //[Spleen] final check to prevent huge packets from being sent to players
    if (b->cursize > 600)
        SV_SendPackets();
    
	b->WriteByte((byte)c);
}

// [RH]
// ClientObituary: Show a message when a player dies
//
void ClientObituary (AActor *self, AActor *inflictor, AActor *attacker)
{
	const char *message;
	char gendermessage[1024];
	int  gender;

	if (!self->player)
		return;

	gender = self->player->userinfo.gender;

	// Treat voodoo dolls as unknown deaths
	if (inflictor && inflictor->player == self->player)
		MeansOfDeath = MOD_UNKNOWN;

	message = NULL;

	switch (MeansOfDeath) {
		case MOD_SUICIDE:
			message = OB_SUICIDE;
			break;
		case MOD_FALLING:
			message = OB_FALLING;
			break;
		case MOD_CRUSH:
			message = OB_CRUSH;
			break;
		case MOD_EXIT:
			message = OB_EXIT;
			break;
		case MOD_WATER:
			message = OB_WATER;
			break;
		case MOD_SLIME:
			message = OB_SLIME;
			break;
		case MOD_LAVA:
			message = OB_LAVA;
			break;
		case MOD_BARREL:
			message = OB_BARREL;
			break;
		case MOD_SPLASH:
			message = OB_SPLASH;
			break;
	}

	if (attacker && !message) {
		if (attacker == self) {
			switch (MeansOfDeath) {
				case MOD_R_SPLASH:
					message = OB_R_SPLASH;
					break;
				case MOD_ROCKET:
					message = OB_ROCKET;
					break;
				default:
					message = OB_KILLEDSELF;
					break;
			}
		} else if (!attacker->player) {
					if (MeansOfDeath == MOD_HIT) {
						switch (attacker->type) {
							case MT_UNDEAD:
								message = OB_UNDEADHIT;
								break;
							case MT_TROOP:
								message = OB_IMPHIT;
								break;
							case MT_HEAD:
								message = OB_CACOHIT;
								break;
							case MT_SERGEANT:
								message = OB_DEMONHIT;
								break;
							case MT_SHADOWS:
								message = OB_SPECTREHIT;
								break;
							case MT_BRUISER:
								message = OB_BARONHIT;
								break;
							case MT_KNIGHT:
								message = OB_KNIGHTHIT;
								break;
							default:
								break;
						}
					} else {
						switch (attacker->type) {
							case MT_POSSESSED:
								message = OB_ZOMBIE;
								break;
							case MT_SHOTGUY:
								message = OB_SHOTGUY;
								break;
							case MT_VILE:
								message = OB_VILE;
								break;
							case MT_UNDEAD:
								message = OB_UNDEAD;
								break;
							case MT_FATSO:
								message = OB_FATSO;
								break;
							case MT_CHAINGUY:
								message = OB_CHAINGUY;
								break;
							case MT_SKULL:
								message = OB_SKULL;
								break;
							case MT_TROOP:
								message = OB_IMP;
								break;
							case MT_HEAD:
								message = OB_CACO;
								break;
							case MT_BRUISER:
								message = OB_BARON;
								break;
							case MT_KNIGHT:
								message = OB_KNIGHT;
								break;
							case MT_SPIDER:
								message = OB_SPIDER;
								break;
							case MT_BABY:
								message = OB_BABY;
								break;
							case MT_CYBORG:
								message = OB_CYBORG;
								break;
							case MT_WOLFSS:
								message = OB_WOLFSS;
								break;
							default:
								break;
						}
					}
		}
	}

	if (message) {
		SexMessage (message, gendermessage, gender);
		SV_BroadcastPrintf (PRINT_MEDIUM, "%s %s.\n", self->player->userinfo.netname, gendermessage);
		return;
	}

	if (attacker && attacker->player) {
		if (((sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) && self->player->userinfo.team == attacker->player->userinfo.team) || sv_gametype == GM_COOP) {
			int rnum = P_Random ();

			self = attacker;
			gender = self->player->userinfo.gender;

			if (rnum < 64)
				message = OB_FRIENDLY1;
			else if (rnum < 128)
				message = OB_FRIENDLY2;
			else if (rnum < 192)
				message = OB_FRIENDLY3;
			else
				message = OB_FRIENDLY4;
		} else {
			switch (MeansOfDeath) {
				case MOD_FIST:
					message = OB_MPFIST;
					break;
				case MOD_CHAINSAW:
					message = OB_MPCHAINSAW;
					break;
				case MOD_PISTOL:
					message = OB_MPPISTOL;
					break;
				case MOD_SHOTGUN:
					message = OB_MPSHOTGUN;
					break;
				case MOD_SSHOTGUN:
					message = OB_MPSSHOTGUN;
					break;
				case MOD_CHAINGUN:
					message = OB_MPCHAINGUN;
					break;
				case MOD_ROCKET:
					message = OB_MPROCKET;
					break;
				case MOD_R_SPLASH:
					message = OB_MPR_SPLASH;
					break;
				case MOD_PLASMARIFLE:
					message = OB_MPPLASMARIFLE;
					break;
				case MOD_BFG_BOOM:
					message = OB_MPBFG_BOOM;
					break;
				case MOD_BFG_SPLASH:
					message = OB_MPBFG_SPLASH;
					break;
				case MOD_TELEFRAG:
					message = OB_MPTELEFRAG;
					break;
			}
		}
	}

	if (message) {
		SexMessage (message, gendermessage, gender);

		std::string work = "%s ";
		work += gendermessage;
		work += ".\n";

		SV_BroadcastPrintf (PRINT_MEDIUM, work.c_str(), self->player->userinfo.netname,
							attacker->player->userinfo.netname);
		return;
	}

	SexMessage (OB_DEFAULT, gendermessage, gender);
	SV_BroadcastPrintf (PRINT_MEDIUM, "%s %s.\n", self->player->userinfo.netname, gendermessage);
}
void SV_SendDamagePlayer(player_t *player, int damage)
{
	for (size_t i=0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		MSG_WriteMarker(&cl->reliablebuf, svc_damageplayer);
		MSG_WriteByte(&cl->reliablebuf, player->id);
		MSG_WriteByte(&cl->reliablebuf, player->armorpoints);
		MSG_WriteShort(&cl->reliablebuf, damage);
	}
}

void SV_SendDamageMobj(AActor *target, int pain)
{
	for (size_t i = 0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		MSG_WriteMarker(&cl->reliablebuf, svc_damagemobj);
		MSG_WriteShort(&cl->reliablebuf, target->netid);
		MSG_WriteShort(&cl->reliablebuf, target->health);
		MSG_WriteByte(&cl->reliablebuf, pain);
	}
}

void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor,
				     bool joinkill)
{
	for (size_t i = 0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		if(!SV_IsPlayerAllowedToSee(players[i], target))
			continue;

		// send death location first
		MSG_WriteMarker(&cl->netbuf, svc_movemobj);
		MSG_WriteShort(&cl->netbuf, target->netid);
		MSG_WriteByte(&cl->netbuf, target->rndindex);
		MSG_WriteLong(&cl->netbuf, target->x);
		MSG_WriteLong(&cl->netbuf, target->y);
		MSG_WriteLong(&cl->netbuf, target->z);
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
		for (size_t i = 0; i < mo->players_aware.size(); i++)
		{
			client_t *cl = &idplayer(mo->players_aware[i]).client;

            // denis - todo - need a queue for destroyed (lost awareness) 
            // objects, as a flood of destroyed things could easily overflow a 
            // buffer
			MSG_WriteMarker(&cl->reliablebuf, svc_removemobj); 
			MSG_WriteShort(&cl->reliablebuf, mo->netid);
		}
	}

	// AActor no longer active. NetID released.
	if(mo->netid)
		ServerNetID.ReleaseNetID( mo->netid );
}

VERSION_CONTROL (sv_main_cpp, "$Id$")



