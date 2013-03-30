// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//  Old version of the server query protocol, kept for clients and older 
//  launchers
//
//-----------------------------------------------------------------------------

#include <string>
#include <vector>

#include "doomtype.h"
#include "doomstat.h"
#include "d_main.h"
#include "d_player.h"
#include "p_local.h"
#include "sv_main.h"
#include "sv_master.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "md5.h"
#include "p_ctf.h"

static buf_t ml_message(MAX_UDP_PACKET);

EXTERN_CVAR (sv_scorelimit) // [CG] Should this go below in //bond ?

EXTERN_CVAR (sv_usemasters)
EXTERN_CVAR (sv_hostname)
EXTERN_CVAR (sv_maxclients)

EXTERN_CVAR (port)

//bond===========================
EXTERN_CVAR (sv_timelimit)			
EXTERN_CVAR (sv_fraglimit)			
EXTERN_CVAR (sv_email)
EXTERN_CVAR (sv_itemsrespawn)
EXTERN_CVAR (sv_weaponstay)
EXTERN_CVAR (sv_friendlyfire)
EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_infiniteammo)
EXTERN_CVAR (sv_nomonsters)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (sv_waddownload)
EXTERN_CVAR (sv_emptyreset)
EXTERN_CVAR (sv_cleanmaps)
EXTERN_CVAR (sv_fragexitswitch)
//bond===========================

EXTERN_CVAR (sv_teamsinplay)

EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (join_password)
EXTERN_CVAR (sv_website)

EXTERN_CVAR (sv_natport)

//
// denis - each launcher reply contains a random token so that
// the server will only allow connections with a valid token
// in order to protect itself from ip spoofing
//
struct token_t
{
	DWORD id;
	QWORD issued;
	netadr_t from;
};

#define MAX_TOKEN_AGE	(20 * TICRATE) // 20s should be enough for any client to load its wads
static std::vector<token_t> connect_tokens;

//
// SV_NewToken
//
DWORD SV_NewToken()
{
	QWORD now = I_GetTime();

	token_t token;
	token.id = rand()*time(0);
	token.issued = now;
	token.from = net_from;
	
	// find an old token to replace
	for(size_t i = 0; i < connect_tokens.size(); i++)
	{
		if(now - connect_tokens[i].issued >= MAX_TOKEN_AGE)
		{
			connect_tokens[i] = token;
			return token.id;
		}
	}

	connect_tokens.push_back(token);

	return token.id;
}

//
// SV_ValidToken
//
bool SV_IsValidToken(DWORD token)
{
	QWORD now = I_GetTime();

	for(size_t i = 0; i < connect_tokens.size(); i++)
	{
		if(connect_tokens[i].id == token
		&& NET_CompareAdr(connect_tokens[i].from, net_from)
		&& now - connect_tokens[i].issued < MAX_TOKEN_AGE)
		{
			// extend token life and confirm
			connect_tokens[i].issued = now;
			return true;
		}
	}
	
	return false;
}

//
// SV_SendServerInfo
// 
// Sends server info to a launcher
// TODO: Clean up and reinvent.
void SV_SendServerInfo()
{
	size_t i;

	SZ_Clear(&ml_message);
	
	MSG_WriteLong(&ml_message, CHALLENGE);
	MSG_WriteLong(&ml_message, SV_NewToken());

	// if master wants a key to be presented, present it we will
	if(MSG_BytesLeft() == 4)
		MSG_WriteLong(&ml_message, MSG_ReadLong());

	MSG_WriteString(&ml_message, (char *)sv_hostname.cstring());

	byte playersingame = 0;
	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
			playersingame++;
	}

	MSG_WriteByte(&ml_message, playersingame);
	MSG_WriteByte(&ml_message, sv_maxclients.asInt());

	MSG_WriteString(&ml_message, level.mapname);

	size_t numwads = wadfiles.size();
	if(numwads > 0xff)numwads = 0xff;

	MSG_WriteByte(&ml_message, numwads - 1);

	for (i = 1; i < numwads; ++i)
		MSG_WriteString(&ml_message, D_CleanseFileName(wadfiles[i], "wad").c_str());

	MSG_WriteBool(&ml_message, (sv_gametype == GM_DM || sv_gametype == GM_TEAMDM));
	MSG_WriteByte(&ml_message, sv_skill.asInt());
	MSG_WriteBool(&ml_message, (sv_gametype == GM_TEAMDM));
	MSG_WriteBool(&ml_message, (sv_gametype == GM_CTF));

	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
		{
			MSG_WriteString(&ml_message, players[i].userinfo.netname.c_str());
			MSG_WriteShort(&ml_message, players[i].fragcount);
			MSG_WriteLong(&ml_message, players[i].ping);

			if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
				MSG_WriteByte(&ml_message, players[i].userinfo.team);
			else
				MSG_WriteByte(&ml_message, TEAM_NONE);
		}
	}

	for (i = 1; i < numwads; ++i)
		MSG_WriteString(&ml_message, wadhashes[i].c_str());

	MSG_WriteString(&ml_message, sv_website.cstring());

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		MSG_WriteLong(&ml_message, sv_scorelimit.asInt());
		
		for(size_t i = 0; i < NUMTEAMS; i++)
		{
			if ((sv_gametype == GM_CTF && i < 2) || (sv_gametype != GM_CTF && i < sv_teamsinplay)) {
				MSG_WriteByte(&ml_message, 1);
				MSG_WriteLong(&ml_message, TEAMpoints[i]);
			} else {
				MSG_WriteByte(&ml_message, 0);
			}
		}
	}
	
	MSG_WriteShort(&ml_message, VERSION);

//bond===========================
	MSG_WriteString(&ml_message, (char *)sv_email.cstring());

	int timeleft = (int)(sv_timelimit - level.time/(TICRATE*60));
	if (timeleft<0) timeleft=0;

	MSG_WriteShort(&ml_message,sv_timelimit.asInt());
	MSG_WriteShort(&ml_message,timeleft);
	MSG_WriteShort(&ml_message,sv_fraglimit.asInt());

	MSG_WriteBool(&ml_message, (sv_itemsrespawn ? true : false));
	MSG_WriteBool(&ml_message, (sv_weaponstay ? true : false));
	MSG_WriteBool(&ml_message, (sv_friendlyfire ? true : false));
	MSG_WriteBool(&ml_message, (sv_allowexit ? true : false));
	MSG_WriteBool(&ml_message, (sv_infiniteammo ? true : false));
	MSG_WriteBool(&ml_message, (sv_nomonsters ? true : false));
	MSG_WriteBool(&ml_message, (sv_monstersrespawn ? true : false));
	MSG_WriteBool(&ml_message, (sv_fastmonsters ? true : false));
	MSG_WriteBool(&ml_message, (sv_allowjump ? true : false));
	MSG_WriteBool(&ml_message, (sv_freelook ? true : false));
	MSG_WriteBool(&ml_message, (sv_waddownload ? true : false));
	MSG_WriteBool(&ml_message, (sv_emptyreset ? true : false));
	MSG_WriteBool(&ml_message, (sv_cleanmaps ? true : false));
	MSG_WriteBool(&ml_message, (sv_fragexitswitch ? true : false));

	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
		{
			MSG_WriteShort(&ml_message, players[i].killcount);
			MSG_WriteShort(&ml_message, players[i].deathcount);
			
			int timeingame = (time(NULL) - players[i].JoinTime)/60;
			if (timeingame<0) timeingame=0;
				MSG_WriteShort(&ml_message, timeingame);
		}
	}
	
//bond===========================

    MSG_WriteLong(&ml_message, (DWORD)0x01020304);
    MSG_WriteShort(&ml_message, sv_maxplayers.asInt());
    
    for (i = 0; i < players.size(); ++i)
    {
        if (players[i].ingame())
        {
            MSG_WriteBool(&ml_message, (players[i].spectator ? true : false));
        }
    }

    MSG_WriteLong(&ml_message, (DWORD)0x01020305);
    MSG_WriteShort(&ml_message, strlen(join_password.cstring()) ? 1 : 0);
    
    // GhostlyDeath -- Send Game Version info
    MSG_WriteLong(&ml_message, GAMEVER);

    MSG_WriteByte(&ml_message, patchfiles.size());
    
    for (size_t i = 0; i < patchfiles.size(); ++i)
        MSG_WriteString(&ml_message, D_CleanseFileName(patchfiles[i]).c_str());

	NET_SendPacket(ml_message, net_from);
}

VERSION_CONTROL (sv_sqpold_cpp, "$Id$")
