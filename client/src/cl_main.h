// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	CL_MAIN
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_net.h"
#include "d_player.h"
#include "d_ticcmd.h"
#include "r_defs.h"
#include "cl_demo.h"

extern netadr_t  serveraddr;
extern BOOL      connected;
extern int       connecttimeout;

extern bool      noservermsgs;
extern int       last_received;

extern buf_t     net_buffer;

extern NetDemo	netdemo;

#define MAXSAVETICS 70

extern bool predicting;

enum netQuitReason_e
{
	NQ_SILENT,     // Don't print a message.
	NQ_DISCONNECT, // Generic message for "typical" forced disconnects initiated by the client.
	NQ_ABORT,      // Connection attempt was aborted
	NQ_PROTO,      // Encountered something unexpected in the protocol
};

#define CL_QuitNetGame(reason) CL_QuitNetGame2(reason, __FILE__, __LINE__)
void CL_QuitNetGame2(const netQuitReason_e reason, const char* file, const int line);
void CL_Reconnect();
void CL_InitNetwork (void);
void CL_RequestConnectInfo(void);
bool CL_PrepareConnect();
void CL_ParseCommands(void);
bool CL_ReadPacketHeader();
void CL_SendCmd(void);
void CL_SaveCmd(void);
void CL_MoveThing(AActor *mobj, fixed_t x, fixed_t y, fixed_t z);
void CL_PredictWorld(void);
void CL_SendUserInfo(void);
bool CL_Connect();

void CL_SendCheat(int cheats);
void CL_SendGiveCheat(const char* item);

void CL_DisplayTics();
void CL_RunTics();

bool CL_SectorIsPredicting(sector_t *sector);
argb_t CL_GetPlayerColor(player_t* player);

std::string M_ExpandTokens(const std::string &str);
