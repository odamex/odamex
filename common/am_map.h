// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//  AutoMap module.
//
//-----------------------------------------------------------------------------


#pragma once

#include "d_event.h"

// Used by ST StatusBar stuff.
#define AM_MSGHEADER (('a'<<24)+('m'<<16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e'<<8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x'<<8))

// Group palette index and RGB value together:
typedef struct am_color_s
{
	palindex_t index;
	argb_t rgb;
} am_color_t;

typedef struct am_colors_s
{
	am_color_t Background;
	am_color_t YourColor;
	am_color_t WallColor;
	am_color_t TSWallColor;
	am_color_t FDWallColor;
	am_color_t CDWallColor;
	am_color_t ThingColor;
	am_color_t ThingColor_Item;
	am_color_t ThingColor_CountItem;
	am_color_t ThingColor_Monster;
	am_color_t ThingColor_NoCountMonster;
	am_color_t ThingColor_Friend;
	am_color_t ThingColor_Projectile;
	am_color_t SecretWallColor;
	am_color_t GridColor;
	am_color_t XHairColor;
	am_color_t NotSeenColor;
	am_color_t LockedColor;
	am_color_t AlmostBackground;
	am_color_t TeleportColor;
	am_color_t ExitColor;
} am_colors_t;

typedef struct am_default_colors_s
{
	std::string Background;
	std::string YourColor;
	std::string WallColor;
	std::string TSWallColor;
	std::string FDWallColor;
	std::string CDWallColor;
	std::string ThingColor;
	std::string ThingColor_Item;
	std::string ThingColor_CountItem;
	std::string ThingColor_Monster;
	std::string ThingColor_NoCountMonster;
	std::string ThingColor_Friend;
	std::string ThingColor_Projectile;
	std::string SecretWallColor;
	std::string GridColor;
	std::string XHairColor;
	std::string NotSeenColor;
	std::string LockedColor;
	std::string AlmostBackground;
	std::string TeleportColor;
	std::string ExitColor;
} am_default_colors_t;

typedef v2fixed64_t mpoint_t;

typedef struct
{
	mpoint_t a, b;
} mline_t;

extern am_default_colors_t AutomapDefaultColors;
extern am_colors_t AutomapDefaultCurrentColors;

extern int am_cheating;

// Called by main loop.
BOOL AM_Responder(event_t* ev);

// Called by main loop.
void AM_Ticker();

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer();

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop();

bool AM_ClassicAutomapVisible();
bool AM_OverlayAutomapVisible();

void AM_SetBaseColorDoom();
void AM_SetBaseColorRaven();
void AM_SetBaseColorStrife();