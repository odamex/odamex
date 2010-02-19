// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2009 by The Odamex Team.
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
#include "d_player.h"
#include "p_local.h"
#include "sv_main.h"
#include "sv_master.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "md5.h"
#include "sv_ctf.h"

std::vector<std::string> wadnames, wadhashes;
extern std::vector<std::string> patchfiles;
static buf_t ml_message(MAX_UDP_PACKET);

EXTERN_CVAR (usemasters)
EXTERN_CVAR (hostname)
EXTERN_CVAR (maxclients)

EXTERN_CVAR (port)

//bond===========================
EXTERN_CVAR (timelimit)			
EXTERN_CVAR (fraglimit)			
EXTERN_CVAR (email)
EXTERN_CVAR (itemsrespawn)
EXTERN_CVAR (weaponstay)
EXTERN_CVAR (friendlyfire)
EXTERN_CVAR (allowexit)
EXTERN_CVAR (infiniteammo)
EXTERN_CVAR (nomonsters)
EXTERN_CVAR (monstersrespawn)
EXTERN_CVAR (fastmonsters)
EXTERN_CVAR (allowjump)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (waddownload)
EXTERN_CVAR (emptyreset)
EXTERN_CVAR (cleanmaps)
EXTERN_CVAR (fragexitswitch)
//bond===========================

EXTERN_CVAR (teamsinplay)

EXTERN_CVAR (maxplayers)
EXTERN_CVAR (password)
EXTERN_CVAR (website)

EXTERN_CVAR (natport)

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

	MSG_WriteString(&ml_message, (char *)hostname.cstring());

	byte playersingame = 0;
	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
			playersingame++;
	}

	MSG_WriteByte(&ml_message, playersingame);
	MSG_WriteByte(&ml_message, maxclients);

	MSG_WriteString(&ml_message, level.mapname);

	size_t numwads = wadnames.size();
	if(numwads > 0xff)numwads = 0xff;

	MSG_WriteByte(&ml_message, numwads - 1);

	for (i = 1; i < numwads; ++i)
		MSG_WriteString(&ml_message, wadnames[i].c_str());

	MSG_WriteBool(&ml_message, (gametype == GM_DM || gametype == GM_TEAMDM));
	MSG_WriteByte(&ml_message, (BYTE)skill);
	MSG_WriteBool(&ml_message, (gametype == GM_TEAMDM));
	MSG_WriteBool(&ml_message, (gametype == GM_CTF));

	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
		{
			MSG_WriteString(&ml_message, players[i].userinfo.netname);
			MSG_WriteShort(&ml_message, players[i].fragcount);
			MSG_WriteLong(&ml_message, players[i].ping);

			if (gametype == GM_TEAMDM || gametype == GM_CTF)
				MSG_WriteByte(&ml_message, players[i].userinfo.team);
			else
				MSG_WriteByte(&ml_message, TEAM_NONE);
		}
	}

	for (i = 1; i < numwads; ++i)
		MSG_WriteString(&ml_message, wadhashes[i].c_str());

	MSG_WriteString(&ml_message, website.cstring());

	if (gametype == GM_TEAMDM || gametype == GM_CTF)
	{
		MSG_WriteLong(&ml_message, scorelimit);
		
		for(size_t i = 0; i < NUMTEAMS; i++)
		{
			if ((gametype == GM_CTF && i < 2) || (gametype != GM_CTF && i < teamsinplay)) {
				MSG_WriteByte(&ml_message, 1);
				MSG_WriteLong(&ml_message, TEAMpoints[i]);
			} else {
				MSG_WriteByte(&ml_message, 0);
			}
		}
	}
	
	MSG_WriteShort(&ml_message, VERSION);

//bond===========================
	MSG_WriteString(&ml_message, (char *)email.cstring());

	int timeleft = (int)(timelimit - level.time/(TICRATE*60));
	if (timeleft<0) timeleft=0;

	MSG_WriteShort(&ml_message,(int)timelimit);
	MSG_WriteShort(&ml_message,timeleft);
	MSG_WriteShort(&ml_message,(int)fraglimit);

	MSG_WriteBool(&ml_message, (itemsrespawn ? true : false));
	MSG_WriteBool(&ml_message, (weaponstay ? true : false));
	MSG_WriteBool(&ml_message, (friendlyfire ? true : false));
	MSG_WriteBool(&ml_message, (allowexit ? true : false));
	MSG_WriteBool(&ml_message, (infiniteammo ? true : false));
	MSG_WriteBool(&ml_message, (nomonsters ? true : false));
	MSG_WriteBool(&ml_message, (monstersrespawn ? true : false));
	MSG_WriteBool(&ml_message, (fastmonsters ? true : false));
	MSG_WriteBool(&ml_message, (allowjump ? true : false));
	MSG_WriteBool(&ml_message, (sv_freelook ? true : false));
	MSG_WriteBool(&ml_message, (waddownload ? true : false));
	MSG_WriteBool(&ml_message, (emptyreset ? true : false));
	MSG_WriteBool(&ml_message, (cleanmaps ? true : false));
	MSG_WriteBool(&ml_message, (fragexitswitch ? true : false));

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
    MSG_WriteShort(&ml_message, (WORD)maxplayers);
    
    for (i = 0; i < players.size(); ++i)
    {
        if (players[i].ingame())
        {
            MSG_WriteBool(&ml_message, (players[i].spectator ? true : false));
        }
    }

    MSG_WriteLong(&ml_message, (DWORD)0x01020305);
    MSG_WriteShort(&ml_message, strlen(password.cstring()) ? 1 : 0);
    
    // GhostlyDeath -- Send Game Version info
    MSG_WriteLong(&ml_message, GAMEVER);

    MSG_WriteByte(&ml_message, patchfiles.size());
    
    for (size_t i = 0; i < patchfiles.size(); ++i)
        MSG_WriteString(&ml_message, patchfiles[i].c_str());

	NET_SendPacket(ml_message, net_from);
}

VERSION_CONTROL (sv_sqpold_cpp, "$Id$")
