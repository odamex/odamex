// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2021 by The Odamex Team.
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
//	Determine map format and handle it accordingly.
//  (Props to DSDA-Doom for the inspiration.)
//
//-----------------------------------------------------------------------------
#ifndef __P_MAPFORMAT__
#define __P_MAPFORMAT__

#include "doomtype.h"
#include "p_spec.h"

class MapFormat
{
  public:
	void P_ApplyZDoomMapFormat(void);
	void P_ApplyDefaultMapFormat(void);

	bool GetZDoom(void);
	bool GetHexen(void);
	bool GetPolyObjs(void);
	bool GetACS(void);
	bool GetMAPINFO(void);
	bool GetSNDSEQ(void);
	bool GetSNDINFO(void);
	bool GetANIMDEFS(void);
	bool GetDoubleSky(void);
	bool GetMAP99(void);
	short GetGeneralizedMask(void);
	unsigned int GetSwitchActivation(void);

	void init_sector_special(sector_t*);
	void player_in_special_sector(player_t*);
	bool actor_in_special_sector(AActor*);
	void spawn_scroller(line_t*);
	void spawn_friction(line_t*);
	void spawn_pusher(line_t*);
	void spawn_extra(int);
	void cross_special_line(line_t*, int, AActor*, bool);
	void shoot_special_line(AActor*, line_t*);
	bool test_activate_line(int, short*, line_t*, int, AActor*);
	bool execute_line_special(int, short*, line_t*, int, AActor*);
	void post_process_line_special(int);
	void post_process_sidedef_special(side_t*, const mapsidedef_t*, sector_t*, int);
	void animate_surfaces(void);
	void check_impact(AActor*);
	void translate_line_flags(unsigned int*);
	void apply_sector_movement_special(AActor*, int);

  protected:
	bool zdoom;
	bool hexen;
	bool polyobjs;
	bool acs;
	bool mapinfo;
	bool sndseq;
	bool sndinfo;
	bool animdefs;
	bool doublesky;
	bool map99;
	short generalized_mask;
	unsigned int switch_activation;
};

extern MapFormat map_format;

int  P_DoorType(int index);
bool P_IsExitLine(int index);
bool P_IsTeleportLine(int index);

#endif