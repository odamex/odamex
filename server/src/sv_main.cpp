// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2007 by The Odamex Team.
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
#include "d_player.h"
#include "s_sound.h"
#include "gi.h"
#include "d_net.h"
#include "g_game.h"
#include "p_local.h"
#include "sv_main.h"
#include "sv_master.h"
#include "i_system.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_random.h"
#include "vectors.h"
#include "sv_ctf.h"
#include "w_wad.h"
#include "md5.h"

#include <algorithm>
#include <sstream>

extern void G_DeferedInitNew (char *mapname);
extern level_locals_t level;

// denis - game manipulation, but no fancy gfx
bool clientside = false, serverside = true;
bool predicting = false;

// General server settings
CVAR (hostname,			"Unnamed",	CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)					// A servers name that will apear in the launcher.
CVAR (email,			"",			CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)					// Server admin's e-mail address. - does not work yet
CVAR (usemasters,		"1",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Server apears in the server list when true.
CVAR (waddownload,		"1",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Send wad files to clients who request them.
CVAR (emptyreset,       "0",        CVAR_ARCHIVE | CVAR_SERVERINFO)                 // Reset current map when last player leaves.
CVAR (clientcount,		"0",        CVAR_NOSET | CVAR_NOENABLEDISABLE)										// tracks number of connected players for scripting

BEGIN_CUSTOM_CVAR (maxplayers,		"16",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)	// Describes the max number of clients that are allowed to connect. - does not work yet
{
	if(var > MAXPLAYERS)
		var.Set(MAXPLAYERS);

	if(var < 0)
		var.Set((float)0);

	while(players.size() > maxplayers)
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
	clientcount.ForceSet(players.size());
	//R_InitTranslationTables();
}
END_CUSTOM_CVAR (maxplayers)


// Game settings
CVAR (allowcheats,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)	// Players are allowed to use cheatcodes when try. - does not work yet
CVAR (deathmatch,		"1",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)	// Deathmatch mode when true, this includes teamDM and CTF.
CVAR (fraglimit,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)					// Sets the winning frag total for deathmatch and teamDM.
CVAR (timelimit,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)					// Sets the max time in minutes for each game.

CVAR (maxcorpses, 		"200", 		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
		// joek - max number of corpses. < 0 is infinite

// Map behavior
BEGIN_CUSTOM_CVAR (skill,		"5",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)	// Game skill setting.
{
	if(var < sk_baby)
		var.Set(sk_baby);
	if(var > sk_nightmare)
		var.Set(sk_nightmare);
}
END_CUSTOM_CVAR(skill)

EXTERN_CVAR(weaponstay)
CVAR (itemsrespawn,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)	// Initial items will respawn after being picked up when true.
CVAR (monstersrespawn,	"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Monsters will respawn after killed when true. - does not work yet
CVAR (fastmonsters,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Monsters move and shoot at double speed when true. - does not work yet
CVAR (nomonsters,		"1",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)	// No monsters will be spawned when true
CVAR (cleanmaps,		"",		CVAR_NULL)										// Deprecated

// Action rules
CVAR (allowexit,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Exit switch functions when true.
CVAR (fragexitswitch,   "0",        CVAR_ARCHIVE | CVAR_SERVERINFO)                 // [ML] 03/4/06: When activated, game must be completed by hitting exit switch once fraglimit has been reached
CVAR (allowjump,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Jump command functions when true.
CVAR (allowfreelook,	"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Freelook works when true.
CVAR (infiniteammo,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Players have infinite ammo when true.

// Teamplay/CTF
CVAR (usectf,			"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)	// CTF will automaticly be enabled on maps that contain flags when true.
CVAR (scorelimit,		"10",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)					// Sets the winning flag capture total for CTF games.
CVAR (friendlyfire,		"1",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Players on the same team cannot cause eachother damage in team games when true.
CVAR (teamplay,			"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)	// TeamDM is enabled when true - requires deathmatch being true.
CVAR (blueteam,			"1",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Players are allowed to select team BLUE when true - TeamDM only.
CVAR (redteam,			"1",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Players are allowed to select team RED  when true - TeamDM only.
CVAR (goldteam,			"0",		CVAR_ARCHIVE | CVAR_SERVERINFO)					// Players are allowed to select team GOLD when true - TeamDM only.

//									Private server settings

BEGIN_CUSTOM_CVAR (password,	"",			CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)									// Remote console password.
{
	// denis - todo implement use of this
	if(strlen(var.cstring()))
		Printf(PRINT_HIGH, "join password set");
}
END_CUSTOM_CVAR(password)

BEGIN_CUSTOM_CVAR (spectate_password,	"",			CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)									// Remote console password.
{
	// denis - todo implement use of this
	if(strlen(var.cstring()))
		Printf(PRINT_HIGH, "spectate password set");
}
END_CUSTOM_CVAR(spectate_password)

BEGIN_CUSTOM_CVAR (rcon_password,	"",			CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)									// Remote console password.
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
END_CUSTOM_CVAR(rcon_password)

CVAR (antiwallhack,		"0",		CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)		// Enable/disable anti wallhack check, temporary
EXTERN_CVAR (speedhackfix)

client_c clients;


#define CLIENT_TIMEOUT 65 // 65 seconds

// [NightFang] - Different welcome strings
char welcomestring[] = "Odamex Deathmatch Server\n";
char welcomestring_teamplay[] = "Odamex Team Deathmatch Server\n";

QWORD gametime;

void SV_UpdateConsolePlayer(player_t &player);

void SV_CheckTeam (player_t & playernum);
team_t SV_GoodTeam (void);
void SV_ForceSetTeam (player_t &who, int team);

void SV_SendServerSettings (client_t *cl);
void SV_ServerSettingChange (void);

// some doom functions
void P_KillMobj (AActor *source, AActor *target, AActor *inflictor);
bool P_CheckSightEdges (const AActor* t1, const AActor* t2, float radius_boost = 0.0);

void SV_WinCheck (void);

// denis - kick player
BEGIN_COMMAND (kick)
{
	if (argc < 2)
		return;

	player_t &player = idplayer(atoi(argv[1]));

	if(!validplayer(player))
	{
		Printf(PRINT_HIGH, "bad client number: %d\n", player.id);
		return;
	}

	if(!player.ingame())
	{
		Printf(PRINT_HIGH, "client %d not in game\n", player.id);
		return;
	}

	SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", player.userinfo.netname);
	if (argc > 2)
	{
		client_t  *cl = &player.client;
		std::string reason = BuildString(argc - 2, (const char **)(argv + 2));
		MSG_WriteMarker (&cl->reliablebuf, svc_print);
		MSG_WriteByte (&cl->reliablebuf, PRINT_HIGH);
		MSG_WriteString (&cl->reliablebuf, reason.c_str());
	}
	SV_DropClient(player);
}
END_COMMAND (kick)

// denis - list connected clients
BEGIN_COMMAND (who)
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
END_COMMAND (who)

BEGIN_COMMAND (say)
{
	if (argc > 1)
	{
		std::string chat = BuildString (argc - 1, (const char **)(argv + 1));
		SV_BroadcastPrintf (PRINT_CHAT, "[console]: %s\n", chat.c_str());
	}

}
END_COMMAND (say)

BEGIN_COMMAND (ctf)
{
	if (ctfmode)	Printf (PRINT_HIGH, "Yes.\n");
	else			Printf (PRINT_HIGH, "No.\n");
}
END_COMMAND (ctf)


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
		maxplayers.Set(w); // denis - todo
	}

	gametime = I_GetTime ();
}


int SV_GetFreeClient(void)
{
	if(players.size() >= maxplayers)
		return -1;

	players.push_back(player_t());

	// update tracking cvar
	clientcount.ForceSet(players.size());

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
			player.client.last_received = gametic;
			SV_ParseCommands(player);
		}
	}

	size_t i;

	// remove disconnected players
	for(i = 0; i < players.size(); i++)
	{
		if(players[i].playerstate == PST_DISCONNECT)
		{
			i = players.erase(players.begin() + i) - players.begin();

			// update tracking cvar
			clientcount.ForceSet(players.size());
		}
	}

	// repair mo after player pointers are reset
	for(i = 0; i < players.size(); i++)
	{
		if(players[i].mo)
			players[i].mo->player = &players[i];
	}
}

//
// SV_Sound
//
void SV_Sound (AActor *mo, byte channel, char *name, byte attenuation)
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

		int x, y;
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


void SV_Sound (player_t &pl, AActor *mo, byte channel, char *name, byte attenuation)
{
	int sfx_id;

	sfx_id = S_FindSound (name);

	if (sfx_id > 255 || sfx_id < 0)
	{
		Printf (PRINT_HIGH, "SV_StartSound: range error. Sfx_id = %d\n", sfx_id);
		return;
	}

	int x, y;
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
void UV_SoundAvoidPlayer (player_t &pl, AActor *mo, byte channel, char *name, byte attenuation)
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
		if(&pl == &players[i])
			continue;

        cl = &clients[i];

		int x, y;
		byte vol = 0;

		if(mo)
		{
			x = mo->x;
			y = mo->y;

			vol = SV_PlayerHearingLoss(players[i], x, y);
		}

		MSG_WriteMarker (&cl->netbuf, svc_startsound);
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
void SV_SoundTeam (byte channel, char* name, byte attenuation, int team)
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
				MSG_WriteShort (&cl->netbuf, players[i].mo->netid );
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

void SV_Sound (fixed_t x, fixed_t y, byte channel, char *name, byte attenuation)
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
        if(deathmatch)
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
	char   *skin;
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
	p->userinfo.color	= MSG_ReadLong();

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

	if (teamplay || ctfmode)
		SV_CheckTeam (player);

	// kill player if team is changed
	if (ctfmode || teamplay)
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
		case TEAM_GOLD:

		if(TEAMenabled[player.userinfo.team])
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
		case TEAM_GOLD:
			player.userinfo.color = (0x0000204E);
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
		if(TEAMenabled[i])
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
	MSG_WriteShort(&cl->reliablebuf, INFO_LookupStateIndex(mo->state)); // denis - sending state fixes monster ghosts appearing under doors

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
		if(cl->version >= 64)
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

	if (teamplay || ctfmode)
	{
		if (a.userinfo.team == b.userinfo.team)
			return true;
		else
			return false;
	}
	else if (!deathmatch)
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
	else if(player.mo && mo->player && SV_IsTeammate(player, *mo->player))
		ok = true;
	else if(player.mo && mo->player && !antiwallhack)
		ok = true;
	else if(player.mo && mo->player && antiwallhack && P_CheckSightEdges(player.mo, mo, 5)/*player.awaresector[sectors - mo->subsector->sector]*/)
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
			MSG_WriteMarker (&cl->netbuf, svc_sector);
			MSG_WriteShort (&cl->netbuf, s);
			MSG_WriteShort (&cl->netbuf, sec->floorheight>>FRACBITS);
			MSG_WriteShort (&cl->netbuf, sec->ceilingheight>>FRACBITS);

			if(cl->version >= 63) // denis - removeme - remove the 'if' condition on this block of code - legacy protocol did not have these lines
			{
				MSG_WriteShort (&cl->netbuf, sec->floorpic);
				MSG_WriteShort (&cl->netbuf, sec->ceilingpic);
			}

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

	// update frags/points
	for (i = 0; i < players.size(); i++)
	{
		if (!players[i].ingame())
			continue;

		MSG_WriteMarker(&cl->reliablebuf, svc_updatefrags);
		MSG_WriteByte(&cl->reliablebuf, players[i].id);
		if(deathmatch)
			MSG_WriteShort(&cl->reliablebuf, players[i].fragcount);
		else
			MSG_WriteShort(&cl->reliablebuf, players[i].killcount);
		MSG_WriteShort(&cl->reliablebuf, players[i].deathcount);
		MSG_WriteShort(&cl->reliablebuf, players[i].points);
	}

	// [deathz0r] send team frags/captures if teamplay is enabled
	if(teamplay)
	{
		MSG_WriteMarker (&cl->reliablebuf, svc_teampoints);
		for (i = 0; i < NUMTEAMS; i++)
			MSG_WriteShort (&cl->reliablebuf, TEAMpoints[i]);
	}

	SV_UpdateHiddenMobj();

	// update flags
	if(ctfmode)
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
	// denis - todo - loop through all (changed?) CVAR_SERVERINFO cvars instead of rewriting this every time
	MSG_WriteMarker (&cl->reliablebuf, svc_serversettings);

	MSG_WriteByte (&cl->reliablebuf, ctfmode);

	// General server settings
//	MSG_WriteString (&cl->reliablebuf, hostname.cstring());		// denis - fixme - what happened to .string?
//	MSG_WriteString (&cl->reliablebuf, email.cstring());			// denis - fixme - what happened to .string?
	MSG_WriteShort  (&cl->reliablebuf, (int)maxplayers);

	// Game settings
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)allowcheats);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)deathmatch);
	MSG_WriteShort  (&cl->reliablebuf, (int)fraglimit);
	MSG_WriteShort  (&cl->reliablebuf, (int)timelimit);

	// Map behavior
	MSG_WriteShort  (&cl->reliablebuf, (int)skill);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)weaponstay);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)nomonsters);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)monstersrespawn);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)itemsrespawn);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)fastmonsters);

	// Action rules
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)allowexit);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)fragexitswitch);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)allowjump);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)allowfreelook);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)infiniteammo);
	MSG_WriteByte   (&cl->reliablebuf, 0); // denis - todo - use this for something

	// Teamplay/CTF
//	MSG_WriteByte   (&cl->reliablebuf, (BOOL)usectf);
	MSG_WriteShort  (&cl->reliablebuf, (int)scorelimit);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)friendlyfire);
	MSG_WriteByte   (&cl->reliablebuf, (BOOL)teamplay);

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
//	SV_ConnectClient
//
//	Called when a client connects
//
void G_DoReborn (player_t &playernum);

void SV_ConnectClient (void)
{
	int n;
	int challenge = MSG_ReadLong();
	client_t  *cl;

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

	Printf (PRINT_HIGH, "%s : connect\n", NET_AdrToString (net_from));

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

	// send welcome message
	MSG_WriteMarker   (&cl->reliablebuf, svc_print);
	MSG_WriteByte   (&cl->reliablebuf, PRINT_HIGH);
	MSG_WriteString (&cl->reliablebuf, welcomestring);

	cl->version = MSG_ReadShort();
	byte connection_type = MSG_ReadByte();

	// wrong version
	if (cl->version != VERSION
		&& cl->version != 62 && cl->version != 63) // denis - removeme - allow legacy protocol support
	{
		MSG_WriteString (&cl->reliablebuf, "Incompatible protocol version");
		MSG_WriteMarker (&cl->reliablebuf, svc_disconnect);

		SV_SendPacket (players[n]);

		return;
	}

	// send consoleplayer number
	MSG_WriteMarker (&cl->reliablebuf, svc_consoleplayer);
	MSG_WriteByte (&cl->reliablebuf, players[n].id);
	MSG_WriteString (&cl->reliablebuf, cl->digest.c_str());

	// get client userinfo
	MSG_ReadByte ();  // clc_userinfo
	SV_SetupUserInfo (players[n]); // send it to other players

	// get rate value
	cl->rate				= MSG_ReadLong();

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

	// send a map name
	MSG_WriteMarker   (&cl->reliablebuf, svc_loadmap);
	MSG_WriteString (&cl->reliablebuf, level.mapname);
	G_DoReborn (players[n]);
	SV_ClientFullUpdate (players[n]);
	SV_UpdateFrags (players[n]);
	SV_SendPacket (players[n]);

	if (!teamplay && !ctfmode)
		SV_BroadcastPrintf (PRINT_HIGH, "%s entered the game\n", players[n].userinfo.netname);
	else
	{
		if(players[n].userinfo.team == TEAM_NONE)
			SV_BroadcastPrintf (PRINT_HIGH, "%s entered the game but has not joined a team yet.\n", players[n].userinfo.netname);
		else
			SV_BroadcastPrintf (PRINT_HIGH, "%s entered the game on the %s team.\n", players[n].userinfo.netname, team_names[players[n].userinfo.team]);
	}
}


//
// SV_DisconnectClient
//
void SV_DisconnectClient(player_t &who)
{
	// already gone though this procedure?
	if(who.playerstate == PST_DISCONNECT)
		return;

	// tell others clients about it
	for (size_t i = 0; i < players.size(); i++)
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
		if(ctfmode) // [Toke - CTF]
			CTF_CheckFlags (who);

		who.mo->Destroy();
		who.mo = AActor::AActorPtr();
	}

	if (gametic - who.client.last_received == CLIENT_TIMEOUT*35) {
        if (ctfmode)
            SV_BroadcastPrintf (PRINT_HIGH, "%s timed out. (%s TEAM, %d POINTS, %d FRAGS)\n",
                who.userinfo.netname, team_names[who.userinfo.team], who.points, who.fragcount);
        else if (teamplay)
            SV_BroadcastPrintf (PRINT_HIGH, "%s timed out. (%s TEAM, %d FRAGS, %d DEATHS)\n",
                who.userinfo.netname, team_names[who.userinfo.team], who.fragcount, who.deathcount);
        else if (deathmatch)
            SV_BroadcastPrintf (PRINT_HIGH, "%s timed out. (%d FRAGS, %d DEATHS)\n",
                who.userinfo.netname, who.fragcount, who.deathcount);
        else
            SV_BroadcastPrintf (PRINT_HIGH, "%s timed out. (%d KILLS, %d DEATHS)\n",
                who.userinfo.netname, who.killcount, who.deathcount);
	} else {
        if (ctfmode)
            SV_BroadcastPrintf (PRINT_HIGH, "%s disconnected. (%s TEAM, %d POINTS, %d FRAGS)\n",
                who.userinfo.netname, team_names[who.userinfo.team], who.points, who.fragcount);
        else if (teamplay)
            SV_BroadcastPrintf (PRINT_HIGH, "%s disconnected. (%s TEAM, %d FRAGS, %d DEATHS)\n",
                who.userinfo.netname, team_names[who.userinfo.team], who.fragcount, who.deathcount);
        else if (deathmatch)
            SV_BroadcastPrintf (PRINT_HIGH, "%s disconnected. (%d FRAGS, %d DEATHS)\n",
                who.userinfo.netname, who.fragcount, who.deathcount);
        else
            SV_BroadcastPrintf (PRINT_HIGH, "%s disconnected. (%d KILLS, %d DEATHS)\n",
                who.userinfo.netname, who.killcount, who.deathcount);
    }

	who.playerstate = PST_DISCONNECT;

	if (emptyreset && players.size() == 0)
        G_DeferedInitNew(level.mapname);
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
// Called when timelimit or fraglimit hit.
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
    char str[80], str2[80];
    std::vector<player_t *> sortedplayers(players.size());
    size_t i, j;

    // Player list sorting
	for (i = 0; i < sortedplayers.size(); i++)
		sortedplayers[i] = &players[i];

    if (ctfmode) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_points);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("--------------------------------------");
        Printf_Bold("           CAPTURE THE FLAG");

        if (scorelimit)
            sprintf (str, "Scorelimit: %-6d", (int)scorelimit);
        else
            sprintf (str, "Scorelimit: N/A    ");

        if (timelimit)
            sprintf (str2, "Timelimit: %-7d", (int)timelimit);
        else
            sprintf (str2, "Timelimit: N/A");

        Printf_Bold("%s  %18s", str, str2);

        for (j = 0; j < 2; j++) {
            if (j == 0) {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("-----------------------------BLUE TEAM");
            } else {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("------------------------------RED TEAM");
            }
            Printf_Bold("Name            Points Caps Frags Time");
            Printf_Bold("--------------------------------------");

            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->userinfo.team == j) {
                    Printf(PRINT_HIGH, "%-15s %-6d N/A  %-5d  N/A",
                        sortedplayers[i]->userinfo.netname,
                        sortedplayers[i]->points,
                        //sortedplayers[i]->captures,
                        sortedplayers[i]->fragcount);
                        //sortedplayers[i]->GameTime / 60);
                }
            }
        }
    } else if (teamplay) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("--------------------------------------");
        Printf_Bold("           TEAM DEATHMATCH");

        if (fraglimit)
            sprintf (str, "Fraglimit: %-7d", (int)fraglimit);
        else
            sprintf (str, "Fraglimit: N/A    ");

        if (timelimit)
            sprintf (str2, "Timelimit: %-7d", (int)timelimit);
        else
            sprintf (str2, "Timelimit: N/A");

        Printf_Bold("%s  %18s", str, str2);

        for (j = 0; j < 2; j++) {
            if (j == 0) {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("-----------------------------BLUE TEAM");
            } else {
                Printf (PRINT_HIGH, "\n");
                Printf_Bold("------------------------------RED TEAM");
            }
            Printf_Bold("Name            Frags Deaths  K/D Time");
            Printf_Bold("--------------------------------------");

            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->userinfo.team == j) {
                    if (sortedplayers[i]->fragcount <= 0) // Copied from HU_DMScores1.
                        sprintf (str, "0.0");
                    else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0)
                        sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount);
                    else
                        sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);

                    Printf(PRINT_HIGH, "%-15s %-5d %-6d %4s  N/A\n",
                        sortedplayers[i]->userinfo.netname,
                        sortedplayers[i]->fragcount,
                        sortedplayers[i]->deathcount,
                        str);
                        //sortedplayers[i]->GameTime / 60);
                }
            }
        }
    } else if (deathmatch) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("--------------------------------------");
        Printf_Bold("              DEATHMATCH");

        if (fraglimit)
            sprintf (str, "Fraglimit: %-7d", (int)fraglimit);
        else
            sprintf (str, "Fraglimit: N/A    ");

        if (timelimit)
            sprintf (str2, "Timelimit: %-7d", (int)timelimit);
        else
            sprintf (str2, "Timelimit: N/A");

        Printf_Bold("%s  %18s", str, str2);

        Printf_Bold("Name            Frags Deaths  K/D Time");
        Printf_Bold("--------------------------------------");

        for (i = 0; i < sortedplayers.size(); i++) {
            if (sortedplayers[i]->fragcount <= 0) // Copied from HU_DMScores1.
                sprintf (str, "0.0");
            else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0)
                sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount);
            else
                sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);

            Printf(PRINT_HIGH, "%-15s %-5d %-6d %4s  N/A",
                sortedplayers[i]->userinfo.netname,
                sortedplayers[i]->fragcount,
                sortedplayers[i]->deathcount,
                str);
                //sortedplayers[i]->GameTime / 60);
        }
    } else if (multiplayer) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

        Printf (PRINT_HIGH, "\n");
        Printf_Bold("--------------------------------------");
        Printf_Bold("             COOPERATIVE");
        Printf_Bold("Name            Kills Deaths  K/D Time");
        Printf_Bold("--------------------------------------");

        for (i = 0; i < sortedplayers.size(); i++) {
            if (sortedplayers[i]->killcount <= 0) // Copied from HU_DMScores1.
                sprintf (str, "0.0");
            else if (sortedplayers[i]->killcount >= 1 && sortedplayers[i]->deathcount == 0)
                sprintf (str, "%2.1f", (float)sortedplayers[i]->killcount);
            else
                sprintf (str, "%2.1f", (float)sortedplayers[i]->killcount / (float)sortedplayers[i]->deathcount);

            Printf(PRINT_HIGH, "%-15s %-5d %-6d %4s  N/A",
                sortedplayers[i]->userinfo.netname,
                sortedplayers[i]->killcount,
                sortedplayers[i]->deathcount,
                str);
                //sortedplayers[i]->GameTime / 60);
        }
    }

    Printf (PRINT_HIGH, "\n");
}

BEGIN_COMMAND (displayscores)
{
    SV_DrawScores();
}
END_COMMAND (displayscores)

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
		if(!teamplay || players[i].userinfo.team != idplayer(who).userinfo.team)
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

	if(!team)
		SV_BroadcastPrintf (PRINT_CHAT, "%s: %s\n", player.userinfo.netname, s);
	else
		SV_TeamPrintf (PRINT_TEAMCHAT, player.id, "(TEAM) %s>> %s\n", player.userinfo.netname, s);
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
        if (!(mo->flags & MF_MISSILE) )
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
			
			MSG_WriteMarker (&cl->netbuf, svc_mobjstate);
			MSG_WriteShort (&cl->netbuf, mo->netid);
			MSG_WriteShort (&cl->netbuf, INFO_LookupStateIndex(mo->state));
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
        if (!(mo->flags & MF_COUNTKILL))
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
			
			MSG_WriteMarker (&cl->netbuf, svc_mobjstate);
			MSG_WriteShort (&cl->netbuf, mo->netid);
			MSG_WriteShort (&cl->netbuf, INFO_LookupStateIndex(mo->state));
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
	if(maxcorpses <= 0)
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
	while (corpses > maxcorpses && (mo = iterator.Next() ) )
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

				if(!SV_IsPlayerAllowedToSee(players[i], players[j].mo))
					continue;

				MSG_WriteMarker(&cl->netbuf, svc_moveplayer);
				MSG_WriteByte(&cl->netbuf, players[j].id);     // player number
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

			if (!allowfreelook)
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

		if(!speedhackfix && gamestate == GS_LEVEL)
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
		player.mo->angle = MSG_ReadShort() << 16;

		if (!allowfreelook)
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

	if(!speedhackfix && gamestate == GS_LEVEL)
	{
		P_PlayerThink(&player);
		player.mo->RunThink();
	}

	cl->lastclientcmdtic = tic;
	cl->lastcmdtic = gametic;
}


void SV_UpdateConsolePlayer(player_t &player)
{
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

	if(!TEAMenabled[team])
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

		case TEAM_GOLD:
			player.userinfo.skin = R_FindSkin ("GoldTeam"); // denis - todo - test this
			break;

		default:
			break;
	}

	if (ctfmode || teamplay)
		if (player.mo && player.userinfo.team != old_team)
			P_DamageMobj (player.mo, 0, 0, 1000, 0);
}

//
// SV_Spectate
//
void SV_Spectate (player_t &player)
{
	bool want_to_spectate = MSG_ReadByte() ? true : false;

	if(want_to_spectate)
	{
		if(player.ingame())
			SV_Suicide(player);

		player.playerstate = PST_SPECTATE;
		player.respawn_time = level.time;
	}
	else
	{
		if(player.playerstate == PST_SPECTATE && level.time > player.respawn_time + TICRATE*2)
			player.playerstate = PST_REBORN;
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

	if (MD5SUM(password + cl->digest) == challenge)
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

	if(!allowcheats)
		return;

	player.cheats = cheats;
}

void SV_WantWad(player_t &player)
{
	client_t *cl = &player.client;

	if(!waddownload)
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
	if(W_IsCommercial(wadnames[i], wadhashes[i]))
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

		if(cmd == -1)
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
			if(player.client.rate > 8000)player.client.rate = 8000;
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
			SV_Spectate (player);
			break;

		case clc_kill:
			if(player.mo && level.time > player.respawn_time + TICRATE*10)
				SV_Suicide (player);
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

		default:
			Printf(PRINT_HIGH, "SV_ParseCommands: Unknown client message %d.\n", (int)cmd);
			SV_DropClient(player);
			return;
		}

		if (msg_badread)
		{
			Printf (PRINT_HIGH, "SV_ReadClientMessage: badread (%d)\n", (int)cmd);
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

		if (teamplay)
		{
			for(size_t i = 0; i < NUMTEAMS; i++)
			{
				if(TEAMpoints[i] >= fraglimit)
				{
					SV_BroadcastPrintf (PRINT_HIGH, "Fraglimit hit. %s team wins!\n", team_names[i]);
					G_ExitLevel (0, 1);
				}
			}
		}

		else
		{
			if (shotclock == 0 && !fragexitswitch)  // [ML] 04/4/06: added !fragexitswitch
			{
				SV_BroadcastPrintf (PRINT_HIGH, "Fraglimit hit.  Game won by %s!\n", WinPlayer->userinfo.netname);

				G_ExitLevel (0, 1);
			}
		}
	}
}

//
// SV_WadDownloads
//
void SV_WadDownloads (void)
{
	// nobody around?
	if(players.empty())
		return;

	// don't send too much
	if(gametic%(2 * (players.size()+1)))
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

		if(read)
		{
			if(!cl->download.next_offset)
			{
				MSG_WriteMarker (&cl->reliablebuf, svc_wadinfo);
				MSG_WriteLong (&cl->reliablebuf, filelen);
			}

			MSG_WriteMarker (&cl->reliablebuf, svc_wadchunk);
			MSG_WriteLong (&cl->reliablebuf, cl->download.next_offset);
			MSG_WriteShort (&cl->reliablebuf, read);
			MSG_WriteChunk (&cl->reliablebuf, buff, read);

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
	int max = 0;
	team_t team = TEAM_NONE;

	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		if(TEAMpoints[i] > max)
		{
			max = TEAMpoints[i];
			team = (team_t)i;
		}
	}

	return team;
}

//
//	SV_TimelimitCheck
//
void SV_TimelimitCheck()
{
	if(!timelimit || level.time < (int)(timelimit * TICRATE * 60))
		return;
	
	// LEVEL TIMER
	if (deathmatch && !teamplay && !ctfmode)
		SV_BroadcastPrintf (PRINT_HIGH, "Timelimit hit.\n");

	if (teamplay && !ctfmode)
	{
		team_t winteam = SV_WinningTeam ();

		if(winteam == TEAM_NONE)
			SV_BroadcastPrintf(PRINT_HIGH, "No team won this game!\n");
		else
			SV_BroadcastPrintf(PRINT_HIGH, "%s team wins with a total of %d %s!\n", team_names[winteam], TEAMpoints[winteam], ctfmode ? "captures" : "frags");
	}

	G_ExitLevel(0, 1);
}

//
// SV_GameTics
//
//	Runs once gametic (35 per second gametic)
//
void SV_GameTics (void)
{
	if (ctfmode)
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

//
// SV_RunTics
//
void SV_RunTics (void)
{
	QWORD nowtime = I_GetTime ();
	QWORD newtics = nowtime - gametime;

	SV_GetPackets();

	if(newtics > 0)
	{
		std::string cmd = I_ConsoleInput();
		if (cmd.length())
		{
			AddCommandString (cmd.c_str());
		}
		
		DObject::BeginFrame ();

		// run the newtime tics
		while (newtics--)
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

		gametime = nowtime;
	}

	// wait until a network message arrives or next tick starts
	if(nowtime == I_GetTime())
		NetWaitOrTimeout((I_MSTime()%TICRATE)+1);
}


//	For Debugging
BEGIN_COMMAND (playerinfo)
{
	player_t *player = &consoleplayer();

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

	Printf (PRINT_HIGH, "---------------[player info]----------- \n");
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

BEGIN_COMMAND (monsterinfo)
{
	AActor *mo;
	TThinkerIterator<AActor> iterator;

	while ( (mo = iterator.Next ()) )
	{
		Printf (PRINT_HIGH, "%d ", mo->type);
	}
}
END_COMMAND (monsterinfo)


//
//	SV_MapEnd
//
//	Runs once at the END of each map BEFORE the map has been loaded,
//
void SV_MapEnd (void)
{
	if (ctfmode)
		CTF_Unload (); // [Toke - CTF - Setup] Turns off CTF mode at the end of each map
}

//
//	SV_MapStart
//
//	Runs once at the START of each map AFTER the map has been loaded,
//
void SV_MapStart (void)
{
	if (ctfmode)
		SV_FlagSetup (); // [Toke - CTF - Setup] Sets up the flags at the start of each CTF map

	// Setup teamplay teams
	if (teamplay && !ctfmode)
	{
		TEAMenabled[TEAM_BLUE] = blueteam ? true : false;
		TEAMenabled[TEAM_RED] = redteam ? true : false;
		TEAMenabled[TEAM_GOLD] = goldteam ? true : false;
	}
}

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


VERSION_CONTROL (sv_main_cpp, "$Id$")


