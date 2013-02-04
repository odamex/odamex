// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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

typedef struct
{ 
    fixed_t 	m_Speed;
    fixed_t 	m_Low;
    fixed_t 	m_High;
    int 		m_Wait;
    int 		m_Count;
    int     	m_Status;
    int     	m_OldStatus;
    bool 		m_Crush;
    int 		m_Tag;
    int	        m_Type;
    bool		m_PostWait;

    int 		m_Direction;
    short 		m_NewSpecial;
    short		m_Texture;
    fixed_t 	m_FloorDestHeight;

    int			m_ResetCount;
    int			m_OrgHeight;
    int			m_Delay;
    int			m_PauseTime;
    int			m_StepTime;
    int			m_PerStepTime;
} pred_floor_t;

typedef struct
{
    int 		m_Type;
    fixed_t 	m_TopHeight;
    fixed_t 	m_Speed;

    int 		m_Direction;
    int 		m_TopWait;
    int 		m_TopCountdown;

    fixed_t 	m_BottomHeight;
    fixed_t		m_Speed1;		// [RH] dnspeed of crushers
    fixed_t		m_Speed2;		// [RH] upspeed of crushers
    bool 		m_Crush;
    int			m_Silent;

    // [RH] Need these for BOOM-ish transferring ceilings
    int			m_Texture;
    int			m_NewSpecial;

    // ID
    int 		m_Tag;
    int 		m_OldDirection;
	int			m_Status;

    line_t      *m_Line;
} pred_ceiling_t;

typedef struct
{
	int     	m_Type;
	int			m_Direction;
	fixed_t		m_FloorDestHeight;
	fixed_t		m_CeilingDestHeight;
	fixed_t		m_Speed;

	fixed_t		m_FloorSpeed;
	fixed_t		m_CeilingSpeed;
	fixed_t		m_FloorTarget;
	fixed_t		m_CeilingTarget;
	bool		m_Crush;
} pred_both_t;

struct plat_pred_t
{
	size_t secnum;
	int tic;

    fixed_t floorheight;
    fixed_t ceilingheight;

    pred_floor_t Floor;
    pred_ceiling_t Ceiling;
    pred_both_t Both;
};

extern TArray <plat_pred_t> real_plats;

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

bool CL_SectorIsPredicting(sector_t *sector);

std::string M_ExpandTokens(const std::string &str);

#endif

