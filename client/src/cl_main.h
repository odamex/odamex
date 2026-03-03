// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2026 by The Odamex Team.
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

#pragma once

#include "i_net.h"
#include "d_player.h"
#include "d_ticcmd.h"
#include "r_defs.h"
#include "cl_demo.h"

#include "client.pb.h"

#include "OdaMessenger.h"

extern netadr_t  serveraddr;
extern bool      connected;
extern int       connecttimeout;

extern bool      noservermsgs;
extern int       last_received;

extern NetDemo      netdemo;
extern OdaMessenger messenger;

#define MAXSAVETICS 70
extern odaproto::clc::PlayerInput localcmds[MAXSAVETICS];

extern bool predicting;

enum netQuitReason_e
{
	NQ_SILENT,          // Don't print a message.
	NQ_DISCONNECT,      // Generic message for "typical" forced disconnects initiated by the client.
	NQ_ABORT,           // Connection attempt was aborted
	NQ_PROTO,           // Encountered something unexpected in the protocol
	NQ_SERVER_DROP,     // Server dropped us on the floor, so just ack and drop our side of the connection.
};

void CL_QuitNetGame(const netQuitReason_e reason);
void CL_CompleteDisconnect(netQuitReason_e reason);
void CL_Reconnect();
void CL_InitNetwork (void);
void CL_RequestConnectInfo(void);
bool CL_PrepareConnect();
void CL_ParseCommands(void);
MessageResultEnum CL_ReadPacketHeader();
MessageResultEnum CL_AcceptNetMessage();
MessageResultEnum CL_ProcessCurrentReliableMessages();
void CL_SendCmd(void);
void CL_SaveCmd(void);
void CL_MoveThing(AActor *mobj, fixed_t x, fixed_t y, fixed_t z);
void CL_PredictWorld(void);
void CL_SendUserInfo(buf_t& netBuf);
bool CL_Connect();

void CL_SendCheat(int cheats);
void CL_SendGiveCheat(const char* item);
void CL_SendSummonCheat(const char* summon);
void CL_SendSummonFriendCheat(const char* summon);

void CL_DisplayTics();
void CL_RunTics();

bool CL_SectorIsPredicting(sector_t *sector);
argb_t CL_GetPlayerColor(const player_t& player);

std::string M_ExpandTokens(const std::string &str);

void SexMessage(const char* from, char* to, gender_t gender, std::string_view victim,
                std::string_view killer, std::string_view spree);
