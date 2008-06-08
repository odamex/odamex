// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2008 by The Odamex Team.
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
#include "d_player.h"
#include "g_game.h"
#include "d_net.h"
#include "p_local.h"
#include "s_sound.h"
#include "gi.h"
#include "i_net.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "st_stuff.h"
#include "m_argv.h"
#include "cl_main.h"
#include "c_console.h"
#include "d_main.h"
#include "cl_ctf.h"
#include "m_random.h"
#include "w_wad.h"
#include "md5.h"
#include "m_fileio.h"

#include <string>
#include <vector>
#include <map>

// denis - fancy gfx, but no game manipulation
bool clientside = true, serverside = false;

extern bool stepmode;

// denis - client version (VERSION or other supported)
short version;

buf_t     net_buffer(MAX_UDP_PACKET);

bool      noservermsgs;
int       last_received;

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

// denis - fast netid lookup
typedef std::map<size_t, AActor::AActorPtr> netid_map_t;
netid_map_t actor_by_netid;

CVAR (friendlyfire,		"1", CVAR_SERVERINFO)
EXTERN_CVAR (weaponstay)

EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_autoaim)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_skin)
EXTERN_CVAR (cl_gender)

CVAR (maxplayers,		"0", CVAR_SERVERINFO)
CVAR (maxclients,       "0", CVAR_SERVERINFO)
CVAR (infiniteammo,		"0", CVAR_SERVERINFO)
CVAR (fraglimit,		"0", CVAR_SERVERINFO)
CVAR (timelimit,		"0", CVAR_SERVERINFO)
CVAR (nomonsters,		"0", CVAR_SERVERINFO)
CVAR (fastmonsters,		"0", CVAR_SERVERINFO)
CVAR (allowexit,		"0", CVAR_SERVERINFO)
CVAR (fragexitswitch,   "0", CVAR_SERVERINFO)       //  [ML] 03/4/06: Activate to allow exit switch at maxfrags, must click to exit
CVAR (allowjump,		"0", CVAR_SERVERINFO)
CVAR (allowfreelook,	"0", CVAR_SERVERINFO)
CVAR (scorelimit,		"0", CVAR_SERVERINFO)
CVAR (monstersrespawn,	"0", CVAR_SERVERINFO)
CVAR (itemsrespawn,		"0", CVAR_SERVERINFO)
CVAR (allowcheats,		"0", CVAR_SERVERINFO)
CVAR (teamplay,			"0", CVAR_SERVERINFO)

CVAR (allowtargetnames, "0", CVAR_SERVERINFO)

// If freelook changes serverside or clientside,
// work out what allowfreelook needs to be
EXTERN_CVAR(sv_freelook)

BEGIN_CUSTOM_CVAR (cl_freelook,		"0", CVAR_ARCHIVE)
{
	allowfreelook.Set((BOOL)(cl_freelook) &&
						((BOOL)(sv_freelook) || serverside));
}
END_CUSTOM_CVAR (cl_freelook)

BEGIN_CUSTOM_CVAR (sv_freelook,		"0", CVAR_SERVERINFO)
{
	allowfreelook.Set((BOOL)(cl_freelook) &&
						((BOOL)(sv_freelook) || serverside));
}
END_CUSTOM_CVAR (sv_freelook)

CVAR (interscoredraw, "1", CVAR_ARCHIVE)	// Nes - Determines whether to draw the scores on intermission.

void CL_RunTics (void);
void CL_PlayerTimes (void);
void CL_GetServerSettings(void);
void CL_RequestDownload(std::string filename, std::string filehash = "");
void CL_TryToConnect(DWORD server_token);
void CL_Decompress(int sequence);

//	[Toke - CTF]
void CalcTeamFrags (void);

// some doom functions (not csDoom)
void G_PlayerReborn (player_t &player);
void CL_SpawnPlayer ();
void P_KillMobj (AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void P_SetPsprite (player_t *player, int position, statenum_t stnum);
void P_ExplodeMissile (AActor* mo);
void G_SetDefaultTurbo (void);
void P_CalcHeight (player_t *player);
BOOL P_CheckMissileSpawn (AActor* th);
void CL_SetMobjSpeedAndAngle(void);

void P_PlayerLookUpDown (player_t *p);
team_t D_TeamByName (const char *team);
gender_t D_GenderByName (const char *gender);
int V_GetColorFromString (const DWORD *palette, const char *colorstring);
void AM_Stop();

void Host_EndGame(const char *msg)
{
    Printf(PRINT_HIGH, "%s", msg);
	CL_QuitNetGame();
}

void CL_QuitNetGame(void)
{
	if(connected)
	{
		MSG_WriteMarker(&net_buffer, clc_disconnect);

		NET_SendPacket(net_buffer, serveraddr);
		SZ_Clear(&net_buffer);
	}

	memset (&serveraddr, 0, sizeof(serveraddr));
	connected = false;
	ctfmode = false;
	gameaction = ga_fullconsole;
	noservermsgs = false;
	AM_Stop();

	serverside = clientside = true;
	
	sv_freelook = 1;

	actor_by_netid.clear();
	players.clear();
}


void CL_Reconnect(void)
{
	if (connected)
	{
		MSG_WriteMarker(&net_buffer, clc_disconnect);
		NET_SendPacket(net_buffer, serveraddr);
		SZ_Clear(&net_buffer);
		connected = false;
		gameaction = ga_fullconsole;

		actor_by_netid.clear();
	}
	else if (lastconaddr.ip[0])
	{
		serveraddr = lastconaddr;
	}

	connecttimeout = 0;
}


//
// CL_DisconnectClient
//
void CL_DisconnectClient(void)
{
	player_t &player = idplayer(MSG_ReadByte());

	// if this was our displayplayer, update camera
	if(&player == &displayplayer())
		displayplayer_id = consoleplayer_id;

	if(player.mo)
		player.mo->Destroy();

	size_t i;

	for(i = 0; i < players.size(); i++)
	{
		if(&players[i] == &player)
		{
			players.erase(players.begin() + i);
			break;
		}
	}

	// repair mo after player pointers are reset
	for(i = 0; i < players.size(); i++)
	{
		if(players[i].mo)
			players[i].mo->player = &players[i];
	}
}

/////// CONSOLE COMMANDS ///////

BEGIN_COMMAND (connect)
{
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
			Printf(PRINT_HIGH, "Could not resolve host %s\n", target.c_str());
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
	Printf (PRINT_HIGH, "-----------------[Players in game]----- \n");

	for (size_t i = 0; i < players.size(); ++i)
		if (players[i].ingame())
			Printf (PRINT_HIGH, "%d. - %s \n", i, players[i].userinfo.netname);

	Printf (PRINT_HIGH, "--------------------------------------- \n");
}
END_COMMAND (players)


BEGIN_COMMAND (playerinfo)
{
	player_t *player = &consoleplayer();

	if(argc > 1)
	{
		size_t who = atoi(argv[1]);
		player_t &p = idplayer(who);

		if(!validplayer(p) || !players[who].ingame())
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
	Printf (PRINT_HIGH, " userinfo.netname   - %s \n",		  player->userinfo.netname);
	Printf (PRINT_HIGH, " userinfo.team      - %d \n",		  player->userinfo.team);
	Printf (PRINT_HIGH, " userinfo.aimdist   - %d \n",		  player->userinfo.aimdist);
	Printf (PRINT_HIGH, " userinfo.color     - %d \n",		  player->userinfo.color);
	Printf (PRINT_HIGH, " userinfo.skin      - %s \n",		  skins[player->userinfo.skin].name);
	Printf (PRINT_HIGH, " userinfo.gender    - %d \n",		  player->userinfo.gender);
	Printf (PRINT_HIGH, " spectator          - %d \n",		  player->spectator);
	Printf (PRINT_HIGH, " time               - %d \n",		  player->GameTime);
	Printf (PRINT_HIGH, "--------------------------------------- \n");
}
END_COMMAND (playerinfo)


BEGIN_COMMAND (kill)
{
    if (allowcheats)
        MSG_WriteMarker(&net_buffer, clc_kill);
    else
        Printf (PRINT_HIGH, "You must run the server with '+set allowcheats 1' to enable this command.\n");
}
END_COMMAND (kill)


BEGIN_COMMAND (gamemode)
{
						Printf (PRINT_HIGH,	"----------------------[Game Modes]----- \n");
						Printf (PRINT_HIGH,	" Deathmatch - %d \n", deathmatch.value());
    if (ctfmode)        Printf (PRINT_HIGH, " CTF	     - ON \n");
    else                Printf (PRINT_HIGH, " CTF        - OFF \n");
    if (teamplay)       Printf (PRINT_HIGH, " TeamPlay   - ON \n");
    else                Printf (PRINT_HIGH, " TeamPlay   - OFF \n");
						Printf (PRINT_HIGH,	"--------------------------------------- \n");
}
END_COMMAND (gamemode)


BEGIN_COMMAND (serverinfo)
{
	Printf (PRINT_HIGH,	"----------------------[Server Info]---- \n"				);
	Printf (PRINT_HIGH, "         ctfmode - %d \n",	(int)ctfmode			);
	Printf (PRINT_HIGH, "        teamplay - %d \n",	(int)teamplay	);
	Printf (PRINT_HIGH, "                      \n"									);
	Printf (PRINT_HIGH, "        hostname -    \n"									);
	Printf (PRINT_HIGH, "           email -    \n"									);
	Printf (PRINT_HIGH, "       allowcheats - %d \n",	(BOOL)allowcheats		);
	Printf (PRINT_HIGH, "      deathmatch - %d \n",	(BOOL)deathmatch		);
	Printf (PRINT_HIGH, "       fraglimit - %d \n",	(int)fraglimit		);
	Printf (PRINT_HIGH, "       timelimit - %d \n",	(int)timelimit		);
	Printf (PRINT_HIGH, "                      \n"									);
	Printf (PRINT_HIGH, "       skill - %d \n",	(int)skill	);
	Printf (PRINT_HIGH, "      weaponstay - %d \n",	(BOOL)weaponstay		);
	Printf (PRINT_HIGH, "      nomonsters - %d \n",	(BOOL)nomonsters		);
	Printf (PRINT_HIGH, " monstersrespawn - %d \n",	(BOOL)monstersrespawn);
	Printf (PRINT_HIGH, "    itemsrespawn - %d \n",	(BOOL)itemsrespawn	);
	Printf (PRINT_HIGH, "    fastmonsters - %d \n",	(BOOL)fastmonsters	);
	Printf (PRINT_HIGH, "                      \n"									);
	Printf (PRINT_HIGH, "       allowexit - %d \n",	(BOOL)allowexit		);
    Printf (PRINT_HIGH, "  fragexitswitch - %d \n",	(BOOL)fragexitswitch);
	Printf (PRINT_HIGH, "       allowjump - %d \n",	(BOOL)allowjump		);
	Printf (PRINT_HIGH, "   allowfreelook - %d \n",	(BOOL)sv_freelook	);
	Printf (PRINT_HIGH, "    infiniteammo - %d \n",	(BOOL)infiniteammo	);
	Printf (PRINT_HIGH, "                      \n"									);
	Printf (PRINT_HIGH, "      scorelimit - %d \n",	(int)scorelimit		);
	Printf (PRINT_HIGH, "    friendlyfire - %d \n",	(BOOL)friendlyfire	);
	Printf (PRINT_HIGH, "                      \n"									);
	Printf (PRINT_HIGH, "allowtargetnames - %d \n", (BOOL)allowtargetnames);
	Printf (PRINT_HIGH,	"--------------------------------------- \n"				);
}
END_COMMAND (serverinfo)

BEGIN_CUSTOM_CVAR (cl_predict_players, "1", CVAR_ARCHIVE)
{
	if (var > 1)
	{
		var.Set (1);
	}
}
END_CUSTOM_CVAR (cl_predict_players)


BEGIN_CUSTOM_CVAR (rate, "10000", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	if (var < 100)
	{
		var.Set (100);
	}
	else if (var > 100000)
	{
		var.Set (100000);
	}
	else if (connected)
	{
		MSG_WriteMarker(&net_buffer, clc_rate);
		MSG_WriteLong(&net_buffer, (int)var);
	}
}
END_CUSTOM_CVAR (rate)


BEGIN_COMMAND (rcon)
{
	if (connected && argc > 1)
	{
		char  command[256];

		if (argc == 2)
			sprintf(command, "%s", argv[1]);
		if (argc == 3)
			sprintf(command, "%s %s", argv[1], argv[2]);
		if (argc == 4)
			sprintf(command, "%s %s %s", argv[1], argv[2], argv[3]);

		MSG_WriteMarker(&net_buffer, clc_rcon);
		MSG_WriteString(&net_buffer, command);
	}
}
END_COMMAND (rcon)


BEGIN_COMMAND (rcon_password)
{
	if (connected && argc > 1)
	{
		MSG_WriteMarker(&net_buffer, clc_rcon_password);

		std::string password = argv[1];
		MSG_WriteString(&net_buffer, MD5SUM(password + digest).c_str());
	}
}
END_COMMAND (rcon_password)


BEGIN_COMMAND (playerteam)
{
	Printf (PRINT_MEDIUM, "Your Team is %d \n", consoleplayer().userinfo.team);
}
END_COMMAND (playerteam)


BEGIN_COMMAND (spectate)
{
	MSG_WriteMarker(&net_buffer, clc_spectate);
	MSG_WriteByte(&net_buffer, true);
}
END_COMMAND (spectate)


BEGIN_COMMAND (join)
{
	MSG_WriteMarker(&net_buffer, clc_spectate);
	MSG_WriteByte(&net_buffer, false);
}
END_COMMAND (join)

BEGIN_COMMAND (quit)
{
	exit (0);
}
END_COMMAND (quit)

//
// CL_MoveThing
//
void CL_MoveThing(AActor *mobj, fixed_t x, fixed_t y, fixed_t z)
{
	P_CheckPosition (mobj, x, y);
	mobj->UnlinkFromWorld ();

	mobj->x = x;
	mobj->y = y;
	mobj->z = z;
	mobj->floorz = tmfloorz;
	mobj->ceilingz = tmceilingz;

	mobj->LinkToWorld ();
}

//
// CL_SendUserInfo
//
void CL_SendUserInfo(void)
{
	userinfo_t coninfo;

    memset (&coninfo, 0, sizeof(coninfo));

	strncpy (coninfo.netname, cl_name.cstring(), MAXPLAYERNAME);
	coninfo.team	 = D_TeamByName (cl_team.cstring()); // [Toke - Teams]
	coninfo.aimdist = (fixed_t)(cl_autoaim * 16384.0);
	coninfo.color	 = V_GetColorFromString (NULL, cl_color.cstring());
	coninfo.skin	 = R_FindSkin (cl_skin.cstring());
	coninfo.gender  = D_GenderByName (cl_gender.cstring());

	MSG_WriteMarker	(&net_buffer, clc_userinfo);
	MSG_WriteString	(&net_buffer, coninfo.netname);
	MSG_WriteByte	(&net_buffer, coninfo.team); // [Toke]
	MSG_WriteLong	(&net_buffer, coninfo.gender);
	MSG_WriteLong	(&net_buffer, coninfo.color);
	MSG_WriteString	(&net_buffer, (char *)skins[coninfo.skin].name); // [Toke - skins]
	MSG_WriteLong	(&net_buffer, coninfo.aimdist);
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
		if(players.size() >= MAXPLAYERS)
			return *p;

		players.push_back(player_s());

		p = &players.back();
		p->id = id;

		// repair mo after player pointers are reset
		for(size_t i = 0; i < players.size(); i++)
		{
			if(players[i].mo)
				players[i].mo->player = &players[i];
		}
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

	strncpy(p->userinfo.netname, MSG_ReadString(), sizeof(p->userinfo.netname));

	p->userinfo.team	= (team_t)MSG_ReadByte();
	p->userinfo.gender	= (gender_t)MSG_ReadLong();
	p->userinfo.color	= MSG_ReadLong();
	p->userinfo.skin	= R_FindSkin(MSG_ReadString());

	p->GameTime			= MSG_ReadShort();

	R_BuildPlayerTranslation (p->id, p->userinfo.color);

	if(p->userinfo.gender >= NUMGENDER)
		p->userinfo.gender = GENDER_NEUTER;

	if (p->mo)
		p->mo->sprite = skins[p->userinfo.skin].sprite;

	extern bool st_firsttime;
	st_firsttime = true;
}


//
// CL_UpdateFrags
//
void CL_UpdateFrags(void)
{
	player_t &p = CL_FindPlayer(MSG_ReadByte());

	if(deathmatch)
		p.fragcount = MSG_ReadShort();
	else
		p.killcount = MSG_ReadShort();
	p.deathcount = MSG_ReadShort();
	p.points = MSG_ReadShort();
}

//
// [deathz0r] Receive team frags/captures
//
void CL_TeamPoints (void)
{
	for(size_t i = 0; i < NUMTEAMS; i++)
		TEAMpoints[i] = MSG_ReadShort();
}

//
// denis - fast netid lookup
//
AActor* CL_FindThingById(size_t id)
{
	netid_map_t::iterator i = actor_by_netid.find(id);

	if(i == actor_by_netid.end())
		return AActor::AActorPtr();
	else
		return i->second;
}

void CL_SetThingId(AActor *mo, size_t newnetid)
{
	mo->netid = newnetid;
	actor_by_netid[newnetid] = mo->ptr();
}

void CL_ClearID(size_t id)
{
    AActor *mo = CL_FindThingById(id);

	if(!mo)
		return;

	if(mo->player)
	{
		if(mo->player->mo == mo)
			mo->player->mo = AActor::AActorPtr();

		mo->player = NULL;
	}

	mo->Destroy();
}

//
// CL_MoveMobj
//
void CL_MoveMobj(void)
{
	AActor  *mo;
	int      netid;
	fixed_t  x, y, z;

	netid = MSG_ReadShort();
	mo = CL_FindThingById (netid);

	byte rndindex = MSG_ReadByte();
	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();

	if (!mo)
		return;

	CL_MoveThing (mo, x, y, z);
	mo->rndindex = rndindex;
}

//
// CL_DamageMobj
//
void CL_DamageMobj()
{
	AActor  *mo;
	int      netid, health, pain;

	netid = MSG_ReadShort();
	health = MSG_ReadShort();
	pain = MSG_ReadByte();

	mo = CL_FindThingById (netid);

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

	if(gamestate != GS_DOWNLOAD)
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

//
// [denis] CL_PrepareConnect
// Process server info and switch to the right wads...
//
std::string missing_file, missing_hash;
bool CL_PrepareConnect(void)
{
	size_t i;
	DWORD server_token = MSG_ReadLong();
	std::string server_host = MSG_ReadString();

	byte recv_teamplay_stats = 0;

	byte playercount = MSG_ReadByte(); // players
	MSG_ReadByte(); // max_players

	std::string server_map = MSG_ReadString();
	byte server_wads = MSG_ReadByte();

	Printf(PRINT_HIGH, "\n");
	Printf(PRINT_HIGH, "> Server: %s\n", server_host.c_str());
	Printf(PRINT_HIGH, "> Map: %s\n", server_map.c_str());

	std::vector<std::string> wadnames(server_wads);
	for(i = 0; i < server_wads; i++)
		wadnames[i] = MSG_ReadString();

	MSG_ReadByte();							// gametype
	MSG_ReadByte();							// skill
	recv_teamplay_stats |= MSG_ReadByte();	// teamplay
	recv_teamplay_stats |= MSG_ReadByte();	// ctf

	for(i = 0; i < playercount; i++)
	{
		MSG_ReadString();
		MSG_ReadShort();
		MSG_ReadLong();
		MSG_ReadByte();
	}

	std::vector<std::string> wadhashes(server_wads);
	for(i = 0; i < server_wads; i++)
	{
		wadhashes[i] = MSG_ReadString();
		Printf(PRINT_HIGH, "> %s\n   %s\n", wadnames[i].c_str(), wadhashes[i].c_str());
	}

	MSG_ReadString();

	if (ctfmode)
	{
		MSG_ReadLong();
		if (MSG_ReadByte())
			MSG_ReadLong();
		if (MSG_ReadByte())
			MSG_ReadLong();
		if (MSG_ReadByte())
			MSG_ReadLong();
	}

	// Receive conditional teamplay information
	if (recv_teamplay_stats)
	{
		MSG_ReadLong();

		for(size_t i = 0; i < NUMTEAMS; i++)
		{
			byte enabled = MSG_ReadByte();

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

	Printf(PRINT_HIGH, "\n");

	std::vector<size_t> missing_files = D_DoomWadReboot(wadnames, wadhashes);

	if(missing_files.size())
	{
		// denis - download files
		missing_file = wadnames[missing_files[0]];
		missing_hash = wadhashes[missing_files[0]];
		gamestate = GS_DOWNLOAD;
		Printf(PRINT_HIGH, "Will download \"%s\" from server\n", missing_file.c_str());
	}

	connecttimeout = 0;
	CL_TryToConnect(server_token);

	return true;
}

//
//  Connecting to a server...
//
extern std::string missing_file, missing_hash;
bool CL_Connect(void)
{
	players.clear();

	memset(packetseq, -1, sizeof(packetseq) );
	packetnum = 0;

	MSG_WriteMarker(&net_buffer, clc_ack);
	MSG_WriteLong(&net_buffer, 0);

	if(gamestate == GS_DOWNLOAD && missing_file.length())
	{
		// denis - do not download commercial wads
		if(W_IsIWAD(missing_file, missing_hash))
		{
			Printf(PRINT_HIGH, "This is a commercial wad and will not be downloaded.\n");
			CL_QuitNetGame();
			return false;
		}

		CL_RequestDownload(missing_file, missing_hash);
	}

	compressor.reset();

	connected = true;
    multiplayer = true;
	serverside = false;

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

	return true;
}


//
// CL_InitNetwork
//
void CL_InitNetwork (void)
{
    netgame = false;  // for old network code

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

    const char *ipaddress = Args.CheckValue ("-connect");

    if (ipaddress)
    {
		NET_StringToAdr (ipaddress, &serveraddr);

		if (!serveraddr.port)
			I_SetPort(serveraddr, SERVERPORT);

		lastconaddr = serveraddr;
		gamestate = GS_CONNECTING;
    }

	G_SetDefaultTurbo ();

    connected = false;
}

void CL_TryToConnect(DWORD server_token)
{
	if (!serveraddr.ip[0])
		return;

	if (!connecttimeout)
	{
		connecttimeout = 140; // 140 tics = 4 seconds

		Printf(PRINT_HIGH, "challenging %s\n", NET_AdrToString(serveraddr));

		SZ_Clear(&net_buffer);
		MSG_WriteLong(&net_buffer, CHALLENGE); // send challenge
		MSG_WriteLong(&net_buffer, server_token); // confirm server token
		MSG_WriteShort(&net_buffer, version); // send client version

		if(gamestate == GS_DOWNLOAD)
			MSG_WriteByte(&net_buffer, 1); // send type of connection (play/spectate/rcon/download)
		else
			MSG_WriteByte(&net_buffer, 0); // send type of connection (play/spectate/rcon/download)
			
		// GhostlyDeath -- Send more version info
		MSG_WriteLong(&net_buffer, GAMEVER);

		CL_SendUserInfo(); // send userinfo

		MSG_WriteLong(&net_buffer, (int)rate);

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

	Printf (level, "%s", str);

	if ((level == PRINT_CHAT || level == PRINT_TEAMCHAT) && show_messages)
		S_Sound (CHAN_VOICE, gameinfo.chatSound, 1, ATTN_NONE);
}


void CL_UpdatePlayer()
{
	byte who = MSG_ReadByte();
	player_t *p = &idplayer(who);

	int x, y, z;

	if(!validplayer(*p) || !p->mo)
	{
		for (int i=0; i<33; i++)
			MSG_ReadByte();
		return;
	}

	int sv_gametic = MSG_ReadLong();
	
	// GhostlyDeath -- Servers will never send updates on spectators
	if (p->spectator && (p != &consoleplayer()))
		p->spectator = 0;

	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();
	CL_MoveThing(p->mo, x, y, z);
	p->mo->angle = MSG_ReadLong();
	byte frame = MSG_ReadByte();
	p->mo->momx = MSG_ReadLong();
	p->mo->momy = MSG_ReadLong();
	p->mo->momz = MSG_ReadLong();

	p->real_origin[0] = x;
	p->real_origin[1] = y;
	p->real_origin[2] = z;

	p->real_velocity[0] = p->mo->momx;
	p->real_velocity[1] = p->mo->momy;
	p->real_velocity[2] = p->mo->momz;

	p->mo->frame = frame;

	// This is a very bright frame. Looks cool :)
	if (p->mo->frame == PLAYER_FULLBRIGHTFRAME)
		p->mo->frame = 32773;

	// denis - fixme - security
	if(!p->mo->sprite || (p->mo->frame&FF_FRAMEMASK) >= sprites[p->mo->sprite].numframes)
		return;

	p->last_received = gametic;
	p->tic = sv_gametic;
}

ticcmd_t localcmds[MAXSAVETICS];

void CL_SaveCmd(void)
{
	memcpy (&localcmds[gametic%MAXSAVETICS], &consoleplayer().cmd, sizeof(ticcmd_t));
}

//
// CL_UpdateLocalPlayer
//
void CL_UpdateLocalPlayer(void)
{
	player_t &p = consoleplayer();

	p.tic = MSG_ReadLong();
	p.real_origin[0] = MSG_ReadLong();
	p.real_origin[1] = MSG_ReadLong();
	p.real_origin[2] = MSG_ReadLong();

	p.real_velocity[0] = MSG_ReadLong();
	p.real_velocity[1] = MSG_ReadLong();
	p.real_velocity[2] = MSG_ReadLong();

	real_plats.clear();
}

void CL_ResendSvGametic(void)
{
	int svgametic = MSG_ReadLong();

	MSG_WriteMarker (&net_buffer, clc_svgametic);
	MSG_WriteLong (&net_buffer, svgametic);
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
// CL_SpawnMobj
//
void CL_SpawnMobj()
{
	fixed_t  x = 0, y = 0, z = 0;
	AActor  *mo;

	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();
	angle_t angle = MSG_ReadLong();

	unsigned short type = MSG_ReadShort();
	unsigned short netid = MSG_ReadShort();
	byte rndindex = MSG_ReadByte();
	SWORD state = MSG_ReadShort();

	if(type >= NUMMOBJTYPES)
		return;

	CL_ClearID(netid);

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
	CL_SetThingId(mo, netid);
	mo->rndindex = rndindex;

	if (state < NUMSTATES)
		P_SetMobjState(mo, (statenum_t)state);

	if(mo->flags & MF_MISSILE)
	{
		AActor *target = CL_FindThingById(MSG_ReadShort());
		if(target)
			mo->target = target->ptr();
		CL_SetMobjSpeedAndAngle();
		P_CheckMissileSpawn(mo);
	}

    if (mo->flags & MF_COUNTKILL)
		level.total_monsters++;

	if (connected && (mo->flags & MF_MISSILE ) && mo->info->seesound)
		S_Sound (mo, CHAN_VOICE, mo->info->seesound, 1, ATTN_NORM);

	if (mo->type == MT_IFOG)
		S_Sound (mo, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);

	if (mo->type == MT_TFOG)
		S_Sound (mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
}

//
// CL_Corpse
// Called after killed thing is created.
//
void CL_Corpse(void)
{
	AActor *mo = CL_FindThingById(MSG_ReadShort());
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
		
    if (mo->flags & MF_COUNTKILL)
		level.killed_monsters++;
}

//
// CL_TouchSpecialThing
//
void CL_TouchSpecialThing (void)
{
	byte who = MSG_ReadByte();
	AActor *mo = CL_FindThingById(MSG_ReadShort());
	player_t &p = idplayer(who);

	if(!validplayer(p) || !mo)
		return;

	if (!p.mo)
		return;

	//P_TouchSpecialThing (mo, p.mo);
}


extern fixed_t cl_viewheight[MAXSAVETICS];
extern fixed_t cl_deltaviewheight[MAXSAVETICS];

//
// CL_SpawnPlayer
//
void CL_SpawnPlayer()
{
	byte		playernum;
	player_t	*p;
	AActor		*mobj;
	fixed_t		x = 0, y = 0, z = 0;
	unsigned short netid;
	angle_t		angle = 0;
	int			i;

	playernum = MSG_ReadByte();
	netid = MSG_ReadShort();
	p = &CL_FindPlayer(playernum);

	angle = MSG_ReadLong();
	x = MSG_ReadLong();
	y = MSG_ReadLong();
	z = MSG_ReadLong();
		
	// GhostlyDeath -- reset prediction
	p->real_origin[0] = x;
	p->real_origin[1] = y;
	p->real_origin[2] = z;

	p->real_velocity[0] = 0;
	p->real_velocity[1] = 0;
	p->real_velocity[2] = 0;

	CL_ClearID(netid);

	// first disassociate the corpse
	if (p->mo)
	{
		p->mo->player = NULL;
		p->mo->health = 0;
	}

	G_PlayerReborn (*p);

	// denis - if this concerns the local player, restart the status bar
	if(p->id == consoleplayer_id)
		ST_Start ();

	mobj = new AActor (x, y, z, MT_PLAYER);

	// set color translations for player sprites
	mobj->translation = translationtables + 256*playernum;
	mobj->angle = angle;
	mobj->pitch = mobj->roll = 0;
	mobj->player = p;
	mobj->health = p->health;
	CL_SetThingId(mobj, netid);

	// [RH] Set player sprite based on skin
	if(p->userinfo.skin >= numskins)
		p->userinfo.skin = 0;

	mobj->sprite = skins[p->userinfo.skin].sprite;

	p->mo = p->camera = mobj->ptr();
	p->fov = 90.0f;
	p->playerstate = PST_LIVE;
	p->refire = 0;
	p->damagecount = 0;
	p->bonuscount = 0;
	p->extralight = 0;
	p->fixedcolormap = 0;

	p->viewheight = VIEWHEIGHT;
	for (i=0; i<MAXSAVETICS; i++)
	{
		cl_viewheight[i] = VIEWHEIGHT;
		cl_deltaviewheight[i] = 0;
	}

	p->attacker = AActor::AActorPtr();
	p->viewz = z + VIEWHEIGHT;

	// spawn a teleport fog
	// tfog = new AActor (x, y, z, MT_TFOG);

	// setup gun psprite
	P_SetupPsprites (p);

	// give all cards in death match mode
	if(deathmatch)
		for (i = 0; i < NUMCARDS; i++)
			p->cards[i] = true;
}

//
// CL_PlayerInfo
// denis - your personal arsenal, as supplied by the server
//
void CL_PlayerInfo(void)
{
	size_t j;

	player_t *p = &consoleplayer();

	for(j = 0; j < NUMWEAPONS; j++)
	{
		p->weaponowned[j] = MSG_ReadByte () ? true : false;
	}

	for(j = 0; j < NUMAMMO; j++)
	{
		p->maxammo[j] = MSG_ReadShort ();
		p->ammo[j] = MSG_ReadShort ();
	}

	p->health = MSG_ReadByte ();
	p->armorpoints = MSG_ReadByte ();
	p->armortype = MSG_ReadByte ();
	p->pendingweapon = (weapontype_t)MSG_ReadByte ();
	p->backpack = MSG_ReadByte () ? true : false;

	if(p->pendingweapon > NUMWEAPONS)
		p->pendingweapon = wp_pistol;
}

//
// CL_SetMobjSpeedAndAngle
//
void CL_SetMobjSpeedAndAngle(void)
{
	AActor *mo;
	int     netid;

	netid = MSG_ReadShort();
	mo = CL_FindThingById(netid);

	if (!mo)
	{
		for (int i=0; i<4; i++)
			MSG_ReadLong();
		return;
	}

	mo->angle = MSG_ReadLong();
	mo->momx = MSG_ReadLong();
	mo->momy = MSG_ReadLong();
	mo->momz = MSG_ReadLong();
}

//
// CL_ExplodeMissile
//
void CL_ExplodeMissile(void)
{
    AActor *mo;
	int     netid;

	netid = MSG_ReadShort();
	mo = CL_FindThingById(netid);

	if (!mo)
		return;

	P_ExplodeMissile(mo);
}


//
// CL_UpdateMobjInfo
//
void CL_UpdateMobjInfo(void)
{
	int netid = MSG_ReadShort();
	int flags = MSG_ReadLong();

	AActor *mo = CL_FindThingById(netid);

	if (!mo)
		return;

	mo->flags = flags;
}


//
// CL_RemoveMobj
//
void CL_RemoveMobj(void)
{
	CL_ClearID(MSG_ReadShort());
}


//
// CL_DamagePlayer
//
void CL_DamagePlayer(void)
{
	player_t *p;
	int       health;
	int       damage;

	p = &idplayer(MSG_ReadByte());

	p->armorpoints = MSG_ReadByte();
	health         = MSG_ReadShort();

	if(!p->mo)
		return;

	damage = p->health - health;
	p->mo->health = p->health = health;

	if (p->health < 0)
		p->health = 0;

	if (damage < 0)  // can't be!
		return;

	if (damage > 0) {
		p->damagecount += damage;

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
void CL_KillMobj(void)
{
 	AActor *source = CL_FindThingById (MSG_ReadShort() );
	AActor *target = CL_FindThingById (MSG_ReadShort() );
	AActor *inflictor = CL_FindThingById (MSG_ReadShort() );
	int health = MSG_ReadShort();

	MeansOfDeath = MSG_ReadLong();
	
	bool joinkill = MSG_ReadByte();

	if (!target)
		return;

	target->health = health;

    if (!serverside && target->flags & MF_COUNTKILL)
		level.killed_monsters++;

	P_KillMobj (source, target, inflictor, joinkill);	
}


///////////////////////////////////////////////////////////
///// CL_Fire* called when someone uses a weapon  /////////
///////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////
/*
void CL_ChangeWeapon (void)
{
player_t* player = &players[consoleplayer];

	player->pendingweapon = (weapontype_t)MSG_ReadByte();

	 // Now set appropriate weapon overlay.
	 P_SetPsprite (player,
	 ps_weapon,
	 weaponinfo[player->readyweapon].downstate);
	 }
*/


//
// CL_Sound
//
void CL_Sound(void)
{
	int netid = MSG_ReadShort();
	int x = MSG_ReadLong();
	int y = MSG_ReadLong();
	byte channel = MSG_ReadByte();
	byte sfx_id = MSG_ReadByte();
	byte attenuation = MSG_ReadByte();
	byte vol = MSG_ReadByte();

	AActor *mo = CL_FindThingById (netid);

	float volume = vol/(float)255;

	if(volume < 0.25f)
	{
		// so far away that it becomes directionless
		AActor *mo = consoleplayer().mo;

		if(mo)
			S_SoundID (mo->x, mo->y, attenuation, sfx_id, volume, attenuation);

		return;
	}

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

	if(volume < 0.25f)
	{
		x = mo->x;
		y = mo->y;
		S_SoundID (x, y, channel, sfx_id, volume, attenuation);
		return;
	}

	S_SoundID (x, y, channel, sfx_id, volume, attenuation);
}

//
// CL_UpdateSector
// Updates floorheight and ceilingheight of a sector.
//
void CL_UpdateSector(void)
{
	unsigned short s = (unsigned short)MSG_ReadShort();
	unsigned short fh = MSG_ReadShort();
	unsigned short ch = MSG_ReadShort();

	unsigned short fp = MSG_ReadShort();
	unsigned short cp = MSG_ReadShort();

	if(!sectors || s >= numsectors)
		return;

	sector_t *sec = &sectors[s];
	sec->floorheight = fh << FRACBITS;
	sec->ceilingheight = ch << FRACBITS;

	if(fp >= numflats)
		fp = numflats;

	sec->floorpic = fp;

	if(cp >= numflats)
		cp = numflats;

	sec->ceilingpic = cp;

	P_ChangeSector (sec, false);
}

//
// CL_UpdateMovingSector
// Updates floorheight and ceilingheight of a sector.
//
void CL_UpdateMovingSector(void)
{
	int tic = MSG_ReadLong();
	unsigned short s = (unsigned short)MSG_ReadShort();
	unsigned long fh = MSG_ReadLong(); // floor height
	MSG_ReadLong(); // ceiling height
	byte state = MSG_ReadByte();
	int count = MSG_ReadLong();

	if(!sectors || s >= numsectors)
		return;

	plat_pred_t pred = {s, state, count, tic, fh};
//	sector_t *sec = &sectors[s];

//	if(!sec->floordata)
//		sec->floordata = new DMovingFloor(sec);

	size_t i;

	for(i = 0; i < real_plats.size(); i++)
	{
		if(real_plats[i].secnum == s)
		{
			real_plats[i] = pred;
			break;
		}
	}

	if(i == real_plats.size())
		real_plats.push_back(pred);
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
			for (int i=0; i<size; i++)
				MSG_ReadByte();

			#ifdef _DEBUG
                Printf (PRINT_LOW, "warning: duplicate packet\n");
			#endif
			return;
		}
	}
}

void CL_Decompress(int sequence)
{
	if(!MSG_BytesLeft() || MSG_NextByte() != svc_compressed)
		return;
	else
		MSG_ReadByte();

	byte method = MSG_ReadByte();

	if(method & minilzo_mask)
		MSG_DecompressMinilzo();

	if(method & adaptive_mask)
		MSG_DecompressAdaptive(compressor.codec_for_received(method & adaptive_select_mask ? 1 : 0));
	else
	{
		// otherwise compressed packets can still contain codec updates
		compressor.codec_for_received(method & adaptive_select_mask ? 1 : 0);
	}

	if(method & adaptive_record_mask)
		compressor.ack_sent(net_message.ptr(), MSG_BytesLeft());
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
}

void CL_GetServerSettings(void)
{
	ctfmode = MSG_ReadByte() ? true : false;

	// General server settings
	maxclients.Set((int)MSG_ReadShort());

	// Game settings
	allowcheats.Set((BOOL)MSG_ReadByte());
	deathmatch.Set((BOOL)MSG_ReadByte());
	fraglimit.Set((int)MSG_ReadShort());
	timelimit.Set((int)MSG_ReadShort());

	// Map behavior
	skill.Set((int)MSG_ReadShort());
	weaponstay.Set((BOOL)MSG_ReadByte());
	nomonsters.Set((BOOL)MSG_ReadByte());
	monstersrespawn.Set((BOOL)MSG_ReadByte());
	itemsrespawn.Set((BOOL)MSG_ReadByte());
	fastmonsters.Set((BOOL)MSG_ReadByte());

	// Action rules
	allowexit.Set((BOOL)MSG_ReadByte());
	fragexitswitch.Set((BOOL)MSG_ReadByte());
	allowjump.Set((BOOL)MSG_ReadByte());
	sv_freelook.Set((BOOL)MSG_ReadByte());
	infiniteammo.Set((BOOL)MSG_ReadByte());
    maxplayers.Set((int)MSG_ReadByte());

	// Teamplay/CTF
	scorelimit.Set((int)MSG_ReadShort());
	friendlyfire.Set((BOOL)MSG_ReadByte());
	teamplay.Set(MSG_ReadByte());
	
	allowtargetnames.Set((BOOL)MSG_ReadByte());

	cvar_t::UnlatchCVars ();
}

//
// CL_SetMobjState
//
void CL_SetMobjState()
{
	AActor *mo = CL_FindThingById (MSG_ReadShort() );
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
}

//
// CL_Actor_Movedir
//
void CL_Actor_Movedir()
{
	AActor *actor = CL_FindThingById (MSG_ReadShort());
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
	AActor *actor = CL_FindThingById (MSG_ReadShort());
	AActor *target = CL_FindThingById (MSG_ReadShort());

	if (!actor || !target)
		return;

	actor->target = target->ptr();
}

//
// CL_Actor_Tracer
//
void CL_Actor_Tracer()
{
	AActor *actor = CL_FindThingById (MSG_ReadShort());
	AActor *tracer = CL_FindThingById (MSG_ReadShort());

	if (!actor || !tracer)
		return;

	actor->tracer = tracer->ptr();
}

//
// CL_Switch
// denis - switch state and timing
//
void CL_Switch()
{
	unsigned l = MSG_ReadLong();
	byte wastoggled = MSG_ReadByte();
	byte state = MSG_ReadByte();
	unsigned time = MSG_ReadLong();

	if (!lines || l >= (unsigned)numlines || state >= 3)
		return;

	if(!P_SetButtonInfo(&lines[l], state, time)) // denis - fixme - security
		if(wastoggled)
			P_ChangeSwitchTexture(&lines[l], lines[l].flags & ML_SPECIAL_REPEAT);  // denis - fixme - security

	if(wastoggled && !(lines[l].flags & ML_SPECIAL_REPEAT)) // non repeat special
		lines[l].special = 0;
}

void CL_ActivateLine(void)
{
	unsigned l = MSG_ReadLong();
	AActor *mo = CL_FindThingById(MSG_ReadShort());
	byte side = MSG_ReadByte();
	byte activationType = MSG_ReadByte();

	if (!lines || l >= (unsigned)numlines)
		return;

	if(mo == consoleplayer().mo && activationType != 2)
		return;

	switch (activationType)
	{
	case 0:
		P_CrossSpecialLine(l, side, mo);
		break;
	case 1:
		P_UseSpecialLine(mo, &lines[l], side);
		break;
	case 2:
		P_ShootSpecialLine(mo, &lines[l]);
		break;
	}
}

void CL_ConsolePlayer(void)
{
	displayplayer_id = consoleplayer_id = MSG_ReadByte();
	digest = MSG_ReadString();
}

void CL_LoadMap(void)
{
	const char *mapname = MSG_ReadString ();

	if(gamestate == GS_DOWNLOAD)
		return;

	if(gamestate == GS_LEVEL)
		G_ExitLevel (0, 0);

	G_InitNew (mapname);

	real_plats.clear();

	CTF_CheckFlags(consoleplayer());

	gameaction = ga_nothing;
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
	if(gamestate != GS_DOWNLOAD) {
        if (multiplayer && interscoredraw)
            AddCommandString("displayscores");
		gameaction = ga_completed;
	}
}

struct download_t
{
	std::string filename, md5;
	buf_t *buf;
	unsigned int got_bytes;
} download = { "", "", NULL, 0 };


void IntDownloadComplete(void)
{
    std::string actual_md5 = MD5SUM(download.buf->ptr(), download.buf->maxsize());

	Printf(PRINT_HIGH, "\nDownload complete, got %u bytes\n", download.buf->maxsize());
	Printf(PRINT_HIGH, "%s\n %s\n", download.filename.c_str(), actual_md5.c_str());

	if(download.md5 == "")
	{
		Printf(PRINT_HIGH, "Server gave no checksum, assuming valid\n", (int)download.buf->maxsize());
	}
	else if(actual_md5 != download.md5)
	{
		Printf(PRINT_HIGH, " %s on server\n", download.md5.c_str());
		Printf(PRINT_HIGH, "Download failed: bad checksum\n");

        download.filename = "";
        download.md5 = "";
        download.got_bytes = 0;
		
        if (download.buf != NULL)
        {
            delete download.buf;
            download.buf = NULL;
        }   
			
        CL_QuitNetGame();
        return;
    }

    // got the wad! save it!
    std::string filename = "./"; // denis - todo try first of waddir/DOOMWADDIR/startdir/progdir in that order
    filename += download.filename;

    // check for existing file
    if(M_FileExists(filename.c_str()))
    {
        // there is an existing file, so use a new file whose name includes the checksum
        filename += ".";
        filename += actual_md5;
    }

    if (!M_WriteFile(filename, download.buf->ptr(), download.buf->maxsize()))
    {
        download.filename = "";
        download.md5 = "";
        download.got_bytes = 0;
		
        if (download.buf != NULL)
        {
            delete download.buf;
            download.buf = NULL;
        }   
            
        CL_QuitNetGame();
        return;            
    }

    Printf(PRINT_HIGH, "Saved download as \"%s\"\n", filename.c_str());

    download.filename = "";
    download.md5 = "";
    download.got_bytes = 0;

    if (download.buf != NULL)
    {
        delete download.buf;
        download.buf = NULL;
    }

    CL_QuitNetGame();
    CL_Reconnect();
}

//
// CL_RequestDownload
// please sir, can i have some more?
//
void CL_RequestDownload(std::string filename, std::string filehash)
{
    // [Russell] - Allow resumeable downloads
	if ((download.filename != filename) ||
        (download.md5 != filehash))
    {
        download.filename = filename;
        download.md5 = filehash;
        download.got_bytes = 0;
    }
	
	// denis todo clear previous downloads
	MSG_WriteMarker(&net_buffer, clc_wantwad);
	MSG_WriteString(&net_buffer, filename.c_str());
	MSG_WriteString(&net_buffer, filehash.c_str());
	MSG_WriteLong(&net_buffer, download.got_bytes);

	NET_SendPacket(net_buffer, serveraddr);

	Printf(PRINT_HIGH, "Requesting download...\n");
	
	// check for completion
	// [Russell] - We go over the boundary, because sometimes the download will
	// pause at 100% if the server disconnected you previously, you can 
	// reconnect a couple of times and this will let the checksum system do its
	// work
	if ((download.buf != NULL) && 
        (download.got_bytes >= download.buf->maxsize()))
	{
        IntDownloadComplete();
	}
}

//
// CL_DownloadStart
// server tells us the size of the file we are about to download
//
void CL_DownloadStart()
{
	DWORD file_len = MSG_ReadLong();

	if(gamestate != GS_DOWNLOAD)
	{
		Printf(PRINT_HIGH, "Server initiated download failed\n");
		return;
	}

	// don't go for more than 100 megs
	if(file_len > 100*1024*1024)
	{
		Printf(PRINT_HIGH, "Download is over 100MiB, aborting!\n");
		CL_QuitNetGame();
		return;
	}
	
    // [Russell] - Allow resumeable downloads
	if (download.got_bytes == 0)
    {
        if (download.buf != NULL)
        {
            delete download.buf;
            download.buf = NULL;
        }
        
        download.buf = new buf_t ((size_t)file_len);
        
        memset(download.buf->ptr(), 0, file_len);
    }
    
	Printf(PRINT_HIGH, "Downloading %d bytes...\n", file_len);
}

//
// CL_Download
// denis - get a little chunk of the file and store it, much like a hampster. Well, hamster; but hampsters can dance and sing. Also much like Scraps, the Ice Age squirrel thing, stores his acorn. Only with a bit more success. Actually, quite a bit more success, specifically as in that the world doesn't crack apart when we store our chunk and it does when Scraps stores his (or her?) acorn. But when Scraps does it, it is funnier. The rest of Ice Age mostly sucks.
//
void CL_Download()
{
	DWORD offset = MSG_ReadLong();
	size_t len = MSG_ReadShort();
	size_t left = MSG_BytesLeft();
	void *p = MSG_ReadChunk(len);

	if(gamestate != GS_DOWNLOAD)
		return;

	// check ranges
	if(offset + len > download.buf->maxsize() || len > left || p == NULL)
	{
		Printf(PRINT_HIGH, "Bad download packet (%d, %d) encountered (%d), aborting\n", (int)offset, (int)left, (int)download.buf->size());
        
        download.filename = "";
        download.md5 = "";
        download.got_bytes = 0;
        
        if (download.buf != NULL)
        {
            delete download.buf;
            download.buf = NULL;
        }
        
		CL_QuitNetGame();
		return;
	}

	// check for missing packet, re-request
	if(offset > download.got_bytes)
	{
		Printf(PRINT_HIGH, "Missed a packet after %d bytes (got %d), re-requesting\n", download.got_bytes, offset);
		MSG_WriteMarker(&net_buffer, clc_wantwad);
		MSG_WriteString(&net_buffer, download.filename.c_str());
		MSG_WriteString(&net_buffer, download.md5.c_str());
		MSG_WriteLong(&net_buffer, download.got_bytes);
		NET_SendPacket(net_buffer, serveraddr);
		return;
	}

	// send keepalive
	NET_SendPacket(net_buffer, serveraddr);

	// copy into downloaded buffer
	memcpy(download.buf->ptr() + offset, p, len);
	download.got_bytes += len;

	// calculate percentage for the user
	static int old_percent = 0;
	int percent = (download.got_bytes*100)/download.buf->maxsize();
	if(percent != old_percent)
	{
		if(!(percent % 10))
			Printf(PRINT_HIGH, "%d%%", percent);
		else
            Printf(PRINT_HIGH, ".");

		old_percent = percent;
	}

	// check for completion
	// [Russell] - We go over the boundary, because sometimes the download will
	// pause at 100% if the server disconnected you previously, you can 
	// reconnect a couple of times and this will let the checksum system do its
	// work
	if(download.got_bytes >= download.buf->maxsize())
	{
        IntDownloadComplete();
	}
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

EXTERN_CVAR (st_scale)

void CL_Spectate()
{
	player_t &player = CL_FindPlayer(MSG_ReadByte());
	
	player.spectator = MSG_ReadByte();
	
	if (&player == &consoleplayer()) {
		st_scale.Callback (); // refresh status bar size
		if (player.spectator) {
			for (int i=0 ; i<NUMPSPRITES ; i++) // remove all weapon sprites
				(&player)->psprites[i].state = NULL;
			player.playerstate = PST_LIVE; // resurrect dead spectators
			// GhostlyDeath -- Sometimes if the player spectates while he is falling down he squats
			player.deltaviewheight = 1000 << FRACBITS;
		} else {
			displayplayer_id = consoleplayer_id; // get out of spynext
		}
	}
	
	// GhostlyDeath -- If the player matches our display player...
	if (&player == &displayplayer())
		displayplayer_id = consoleplayer_id;
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
	cmds[svc_abort]			= &CL_EndGame;
	cmds[svc_loadmap]			= &CL_LoadMap;
	cmds[svc_playerinfo]		= &CL_PlayerInfo;
	cmds[svc_consoleplayer]		= &CL_ConsolePlayer;
	cmds[svc_updatefrags]		= &CL_UpdateFrags;
	cmds[svc_moveplayer]		= &CL_UpdatePlayer;
	cmds[svc_updatelocalplayer]	= &CL_UpdateLocalPlayer;
	cmds[svc_userinfo]			= &CL_SetupUserInfo;
	cmds[svc_teampoints]		= &CL_TeamPoints;

	cmds[svc_svgametic]			= &CL_ResendSvGametic;
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

	cmds[svc_fireshotgun]		= &CL_FireShotgun;
	cmds[svc_firessg]			= &CL_FireSSG;
	cmds[svc_firechaingun]		= &CL_FireChainGun;
	cmds[svc_disconnectclient]	= &CL_DisconnectClient;
	cmds[svc_activateline]		= &CL_ActivateLine;
	cmds[svc_sector]			= &CL_UpdateSector;
	cmds[svc_movingsector]		= &CL_UpdateMovingSector;
	cmds[svc_switch]			= &CL_Switch;
	cmds[svc_print]				= &CL_Print;

	cmds[svc_startsound]		= &CL_Sound;
	cmds[svc_soundorigin]		= &CL_SoundOrigin;
	cmds[svc_mobjstate]			= &CL_SetMobjState;
	cmds[svc_actor_movedir]		= &CL_Actor_Movedir;
	cmds[svc_actor_target]		= &CL_Actor_Target;
	cmds[svc_actor_tracer]		= &CL_Actor_Tracer;
	cmds[svc_missedpacket]		= &CL_CheckMissedPacket;
	cmds[svc_forceteam]			= &CL_ForceSetTeam;

	cmds[svc_ctfevent]			= &CL_CTFEvent;
	cmds[svc_serversettings]	= &CL_GetServerSettings;
	cmds[svc_disconnect]		= &CL_EndGame;
	cmds[svc_full]				= &CL_FullGame;
	cmds[svc_reconnect]			= &CL_Reconnect;
	cmds[svc_exitlevel]			= &CL_ExitLevel;

	cmds[svc_wadinfo]			= &CL_DownloadStart;
	cmds[svc_wadchunk]			= &CL_Download;

	cmds[svc_challenge]			= &CL_Clear;
	cmds[svc_launcher_challenge]= &CL_Clear;
	
	cmds[svc_spectate]   		= &CL_Spectate;
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

		if (msg_badread)
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
	}

}

extern int outrate;

//
// CL_SendCmd
//
void CL_SendCmd(void)
{
	ticcmd_t *cmd;
	player_t *p;

	if (gametic < 1 )
		return;

	p = &consoleplayer();

	if(!p->mo)
		return;

	// denis - we know server won't accept two changes per tic, so we shouldn't
	//static int last_sent_tic = 0;
	//if(last_sent_tic == gametic)
	//	return;
	
	// GhostlyDeath -- If we are spectating, tell the server of our new position
	if (p->spectator)
	{
		MSG_WriteMarker(&net_buffer, clc_spectate);
		MSG_WriteByte(&net_buffer, 5);
		MSG_WriteLong(&net_buffer, displayplayer().mo->x);
		MSG_WriteLong(&net_buffer, displayplayer().mo->y);
		MSG_WriteLong(&net_buffer, displayplayer().mo->z);
	}
	// GhostlyDeath -- We just throw it all away down here since we need those buttons!

	MSG_WriteMarker(&net_buffer, clc_move);

    MSG_WriteLong(&net_buffer, gametic); // current tic

    // send the previous cmds in the message, so if the last packet
    // was dropped, it can be recovered
    cmd = &localcmds[(gametic-1) % MAXSAVETICS];

	MSG_WriteByte(&net_buffer, cmd->ucmd.buttons);
	MSG_WriteShort(&net_buffer, p->mo->angle >> 16);
	MSG_WriteShort(&net_buffer, p->mo->pitch >> 16);
	MSG_WriteShort(&net_buffer, cmd->ucmd.forwardmove);
	MSG_WriteShort(&net_buffer, cmd->ucmd.sidemove);
	MSG_WriteByte(&net_buffer, cmd->ucmd.impulse);

    // send the current cmds in the message
    cmd = &consoleplayer().cmd;

	MSG_WriteByte(&net_buffer, cmd->ucmd.buttons);
	if(stepmode) MSG_WriteShort(&net_buffer, cmd->ucmd.yaw);
	else MSG_WriteShort(&net_buffer, (p->mo->angle + (cmd->ucmd.yaw << 16)) >> 16);
	MSG_WriteShort(&net_buffer, (p->mo->pitch + (cmd->ucmd.pitch << 16)) >> 16);
	MSG_WriteShort(&net_buffer, cmd->ucmd.forwardmove);
	MSG_WriteShort(&net_buffer, cmd->ucmd.sidemove);
	MSG_WriteByte(&net_buffer, cmd->ucmd.impulse);

    NET_SendPacket(net_buffer, serveraddr);
	outrate += net_buffer.size();
    SZ_Clear(&net_buffer);
}

//
// CL_PlayerTimes
//
void CL_PlayerTimes (void)
{
	for (size_t i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
			players[i].GameTime++;
	}
}

//
//	CL_RunTics
//
void CL_RunTics (void)
{
	static char TicCount = 0;

	// Only do this once a second.
	if ( TicCount++ >= 35 )
	{
		CL_PlayerTimes ();
		TicCount = 0;
	}

	if (ctfmode)
		CTF_RunTics ();
}

void OnChangedSwitchTexture (line_t *line, int useAgain) {}
void OnActivatedLine (line_t *line, AActor *mo, int side, int activationType) {}

VERSION_CONTROL (cl_main_cpp, "$Id$")




