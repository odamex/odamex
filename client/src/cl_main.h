// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	CL_MAIN
//
//-----------------------------------------------------------------------------

#ifndef __I_CLMAIN_H__
#define __I_CLMAIN_H__

#include "i_net.h"
#include "d_ticcmd.h"
#include "r_defs.h"
#include "cl_demo.h"
#include <string>

extern netadr_t  serveraddr;
extern BOOL      connected;
extern int       connecttimeout;

extern bool      noservermsgs;
extern int       last_received;

extern buf_t     net_buffer;

extern NetDemo	netdemo;

#define MAXSAVETICS 70

extern bool predicting;


void CL_QuitNetGame(void);
void CL_InitNetwork (void);
void CL_RequestConnectInfo(void);
bool CL_PrepareConnect(void);
void CL_ParseCommands(void);
void CL_ReadPacketHeader(void);
void CL_SendCmd(void);
void CL_SaveCmd(void);
void CL_MoveThing(AActor *mobj, fixed_t x, fixed_t y, fixed_t z);
void CL_PredictWorld(void);
void CL_SendUserInfo(void);
bool CL_Connect(void);

void CL_DisplayTics();
void CL_RunTics();

bool CL_SectorIsPredicting(sector_t *sector);

std::string M_ExpandTokens(const std::string &str);

#endif

